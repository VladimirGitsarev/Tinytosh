#include "BambuService.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "ConfigManager.h"

BambuService* BambuService::instance = nullptr;

BambuService::BambuService() : lastReconnectAttempt(0), isScanning(false), scanTaskHandle(NULL), lastMessageTime(0) {
    instance = this;
}

void BambuService::begin(Config* conf, BambuData* bData) {
    config = conf;
    data = bData;
    
    espClient.setInsecure(); 
    espClient.setHandshakeTimeout(BAMBU_TLS_TIMEOUT_SEC); 
    
    client.setClient(espClient);
    client.setBufferSize(BAMBU_BUFFER_SIZE); 
    client.setCallback(BambuService::mqttCallbackStatic);
}

void BambuService::loop() {
    if (!config->show_bambu || config->bambu_sn == "") {
        if (client.connected()) {
            client.disconnect();
            Serial.println("BambuService: Disconnected (Module disabled or missing credentials)");
        }
        return;
    }

    if (data->status != "SYNCING" && lastMessageTime > 0 && (millis() - lastMessageTime > BAMBU_DATA_TIMEOUT_MS)) {
        Serial.println("BambuService: Connection timed out. Resetting to SYNCING");
        
        data->status = "SYNCING";
        data->progress = 0;
        data->time_left = 0;
        data->nozzle_temp = 0.0;
        data->nozzle_target = 0.0;
        data->bed_temp = 0.0;
        data->bed_target = 0.0;
        data->layer = 0;
        data->total_layers = 0;
        data->file_name = "None";
        data->fan_part = 0;
        data->fan_aux = 0;

        if (client.connected()) {
            client.disconnect();
        }
    }

    if (client.connected()) {
        client.loop();
        return;
    }

    if (millis() - lastReconnectAttempt > BAMBU_RECONNECT_INTERVAL_MS || lastReconnectAttempt == 0) {
        lastReconnectAttempt = millis();

        if (isScanning) return; 

        if (config->bambu_ip != "") {
            WiFiClient probeClient;
            if (!probeClient.connect(config->bambu_ip.c_str(), BAMBU_MQTT_PORT, BAMBU_PROBE_KNOWN_IP_MS)) {
                Serial.println("BambuService: Printer offline at saved IP (" + config->bambu_ip + ").");
                startScannerSafely();
                return;
            }
            probeClient.stop(); 

            Serial.println("BambuService: Printer answered at " + config->bambu_ip + "! Negotiating TLS...");
            client.setServer(config->bambu_ip.c_str(), BAMBU_MQTT_PORT);
            
            if (client.connect("TinytoshClient", "bblp", config->bambu_code.c_str())) {
                Serial.println("BambuService: Connected!");
                
                lastMessageTime = millis(); 

                String topic = "device/" + config->bambu_sn + "/report";
                client.subscribe(topic.c_str());

                String reqTopic = "device/" + config->bambu_sn + "/request";
                String payload = "{\"pushing\": {\"sequence_id\": \"1\", \"command\": \"pushall\"}}";
                client.publish(reqTopic.c_str(), payload.c_str());
                
            } else {
                Serial.print("BambuService: TLS Connect failed, rc=");
                Serial.println(client.state());
            }
        } else {
            Serial.println("BambuService: No saved IP.");
            startScannerSafely();
        }
    }
}

void BambuService::startScannerSafely() {
    if (isScanning) return;

    if (WiFi.status() == WL_CONNECTED) {
        String localIp = WiFi.localIP().toString();
        if (localIp != "0.0.0.0" && localIp != "(IP unset)") {
            int lastDot = localIp.lastIndexOf('.');
            if (lastDot > 0) {
                scanBaseIp = localIp.substring(0, lastDot + 1);
                isScanning = true;
                Serial.println("BambuService: Starting background scanner on subnet " + scanBaseIp + "x");
                
                xTaskCreate(
                    BambuService::scanNetworkTask, 
                    "BambuScanner", 
                    4096, 
                    this, 
                    1, 
                    &scanTaskHandle
                );
                return;
            }
        }
    }
    Serial.println("BambuService: Cannot scan right now. Waiting for stable WiFi IP.");
}

void BambuService::scanNetworkTask(void* parameter) {
    BambuService* service = (BambuService*)parameter;

    for (int i = 1; i <= 254; i++) {
        if (!service->isScanning) break;

        String testIp = service->scanBaseIp + String(i);
        if (i % 50 == 0) Serial.println("BambuService: Scanning network... (" + testIp + ")");

        WiFiClient probeClient;
        if (probeClient.connect(testIp.c_str(), BAMBU_MQTT_PORT, BAMBU_PROBE_SCAN_MS)) {
            probeClient.stop();
            Serial.println("BambuService: 🎉 PRINTER FOUND at " + testIp);
            service->config->bambu_ip = testIp;
            service->lastReconnectAttempt = millis() - BAMBU_RECONNECT_INTERVAL_MS + BAMBU_SCAN_COOLDOWN_MS;
            service->isScanning = false; 

            extern ConfigManager configManager; 
            configManager.saveConfig(*(service->config));

            break;
        }

        vTaskDelay(pdMS_TO_TICKS(BAMBU_SCAN_YIELD_MS)); 
    }

    if (service->isScanning) {
        Serial.println("BambuService: Scan complete. Printer not found on network.");
    }
    
    service->isScanning = false;
    service->scanTaskHandle = NULL;
    vTaskDelete(NULL);
}

void BambuService::mqttCallbackStatic(char* topic, byte* payload, unsigned int length) {
    if (instance) instance->mqttCallback(topic, payload, length);
}

void BambuService::mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];

    DynamicJsonDocument doc(BAMBU_JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("BambuService JSON Parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("print")) {
        lastMessageTime = millis();

        JsonObject printData = doc["print"];
        
        if (printData.containsKey("gcode_state")) data->status = printData["gcode_state"].as<String>();
        if (printData.containsKey("mc_percent")) data->progress = printData["mc_percent"].as<int>();
        if (printData.containsKey("mc_remaining_time")) data->time_left = printData["mc_remaining_time"].as<int>();
        
        if (printData.containsKey("nozzle_temper")) data->nozzle_temp = printData["nozzle_temper"].as<float>();
        if (printData.containsKey("nozzle_target_temper")) data->nozzle_target = printData["nozzle_target_temper"].as<float>();
        if (printData.containsKey("bed_temper")) data->bed_temp = printData["bed_temper"].as<float>();
        if (printData.containsKey("bed_target_temper")) data->bed_target = printData["bed_target_temper"].as<float>();
        
        if (printData.containsKey("layer_num")) data->layer = printData["layer_num"].as<int>();
        if (printData.containsKey("total_layer_num")) data->total_layers = printData["total_layer_num"].as<int>();
        if (printData.containsKey("subtask_name")) data->file_name = printData["subtask_name"].as<String>();
        
        if (printData.containsKey("cooling_fan_speed")) {
            data->fan_part = map(printData["cooling_fan_speed"].as<int>(), 0, 15, 0, 100);
        }
        if (printData.containsKey("big_fan1_speed")) {
            data->fan_aux = map(printData["big_fan1_speed"].as<int>(), 0, 15, 0, 100);
        }

    }
}