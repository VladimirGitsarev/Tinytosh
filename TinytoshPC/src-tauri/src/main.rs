#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::thread;
use std::time::Duration;
use std::sync::Mutex; 
use std::env; 
use std::collections::HashMap;
use tauri::menu::{Menu, MenuItem};
use tauri::tray::{TrayIconBuilder, TrayIconEvent, MouseButton};
use tauri::{Manager, WindowEvent, Size, LogicalSize}; 
use sysinfo::{System, Disks, Networks}; 
use serialport::{SerialPort, SerialPortType};
use tauri_plugin_autostart::ManagerExt;
use mdns_sd::{ServiceDaemon, ServiceEvent};
use mac_address::get_mac_address;

struct AppState {
    stats: Mutex<String>,
    port: Mutex<Option<Box<dyn SerialPort>>>,
    active_port_name: Mutex<String>,
    manual_disconnect: Mutex<bool>,
    status_msg: Mutex<String>, 
    discovered_wifi: Mutex<HashMap<String, String>>,
    target_wifi_ip: Mutex<String>,
    command_queue: Mutex<Vec<String>>,
    serial_buffer: Mutex<String>,
    latest_config: Mutex<String>,
    user_forced_wifi: Mutex<bool>,
}

#[derive(serde::Serialize)]
struct BridgeStats {
    pc_id: String,
    cpu_percent: f32,
    net_down_kb: u64, 
    mem_percent: f64,
    disk_percent: u64,
}

#[derive(serde::Serialize)]
struct PortStatus {
    ports: Vec<String>,
    connected: Option<String>,
    status_text: String,
    target_ip: String,
}

#[tauri::command]
fn set_autostart(app: tauri::AppHandle, enable: bool) -> Result<(), String> {
    let autostart_manager = app.autolaunch();
    if enable {
        autostart_manager.enable().map_err(|e| e.to_string())?;
    } else {
        autostart_manager.disable().map_err(|e| e.to_string())?;
    }
    Ok(())
}

#[tauri::command]
fn check_autostart(app: tauri::AppHandle) -> bool {
    app.autolaunch().is_enabled().unwrap_or(false)
}

#[tauri::command]
fn get_stats(state: tauri::State<AppState>) -> String {
    state.stats.lock().unwrap().clone()
}

#[tauri::command]
fn get_ports(state: tauri::State<AppState>) -> PortStatus {
    let mut ports: Vec<String> = serialport::available_ports()
        .map(|p| p.into_iter().map(|x| format!("Serial: {}", x.port_name)).collect())
        .unwrap_or(vec![]);
    
    let active = state.active_port_name.lock().unwrap().clone();
    
    if !active.is_empty() && !active.starts_with("WiFi:") && !ports.contains(&active) {
        ports.insert(0, active.clone());
    }
    
    let wifi_devices = state.discovered_wifi.lock().unwrap();
    for (ip, name) in wifi_devices.iter() {
        ports.push(format!("WiFi: {} ({})", name, ip));
    }
    
    let connected = if active.is_empty() { None } else { Some(active) };
    let status_text = state.status_msg.lock().unwrap().clone();
    let target_ip = state.target_wifi_ip.lock().unwrap().clone();
    
    PortStatus { ports, connected, status_text, target_ip }
}

#[tauri::command]
async fn toggle_connection(state: tauri::State<'_, AppState>, port_name: String, connect: bool) -> Result<String, String> {
    let mut port_guard = state.port.lock().unwrap();
    let mut name_guard = state.active_port_name.lock().unwrap();
    let mut manual_guard = state.manual_disconnect.lock().unwrap();
    let mut status_guard = state.status_msg.lock().unwrap();
    let mut target_wifi = state.target_wifi_ip.lock().unwrap();
    let mut forced_wifi = state.user_forced_wifi.lock().unwrap();
    
    if !connect {
        *port_guard = None;
        *name_guard = String::new();
        *target_wifi = String::new();
        *manual_guard = true;
        *forced_wifi = false; 
        *status_guard = "⏸️ Disconnected".to_string(); 
        return Ok("Disconnected".to_string());
    }

    if port_name.starts_with("WiFi: ") {
        let ip = if let Some(start) = port_name.rfind('(') {
            if let Some(end) = port_name.rfind(')') {
                port_name[start + 1..end].to_string()
            } else { String::new() }
        } else { String::new() };

        if !ip.is_empty() {
            *port_guard = None;
            *name_guard = port_name; 
            *target_wifi = ip;
            *manual_guard = false;
            *forced_wifi = true; 
            *status_guard = "📶 Connecting via WiFi...".to_string();
            return Ok("Connected".to_string());
        }
    }

    let actual_port = if port_name.starts_with("Serial: ") {
        port_name.replace("Serial: ", "")
    } else {
        port_name.clone()
    };

    match serialport::new(actual_port, 115200).timeout(Duration::from_millis(100)).open() {
        Ok(p) => {
            *port_guard = Some(p);
            *name_guard = port_name;
            *manual_guard = false;
            *forced_wifi = false; 
            *status_guard = String::new();
            Ok("Connected".to_string())
        }
        Err(e) => {
            let err_msg = format!("Connection failed: {}", e);
            *status_guard = err_msg.clone();
            Err(err_msg)
        }
    }
}

#[tauri::command]
async fn fetch_device_data(state: tauri::State<'_, AppState>) -> Result<String, String> {
    let active = state.active_port_name.lock().unwrap().clone();
    
    if active.starts_with("WiFi:") {
        let ip = state.target_wifi_ip.lock().unwrap().clone();
        if ip.is_empty() { return Err("No IP".into()); }
        let url = format!("http://{}/update", ip);
        
        let agent = ureq::builder().timeout(Duration::from_secs(2)).build();
        match agent.get(&url).call() {
            Ok(response) => response.into_string().map_err(|e| e.to_string()),
            Err(e) => Err(e.to_string())
        }
    } else if active.starts_with("Serial:") {
        state.latest_config.lock().unwrap().clear();
        state.command_queue.lock().unwrap().push("GET_UPDATE\n".to_string());
        
        for _ in 0..30 {
            let cfg = state.latest_config.lock().unwrap().clone();
            if !cfg.is_empty() { return Ok(cfg); }
            thread::sleep(Duration::from_millis(100));
        }
        Err("Serial timeout".into())
    } else {
        Err("Not connected".into())
    }
}

#[tauri::command]
async fn save_device_settings(state: tauri::State<'_, AppState>, query: String, json_payload: String) -> Result<String, String> {
    let active = state.active_port_name.lock().unwrap().clone();
    
    if active.starts_with("WiFi:") {
        let ip = state.target_wifi_ip.lock().unwrap().clone();
        if ip.is_empty() { return Err("No IP".into()); }
        let url = format!("http://{}/save?{}", ip, query);
        
        let agent = ureq::builder().timeout(Duration::from_secs(12)).build();
        match agent.get(&url).call() {
            Ok(_) => Ok("Success".into()),
            Err(e) => Err(e.to_string())
        }
    } else if active.starts_with("Serial:") {
        state.command_queue.lock().unwrap().push(format!("SAVE_CFG:{}\n", json_payload));
        Ok("Sent via Serial".into())
    } else {
        Err("Not connected".into())
    }
}

fn show_window_safely(window: tauri::WebviewWindow) {
    let _ = window.set_min_size(Some(Size::Logical(LogicalSize { width: 450.0, height: 450.0 })));
    let _ = window.unminimize(); 
    let _ = window.show();
    let _ = window.set_focus();
}

fn main() {
    let app_state = AppState {
        stats: Mutex::new("{}".to_string()),
        port: Mutex::new(None),
        active_port_name: Mutex::new(String::new()),
        manual_disconnect: Mutex::new(false),
        status_msg: Mutex::new("Waiting for connection...".to_string()),
        discovered_wifi: Mutex::new(HashMap::new()),
        target_wifi_ip: Mutex::new(String::new()),
        command_queue: Mutex::new(Vec::new()),
        serial_buffer: Mutex::new(String::new()),
        latest_config: Mutex::new(String::new()),
        user_forced_wifi: Mutex::new(false),
    };

    let my_pc_id = match get_mac_address() {
        Ok(Some(mac)) => {
            let mac_str = mac.to_string().replace(":", "").to_lowercase();
            let pid = std::process::id();
            if mac_str.len() >= 4 { format!("pc-{}:{}", &mac_str[mac_str.len() - 4..], pid) } 
            else { format!("pc-fallback-{}", pid) }
        }
        _ => format!("pc-fallback-{}", std::process::id()), 
    };

    tauri::Builder::default()
        .plugin(tauri_plugin_autostart::init(tauri_plugin_autostart::MacosLauncher::LaunchAgent, Some(vec!["--minimized"])))
        .manage(app_state) 
        .invoke_handler(tauri::generate_handler![
            get_stats, get_ports, toggle_connection, set_autostart, check_autostart,
            fetch_device_data, save_device_settings
        ])
        .setup(move |app| {
            let quit_i = MenuItem::with_id(app, "quit", "Quit", true, None::<&str>)?;
            let show_i = MenuItem::with_id(app, "show", "Show", true, None::<&str>)?;
            let menu = Menu::with_items(app, &[&show_i, &quit_i])?;
            let icon = app.default_window_icon().unwrap().clone();
            
            let _tray = TrayIconBuilder::new().icon(icon).menu(&menu)
                .on_menu_event(|app: &tauri::AppHandle, event| {
                    match event.id().as_ref() {
                        "quit" => app.exit(0),
                        "show" => { if let Some(w) = app.get_webview_window("main") { show_window_safely(w); } }
                        _ => {}
                    }
                })
                .on_tray_icon_event(|tray: &tauri::tray::TrayIcon, event| {
                    if let TrayIconEvent::Click { button: MouseButton::Left, .. } = event {
                        let app = tray.app_handle();
                        if let Some(w) = app.get_webview_window("main") {
                            if w.is_visible().unwrap_or(false) && !w.is_minimized().unwrap_or(false) { 
                                let _ = w.hide(); 
                            } else { 
                                show_window_safely(w); 
                            }
                        }
                    }
                })
                .build(app)?;
            
            let app_handle_mdns = app.handle().clone();
            thread::spawn(move || {
                let mdns = ServiceDaemon::new().expect("Failed to create mDNS daemon");
                let receiver = mdns.browse("_http._tcp.local.").expect("Failed to browse");
                let state = app_handle_mdns.state::<AppState>();

                while let Ok(event) = receiver.recv() {
                    if let ServiceEvent::ServiceResolved(info) = event {
                        let name = info.get_fullname().to_lowercase();
                        if name.contains("tinytosh") {
                            if let Some(ip) = info.get_addresses().iter().next() {
                                let ip_str = ip.to_string();
                                let display_name = name.replace("._http._tcp.local.", ""); 
                                state.discovered_wifi.lock().unwrap().insert(ip_str.clone(), display_name);
                                let mut target = state.target_wifi_ip.lock().unwrap();
                                if target.is_empty() { *target = ip_str; }
                            }
                        }
                    }
                }
            });

            let args: Vec<String> = env::args().collect();
            if !args.contains(&"--minimized".to_string()) {
                if let Some(w) = app.get_webview_window("main") { show_window_safely(w); }
            }

            let app_handle = app.handle().clone();
            let thread_pc_id = my_pc_id.clone();
            
            thread::spawn(move || {
                let state = app_handle.state::<AppState>();
                let mut sys = System::new_all();
                let mut disks = Disks::new_with_refreshed_list();
                let mut networks = Networks::new_with_refreshed_list(); 
                let mut scan_counter = 3;
                let mut wifi_failures = 0; 
                let mut background_scanning = false;
                let mut previous_target = String::new();
                
                let agent = ureq::builder()
                    .timeout_connect(Duration::from_millis(500))
                    .timeout_read(Duration::from_millis(500))
                    .timeout_write(Duration::from_millis(500))
                    .build();

                loop {
                    sys.refresh_cpu_usage(); 
                    sys.refresh_memory();
                    disks.refresh_list(); 
                    networks.refresh(); 

                    let cpu = sys.global_cpu_usage(); 
                    let ram = sys.used_memory() as f64 / sys.total_memory() as f64 * 100.0;
                    
                    let disk_usage = disks.list().iter()
                        .find(|d| d.mount_point().to_str() == Some("/")) 
                        .or_else(|| disks.list().iter().find(|d| d.mount_point().to_str() == Some("C:\\"))) 
                        .map(|d| (d.total_space() - d.available_space()) * 100 / d.total_space())
                        .unwrap_or(0);

                    let total_rx_bytes: u64 = networks.iter().map(|(_, n)| n.received()).sum();
                    let download_kb = total_rx_bytes / 1024; 

                    let data = BridgeStats { 
                        pc_id: thread_pc_id.clone(),
                        cpu_percent: cpu, 
                        net_down_kb: download_kb, 
                        mem_percent: ram, 
                        disk_percent: disk_usage 
                    };
                    
                    let payload = serde_json::to_string(&data).unwrap_or("{}".to_string());
                    if let Ok(mut stats_lock) = state.stats.lock() { *stats_lock = payload.clone(); }

                    let manual_disconnect = *state.manual_disconnect.lock().unwrap();
                    let mut user_forced_wifi = *state.user_forced_wifi.lock().unwrap();
                    let mut is_port_open = state.port.lock().unwrap().is_some();

                    // 1. CONTINUOUS CABLE DETECTION
                    let available = serialport::available_ports().unwrap_or(vec![]);
                    let mut target_port = String::new();
                    
                    for p in &available {
                        let name = p.port_name.to_lowercase();
                        let product = match &p.port_type { 
                            SerialPortType::UsbPort(info) => info.product.clone().unwrap_or_default().to_lowercase(), 
                            _ => String::new() 
                        };
                        
                        if name.contains("usb") || name.contains("acm") || name.contains("serial") || name.contains("jtag") || name.contains("com") ||
                           product.contains("cp210") || product.contains("ch340") || product.contains("esp32") {
                            target_port = p.port_name.clone();
                            break;
                        }
                    }

                    if user_forced_wifi && target_port.is_empty() {
                        *state.user_forced_wifi.lock().unwrap() = false;
                        user_forced_wifi = false; 
                    }

                    // 2. USB SCANNER
                    if !is_port_open && !manual_disconnect && !user_forced_wifi {
                        scan_counter += 1;
                        if scan_counter > 2 { 
                            scan_counter = 0;
                            if !target_port.is_empty() {
                                if let Ok(p) = serialport::new(target_port.clone(), 115200).timeout(Duration::from_millis(100)).open() {
                                    let mut port_guard = state.port.lock().unwrap();
                                    *port_guard = Some(p);
                                    *state.active_port_name.lock().unwrap() = format!("Serial: {}", target_port);
                                    is_port_open = true;
                                }
                            }
                        }
                    }

                    // 3. USB COMMUNICATION
                    let mut sent_via_serial = false;
                    if is_port_open {
                        let mut port_guard = state.port.lock().unwrap();
                        if let Some(port) = port_guard.as_mut() {
                            let mut cmds = state.command_queue.lock().unwrap();
                            let has_cmds = !cmds.is_empty();
                            for cmd in cmds.iter() {
                                for chunk in cmd.as_bytes().chunks(64) {
                                    let _ = port.write(chunk);
                                    thread::sleep(Duration::from_millis(5)); 
                                }
                            }
                            cmds.clear();
                            if has_cmds { thread::sleep(Duration::from_millis(150)); }

                            if port.write(format!("{}\n", payload).as_bytes()).is_ok() {
                                sent_via_serial = true;
                                scan_counter = 0; 
                                wifi_failures = 0; 
                                *state.status_msg.lock().unwrap() = "🔌 Connected via USB".to_string();
                                *state.target_wifi_ip.lock().unwrap() = String::new(); 
                                
                                let mut temp_buf: Vec<u8> = vec![0; 1024];
                                while let Ok(bytes_read) = port.read(&mut temp_buf) {
                                    if bytes_read == 0 { break; }
                                    let incoming = String::from_utf8_lossy(&temp_buf[..bytes_read]);
                                    state.serial_buffer.lock().unwrap().push_str(&incoming);
                                }

                                let mut buf_guard = state.serial_buffer.lock().unwrap();
                                while let Some(pos) = buf_guard.find('\n') {
                                    let line = buf_guard[..pos].trim().to_string();
                                    *buf_guard = buf_guard[pos + 1..].to_string();
                                    if line.starts_with("SYS_UPDATE:") {
                                        *state.latest_config.lock().unwrap() = line.replace("SYS_UPDATE:", "");
                                    }
                                }
                            } else {
                                *port_guard = None;
                                is_port_open = false; 
                                
                                let mut name_guard = state.active_port_name.lock().unwrap();
                                if name_guard.starts_with("Serial:") {
                                    *name_guard = String::new(); 
                                }
                                *state.status_msg.lock().unwrap() = "⏳ Scanning network for devices...".to_string();
                                *state.target_wifi_ip.lock().unwrap() = String::new(); 
                            }
                        }
                    }

                    // 4. WI-FI COMMUNICATION
                    if !sent_via_serial && !manual_disconnect {
                        let mut target_ip = state.target_wifi_ip.lock().unwrap().clone();
                        
                        if target_ip != previous_target {
                            wifi_failures = 0;
                            background_scanning = false;
                            previous_target = target_ip.clone();
                        }
                        
                        if target_ip.is_empty() {
                            let wifi_map = state.discovered_wifi.lock().unwrap();
                            if let Some((ip, _)) = wifi_map.iter().next() {
                                target_ip = ip.clone();
                                *state.target_wifi_ip.lock().unwrap() = target_ip.clone();
                                previous_target = target_ip.clone();
                            }
                        }
                        
                        if !target_ip.is_empty() {
                            let url = format!("http://{}/pc-stats", target_ip); 
                            
                            match agent.post(&url)
                                .set("Content-Type", "application/json")
                                .set("Connection", "close")
                                .send_string(&payload) {
                                Ok(_) => {
                                    wifi_failures = 0; 
                                    background_scanning = false;
                                    let wifi_map = state.discovered_wifi.lock().unwrap();
                                    let pretty_name = wifi_map.get(&target_ip).unwrap_or(&target_ip).clone();
                                    let ui_string = format!("WiFi: {} ({})", pretty_name, target_ip);
                                    
                                    *state.status_msg.lock().unwrap() = "📶 Connected via WiFi".to_string();
                                    *state.active_port_name.lock().unwrap() = ui_string;
                                }
                                Err(ureq::Error::Status(403, _)) => {
                                    *state.status_msg.lock().unwrap() = "❌ Device already paired to another PC".to_string();
                                    *state.active_port_name.lock().unwrap() = String::new();
                                    *state.target_wifi_ip.lock().unwrap() = String::new();
                                    state.discovered_wifi.lock().unwrap().remove(&target_ip);
                                    background_scanning = true; 
                                }
                                Err(_) => {
                                    wifi_failures += 1;
                                    
                                    if !background_scanning && wifi_failures <= 6 { 
                                        *state.status_msg.lock().unwrap() = "⏳ Reconnecting...".to_string();
                                    } else {
                                        background_scanning = true;
                                        *state.status_msg.lock().unwrap() = "⏳ Scanning network for devices...".to_string();
                                        *state.active_port_name.lock().unwrap() = String::new();
                                        *state.user_forced_wifi.lock().unwrap() = false; 
                                        
                                        let wifi_map = state.discovered_wifi.lock().unwrap();
                                        if !wifi_map.is_empty() {
                                            let ips: Vec<String> = wifi_map.keys().cloned().collect();
                                            if let Some(pos) = ips.iter().position(|x| x == &target_ip) {
                                                let next_pos = (pos + 1) % ips.len();
                                                let next_ip = ips[next_pos].clone();
                                                *state.target_wifi_ip.lock().unwrap() = next_ip.clone();
                                                previous_target = next_ip;
                                            } else {
                                                let next_ip = ips[0].clone();
                                                *state.target_wifi_ip.lock().unwrap() = next_ip.clone();
                                                previous_target = next_ip;
                                            }
                                        } else {
                                            *state.target_wifi_ip.lock().unwrap() = String::new();
                                            previous_target = String::new();
                                        }
                                    }
                                }
                            }
                        } else {
                            *state.status_msg.lock().unwrap() = "⏳ Scanning network for devices...".to_string();
                            *state.active_port_name.lock().unwrap() = String::new();
                        }
                    } else if manual_disconnect {
                        *state.status_msg.lock().unwrap() = "⏸️ Disconnected".to_string();
                    }

                    thread::sleep(Duration::from_secs(1));
                }
            });
            Ok(())
        })
        .on_window_event(|window, event| { if let WindowEvent::CloseRequested { api, .. } = event { window.hide().unwrap(); api.prevent_close(); } })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}