#ifndef WEB_SERVER_SERVICE_H
#define WEB_SERVER_SERVICE_H

#include <WebServer.h>
#include <WiFiManager.h>
#include "structs.h"

typedef void (*ConfigSaveCallback)();

class WebServerService {
public:
    WebServerService(int port, ConfigSaveCallback callback);
    void begin();
    void handleClient();
    void setAppState(AppState* appState);
    
    void handleRoot();
    void handleSave();
    void handleUpdate();

    String generateRootPageContent();
    
private:
    WebServer server;
    WiFiManager wm;
    ConfigSaveCallback saveCallback;
    
    AppState* state;
    
    String getWeatherIcon(int wmo_code);
    String getCurrentTimeShort(String format);
    String getFullDate();
};

#endif