#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::thread;
use std::time::Duration;
use std::sync::Mutex; 
use std::env; 
use std::collections::HashMap;
use tauri::menu::{Menu, MenuItem};
use tauri::tray::{TrayIconBuilder, TrayIconEvent, MouseButton, MouseButtonState};
use tauri::{Manager, WindowEvent, Size, LogicalSize}; 
use sysinfo::{System, Disks, Networks}; 
use serialport::{SerialPort, SerialPortType};
use tauri_plugin_autostart::ManagerExt;
use mdns_sd::{ServiceDaemon, ServiceEvent};
use mac_address::get_mac_address;
use any_ascii::any_ascii;
#[cfg(not(target_os = "macos"))]
use futures::StreamExt;
#[cfg(not(target_os = "macos"))]
use nowhear::{MediaEvent, MediaSourceBuilder, MediaSource};

// Main Loop
const LOOP_INTERVAL_MS: u64 = 1000;          // Base speed of the main background loop

// Wi-Fi Telemetry & Connection
const WIFI_THROTTLE_TICKS: i32 = 3;         // 3 ticks * 1000ms = 3 seconds. Prevents overloading the ESP32's sync web server
const MAX_WIFI_FAILURES: i32 = 5;           // Consecutive failed HTTP requests before dropping connection and rescanning
const HTTP_REQUEST_TIMEOUT_MS: u64 = 500;   

// Global Delays & Timeouts
const FETCH_CONFIG_TIMEOUT_SEC: u64 = 2;    
const SAVE_CONFIG_TIMEOUT_SEC: u64 = 15;    

const SERIAL_BAUD_RATE: u32 = 115_200;      
const SERIAL_TIMEOUT_MS: u64 = 100;         
const SERIAL_CHUNK_SIZE: usize = 64;        
const SERIAL_CHUNK_DELAY_MS: u64 = 5;       
const SERIAL_CMD_DELAY_MS: u64 = 150;       
const SERIAL_READ_BUFFER_SIZE: usize = 1024;
const MAX_SERIAL_BUFFER_SIZE: usize = 16384;

const FETCH_CONFIG_MAX_RETRIES: i32 = 30;   
const FETCH_CONFIG_RETRY_DELAY_MS: u64 = 100; 

const WINDOW_MIN_WIDTH: f64 = 450.0;        
const WINDOW_MIN_HEIGHT: f64 = 450.0;       

// Structs

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
    user_selected_port: Mutex<String>, 
    media_info: Mutex<MediaStats>,
    device_logs: Mutex<Vec<String>>,
    log_reading_enabled: Mutex<bool>,
}

#[derive(serde::Serialize, Clone, Default)]
struct MediaStats {
    media_status: String,
    media_name: String,
    media_author: String,
    media_album: String,
}

#[derive(serde::Serialize)]
struct BridgeStats {
    pc_id: String,
    cpu_percent: f32,
    net_down_kb: u64, 
    mem_percent: f64,
    disk_percent: u64,
    #[serde(flatten)]
    media: MediaStats,
}

#[derive(serde::Serialize)]
struct PortStatus {
    ports: Vec<String>,
    connected: Option<String>,
    status_text: String,
    target_ip: String,
}

// Commands

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
fn get_logs(state: tauri::State<AppState>) -> Vec<String> {
    let mut l = state.device_logs.lock().unwrap();
    let cp = l.clone();
    l.clear();
    cp
}

#[tauri::command]
fn toggle_logging(state: tauri::State<AppState>, enable: bool) {
    *state.log_reading_enabled.lock().unwrap() = enable;
    if !enable {
        state.device_logs.lock().unwrap().clear();
    }
}

#[tauri::command]
async fn toggle_connection(state: tauri::State<'_, AppState>, port_name: String) -> Result<String, String> {
    let mut manual_guard = state.manual_disconnect.lock().unwrap();
    let mut user_port_guard = state.user_selected_port.lock().unwrap();

    if port_name.is_empty() {
        *manual_guard = true;
        *user_port_guard = String::new();
        *state.active_port_name.lock().unwrap() = String::new();
        *state.target_wifi_ip.lock().unwrap() = String::new();
        if let Ok(mut port_lock) = state.port.lock() { *port_lock = None; }
        *state.status_msg.lock().unwrap() = "⏸️ Disconnected".to_string();
        Ok("Disconnected".to_string())
    } else {
        *manual_guard = false;
        *user_port_guard = port_name;
        Ok("Connecting...".to_string())
    }
}

#[tauri::command]
async fn fetch_device_data(state: tauri::State<'_, AppState>) -> Result<String, String> {
    let active = state.active_port_name.lock().unwrap().clone();
    
    if active.starts_with("WiFi:") {
        let ip = state.target_wifi_ip.lock().unwrap().clone();
        if ip.is_empty() { return Err("No IP".into()); }
        let url = format!("http://{}/update", ip);
        
        let agent = ureq::builder().timeout(Duration::from_secs(FETCH_CONFIG_TIMEOUT_SEC)).build();
        match agent.get(&url).call() {
            Ok(response) => response.into_string().map_err(|e| e.to_string()),
            Err(e) => Err(e.to_string())
        }
    } else if active.starts_with("Serial:") {
        state.latest_config.lock().unwrap().clear();
        state.command_queue.lock().unwrap().push("GET_UPDATE\n".to_string());
        
        for _ in 0..FETCH_CONFIG_MAX_RETRIES {
            let cfg = state.latest_config.lock().unwrap().clone();
            if !cfg.is_empty() { return Ok(cfg); }
            thread::sleep(Duration::from_millis(FETCH_CONFIG_RETRY_DELAY_MS));
        }
        Err("Serial timeout".into())
    } else {
        Err("Not connected".into())
    }
}

#[tauri::command]
async fn save_device_settings(state: tauri::State<'_, AppState>, json_payload: String) -> Result<String, String> {
    let active = state.active_port_name.lock().unwrap().clone();
    
    if active.starts_with("WiFi:") {
        let ip = state.target_wifi_ip.lock().unwrap().clone();
        if ip.is_empty() { return Err("No IP".into()); }
        
        let url = format!("http://{}/save", ip); 
        
        let agent = ureq::builder().timeout(Duration::from_secs(SAVE_CONFIG_TIMEOUT_SEC)).build();
        
        match agent.post(&url).set("Content-Type", "application/json").send_string(&json_payload) {
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

fn show_window_safely(app_handle: &tauri::AppHandle, window: tauri::WebviewWindow) {
    #[cfg(target_os = "macos")]
    let _ = app_handle.set_activation_policy(tauri::ActivationPolicy::Regular);

    let _ = window.set_min_size(Some(Size::Logical(LogicalSize { width: WINDOW_MIN_WIDTH, height: WINDOW_MIN_HEIGHT })));
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
        user_selected_port: Mutex::new(String::new()),
        media_info: Mutex::new(MediaStats::default()),
        device_logs: Mutex::new(Vec::new()),
        log_reading_enabled: Mutex::new(false),
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
            fetch_device_data, save_device_settings, get_logs, toggle_logging
        ])
        .setup(move |app| {
            let args: Vec<String> = env::args().collect();
            let is_minimized = args.contains(&"--minimized".to_string());

            #[cfg(target_os = "macos")]
            if is_minimized {
                let _ = app.set_activation_policy(tauri::ActivationPolicy::Accessory);
            } else {
                let _ = app.set_activation_policy(tauri::ActivationPolicy::Regular);
            }

            let quit_i = MenuItem::with_id(app, "quit", "Quit", true, None::<&str>)?;
            let show_i = MenuItem::with_id(app, "show", "Show", true, None::<&str>)?;
            let menu = Menu::with_items(app, &[&show_i, &quit_i])?;
            let icon = app.default_window_icon().unwrap().clone();
            
            let _tray = TrayIconBuilder::new().icon(icon).menu(&menu)
                .on_menu_event(|app: &tauri::AppHandle, event| {
                    match event.id().as_ref() {
                        "quit" => app.exit(0),
                        "show" => { if let Some(w) = app.get_webview_window("main") { show_window_safely(app, w); } }
                        _ => {}
                    }
                })
                .on_tray_icon_event(|tray: &tauri::tray::TrayIcon, event| {
                    if let TrayIconEvent::Click { button: MouseButton::Left, button_state: MouseButtonState::Up, .. } = event {
                        let app = tray.app_handle();
                        if let Some(w) = app.get_webview_window("main") {
                            
                            let is_visible = w.is_visible().unwrap_or(false);
                            let is_minimized = w.is_minimized().unwrap_or(false);
                            let is_focused = w.is_focused().unwrap_or(false);

                            if is_visible && !is_minimized && is_focused { 
                                let _ = w.hide(); 
                                
                                #[cfg(target_os = "macos")]
                                let _ = app.set_activation_policy(tauri::ActivationPolicy::Accessory);
                            } else { 
                                show_window_safely(app, w); 
                            }
                        }
                    }
                })
                .build(app)?;
            
            let app_handle_media = app.handle().clone();

            #[cfg(not(target_os = "macos"))]
            tauri::async_runtime::spawn(async move {
                match MediaSourceBuilder::new().build().await {
                    Ok(source) => {                        
                        let mut current_player: Option<String> = None;

                        if let Ok(players) = source.list_players().await {
                            for player_name in players {
                                if let Ok(info) = source.get_player(&player_name).await {
                                    let title = info.current_track.as_ref().map(|t| t.title.trim().to_string()).unwrap_or_default();
                                    let has_title = !title.is_empty() && title.to_lowercase() != "unknown";
                                    if !has_title { continue; }
                                    
                                    let status = format!("{:?}", info.playback_state).to_lowercase();
                                    if status == "playing" || status == "paused" {
                                        current_player = Some(player_name.clone());
                                        let state = app_handle_media.state::<AppState>();
                                        let mut m = state.media_info.lock().unwrap();
                                        m.media_status = status.clone();
                                        if let Some(track) = info.current_track {
                                            m.media_name = any_ascii(&track.title);
                                            m.media_author = any_ascii(&track.artist.join(", "));
                                            m.media_album = any_ascii(&track.album.unwrap_or_default());
                                        }
                                        if status == "playing" { break; }
                                    }
                                }
                            }
                        }

                        if let Ok(mut stream) = source.event_stream().await {
                            while let Some(event) = stream.next().await {
                                if matches!(event, MediaEvent::PositionChanged { .. }) { continue; }

                                match event {
                                    MediaEvent::TrackChanged { player_name, .. } | MediaEvent::StateChanged { player_name, .. } => {
                                        if let Ok(info) = source.get_player(&player_name).await {
                                            let status = format!("{:?}", info.playback_state).to_lowercase();
                                            let title = info.current_track.as_ref().map(|t| t.title.trim().to_string()).unwrap_or_default();
                                            let has_title = !title.is_empty() && title.to_lowercase() != "unknown";

                                            if status == "stopped" || (!has_title && current_player.as_ref() == Some(&player_name)) {
                                                if current_player.as_ref() == Some(&player_name) {
                                                    current_player = None;
                                                    let state = app_handle_media.state::<AppState>();
                                                    let mut m = state.media_info.lock().unwrap();
                                                    m.media_status = "stopped".to_string();
                                                    m.media_name = String::new();
                                                    m.media_author = String::new();
                                                    m.media_album = String::new();
                                                }
                                            } else if has_title {
                                                let is_stealing_focus = status == "playing";
                                                let is_current = current_player.as_ref() == Some(&player_name);

                                                if is_stealing_focus || is_current {
                                                    current_player = Some(player_name.clone());
                                                    let state = app_handle_media.state::<AppState>();
                                                    let mut m = state.media_info.lock().unwrap();
                                                    m.media_status = status.clone();
                                                    if let Some(track) = info.current_track {
                                                        m.media_name = any_ascii(&track.title);
                                                        m.media_author = any_ascii(&track.artist.join(", "));
                                                        m.media_album = any_ascii(&track.album.unwrap_or_default());
                                                    }
                                                }
                                            }
                                        }
                                    },
                                    MediaEvent::PlayerRemoved { player_name } => {
                                        if current_player.as_ref() == Some(&player_name) {
                                            current_player = None;
                                            let state = app_handle_media.state::<AppState>();
                                            let mut m = state.media_info.lock().unwrap();
                                            m.media_status = "stopped".to_string();
                                            m.media_name = String::new();
                                            m.media_author = String::new();
                                            m.media_album = String::new();
                                        }
                                    },
                                    _ => {}
                                }

                                if current_player.is_none() {
                                    if let Ok(players) = source.list_players().await {
                                        for p_name in players {
                                            if let Ok(info) = source.get_player(&p_name).await {
                                                let status = format!("{:?}", info.playback_state).to_lowercase();
                                                let title = info.current_track.as_ref().map(|t| t.title.trim().to_string()).unwrap_or_default();
                                                let has_title = !title.is_empty() && title.to_lowercase() != "unknown";
                                                
                                                if has_title && (status == "playing" || status == "paused") {
                                                    current_player = Some(p_name.clone());
                                                    let state = app_handle_media.state::<AppState>();
                                                    let mut m = state.media_info.lock().unwrap();
                                                    m.media_status = status;
                                                    if let Some(track) = info.current_track {
                                                        m.media_name = any_ascii(&track.title);
                                                        m.media_author = any_ascii(&track.artist.join(", "));
                                                        m.media_album = any_ascii(&track.album.unwrap_or_default());
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Err(_) => {}
                }
            });

            #[cfg(target_os = "macos")]
            thread::spawn(move || {
                let state = app_handle_media.state::<AppState>();
                
                loop {
                    let mut m = state.media_info.lock().unwrap();
                    
                    if let Some(info) = mediaremote_rs::get_now_playing() {
                        let title = info.title.trim().to_string();
                        let has_title = !title.is_empty() && title.to_lowercase() != "unknown";
                        
                        if !has_title {
                            m.media_status = "stopped".to_string();
                            m.media_name = String::new();
                            m.media_author = String::new();
                            m.media_album = String::new();
                        } else {
                            m.media_status = if info.playing { "playing".to_string() } else { "paused".to_string() };
                            m.media_name = any_ascii(&title);
                            m.media_author = any_ascii(&info.artist.unwrap_or_default());
                            m.media_album = any_ascii(&info.album.unwrap_or_default());
                        }
                    } else {
                        m.media_status = "stopped".to_string();
                        m.media_name = String::new();
                        m.media_author = String::new();
                        m.media_album = String::new();
                    }
                    
                    drop(m);
                    thread::sleep(Duration::from_millis(1000));
                }
            });

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
                if let Some(w) = app.get_webview_window("main") { 
                    show_window_safely(&app.handle(), w); 
                }
            }

            let app_handle = app.handle().clone();
            let thread_pc_id = my_pc_id.clone();
            
            thread::spawn(move || {
                let state = app_handle.state::<AppState>();
                let mut sys = System::new_all();
                let mut disks = Disks::new_with_refreshed_list();
                let mut networks = Networks::new_with_refreshed_list(); 
                
                let agent = ureq::builder()
                    .timeout_connect(Duration::from_millis(HTTP_REQUEST_TIMEOUT_MS))
                    .timeout_read(Duration::from_millis(HTTP_REQUEST_TIMEOUT_MS))
                    .timeout_write(Duration::from_millis(HTTP_REQUEST_TIMEOUT_MS))
                    .build();

                #[derive(PartialEq)]
                enum AppPhase { Idle, InitScan, TestUsb(String), TestWifi(String), ConnectedUsb, ConnectedWifi }
                
                let mut phase = AppPhase::InitScan;
                let mut usb_candidates: Vec<String> = Vec::new();
                let mut wifi_candidates: Vec<String> = Vec::new();
                let mut candidate_idx = 0;
                
                let mut test_ticks = 0;
                let mut wifi_failures = 0;

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

                    let media_data = state.media_info.lock().unwrap().clone();

                    let data = BridgeStats { 
                        pc_id: thread_pc_id.clone(),
                        cpu_percent: cpu, 
                        net_down_kb: download_kb, 
                        mem_percent: ram, 
                        disk_percent: disk_usage,
                        media: media_data
                    };
                    
                    let payload = serde_json::to_string(&data).unwrap_or("{}".to_string());
                    if let Ok(mut stats_lock) = state.stats.lock() { *stats_lock = payload.clone(); }

                    // 1. Force disconnect
                    let manual_disconnect = *state.manual_disconnect.lock().unwrap();

                    if manual_disconnect {
                        if state.port.lock().unwrap().is_some() { *state.port.lock().unwrap() = None; }
                        *state.active_port_name.lock().unwrap() = String::new();
                        *state.target_wifi_ip.lock().unwrap() = String::new();
                        *state.status_msg.lock().unwrap() = "⏸️ Disconnected".to_string();
                        phase = AppPhase::Idle;
                        thread::sleep(Duration::from_millis(LOOP_INTERVAL_MS));
                        continue;
                    }

                    if phase == AppPhase::Idle {
                        phase = AppPhase::InitScan;
                    }

                    // 2. Catch manual user overrides
                    let mut user_req = state.user_selected_port.lock().unwrap();
                    let req_port = user_req.clone();
                    *user_req = String::new();

                    if !req_port.is_empty() {
                        if state.port.lock().unwrap().is_some() { *state.port.lock().unwrap() = None; }
                        *state.active_port_name.lock().unwrap() = String::new();
                        *state.target_wifi_ip.lock().unwrap() = String::new();
                        
                        usb_candidates.clear();
                        wifi_candidates.clear();
                        candidate_idx = 0;
                        test_ticks = 0;
                        wifi_failures = 0;

                        if req_port.starts_with("WiFi:") {
                            let ip = if let Some(start) = req_port.rfind('(') {
                                if let Some(end) = req_port.rfind(')') {
                                    req_port[start + 1..end].to_string()
                                } else { String::new() }
                            } else { String::new() };
                            if !ip.is_empty() { 
                                phase = AppPhase::TestWifi(ip); 
                            }
                        } else {
                            let port = req_port.replace("Serial: ", "");
                            phase = AppPhase::TestUsb(port);
                        }
                        continue; 
                    }

                    // 3. Execute state machine
                    match &phase {
                        AppPhase::Idle => {} 
                        
                        AppPhase::InitScan => {
                            *state.status_msg.lock().unwrap() = "🔍 Scanning for devices...".to_string();
                            *state.active_port_name.lock().unwrap() = String::new();
                            *state.target_wifi_ip.lock().unwrap() = String::new();
                            if state.port.lock().unwrap().is_some() { *state.port.lock().unwrap() = None; }

                            usb_candidates.clear();
                            let available = serialport::available_ports().unwrap_or(vec![]);
                            for p in &available {
                                let name = p.port_name.to_lowercase();
                                if name.contains("bluetooth") || name.contains("bt-") || name.contains("incoming") || name.contains("wlan") { continue; }
                                let product = match &p.port_type { SerialPortType::UsbPort(info) => info.product.clone().unwrap_or_default().to_lowercase(), _ => String::new() };
                                if name.contains("usb") || name.contains("acm") || name.contains("serial") || name.contains("jtag") || name.starts_with("com") ||
                                   product.contains("cp210") || product.contains("ch340") || product.contains("ch910") || product.contains("esp") {
                                    usb_candidates.push(p.port_name.clone());
                                }
                            }

                            wifi_candidates.clear();
                            let wifi_map = state.discovered_wifi.lock().unwrap();
                            for ip in wifi_map.keys() { 
                                wifi_candidates.push(ip.clone()); 
                            }

                            candidate_idx = 0;
                            if !usb_candidates.is_empty() {
                                phase = AppPhase::TestUsb(usb_candidates[0].clone());
                            } else if !wifi_candidates.is_empty() {
                                phase = AppPhase::TestWifi(wifi_candidates[0].clone());
                            }
                            test_ticks = 0;
                            wifi_failures = 0;
                        }

                        AppPhase::TestUsb(port_name) => {
                            if test_ticks == 0 {
                                *state.status_msg.lock().unwrap() = format!("⚪ Connecting to USB ({})...", port_name);
                                if let Ok(mut p) = serialport::new(port_name.clone(), SERIAL_BAUD_RATE).timeout(Duration::from_millis(SERIAL_TIMEOUT_MS)).open() {
                                    let _ = p.write(b"GET_UPDATE\n");
                                    *state.port.lock().unwrap() = Some(p);
                                    state.serial_buffer.lock().unwrap().clear();
                                    state.latest_config.lock().unwrap().clear();
                                } else {
                                    test_ticks = 99; 
                                }
                            }

                            test_ticks += 1;
                            let mut handshake_success = false;

                            if let Some(port) = state.port.lock().unwrap().as_mut() {
                                let mut temp_buf: Vec<u8> = vec![0; SERIAL_READ_BUFFER_SIZE];
                                while let Ok(bytes_read) = port.read(&mut temp_buf) {
                                    if bytes_read == 0 { break; }
                                    let incoming = String::from_utf8_lossy(&temp_buf[..bytes_read]);
                                    state.serial_buffer.lock().unwrap().push_str(&incoming);
                                }
                                let log_enabled = *state.log_reading_enabled.lock().unwrap();
                                let mut buf = state.serial_buffer.lock().unwrap();
                                while let Some(newline_offset) = buf.find('\n') {
                                    let line = buf[..newline_offset].trim().to_string();
                                    if line.starts_with("SYS_UPDATE:") {
                                        *state.latest_config.lock().unwrap() = line[11..].to_string();
                                        handshake_success = true; 
                                    } else if !line.is_empty() && log_enabled {
                                        state.device_logs.lock().unwrap().push(line);
                                    }
                                    *buf = buf[newline_offset + 1..].to_string();
                                }
                                if buf.len() > MAX_SERIAL_BUFFER_SIZE { buf.clear(); }
                            }

                            if handshake_success {
                                *state.status_msg.lock().unwrap() = "🔌 Connected via USB".to_string();
                                *state.active_port_name.lock().unwrap() = format!("Serial: {}", port_name); 
                                test_ticks = 0; // Reset tick counter for pushing payload
                                phase = AppPhase::ConnectedUsb;
                            } else if test_ticks > 4 { 
                                *state.port.lock().unwrap() = None;
                                candidate_idx += 1;
                                
                                if candidate_idx < usb_candidates.len() {
                                    phase = AppPhase::TestUsb(usb_candidates[candidate_idx].clone());
                                    test_ticks = 0;
                                } else {
                                    candidate_idx = 0;

                                    wifi_candidates.clear();
                                    for ip in state.discovered_wifi.lock().unwrap().keys() { 
                                        wifi_candidates.push(ip.clone()); 
                                    }

                                    if !wifi_candidates.is_empty() {
                                        phase = AppPhase::TestWifi(wifi_candidates[0].clone());
                                        test_ticks = 0;
                                    } else {
                                        phase = AppPhase::InitScan;
                                    }
                                }
                            }
                        }

                        AppPhase::TestWifi(ip) => {
                            *state.status_msg.lock().unwrap() = format!("⚪ Connecting to Wi-Fi ({})...", ip);
                            let url = format!("http://{}/pc-stats", ip);
                            
                            match agent.post(&url).set("Content-Type", "application/json").set("Connection", "close").send_string(&payload) {
                                Ok(_) => {
                                    let wifi_map = state.discovered_wifi.lock().unwrap();
                                    let pretty_name = wifi_map.get(ip).unwrap_or(ip).clone();
                                    *state.active_port_name.lock().unwrap() = format!("WiFi: {} ({})", pretty_name, ip);
                                    *state.target_wifi_ip.lock().unwrap() = ip.clone();
                                    *state.status_msg.lock().unwrap() = "📶 Connected via WiFi".to_string();
                                    wifi_failures = 0;
                                    test_ticks = 0; // Reset tick counter for pushing payload
                                    phase = AppPhase::ConnectedWifi;
                                }
                                Err(ureq::Error::Status(403, _)) => {
                                    *state.status_msg.lock().unwrap() = "❌ Device already paired".to_string();
                                    state.discovered_wifi.lock().unwrap().remove(ip);
                                    test_ticks = 99; 
                                }
                                Err(_) => { 
                                    test_ticks = 99; 
                                } 
                            }

                            if phase != AppPhase::ConnectedWifi {
                                candidate_idx += 1;
                                if candidate_idx < wifi_candidates.len() {
                                    phase = AppPhase::TestWifi(wifi_candidates[candidate_idx].clone());
                                    test_ticks = 0;
                                } else {
                                    phase = AppPhase::InitScan;
                                }
                            }
                        }

                        AppPhase::ConnectedUsb => {
                            let mut connection_lost = false;
                            let mut port_guard = state.port.lock().unwrap();
                            
                            if let Some(port) = port_guard.as_mut() {
                                
                                // 1. Always process and send manual queue commands immediately 
                                let mut cmds = state.command_queue.lock().unwrap();
                                let has_cmds = !cmds.is_empty();
                                for cmd in cmds.iter() {
                                    for chunk in cmd.as_bytes().chunks(SERIAL_CHUNK_SIZE) { let _ = port.write(chunk); thread::sleep(Duration::from_millis(SERIAL_CHUNK_DELAY_MS)); }
                                }
                                cmds.clear();
                                if has_cmds { thread::sleep(Duration::from_millis(SERIAL_CMD_DELAY_MS)); }

                                // 2. Continuous data push
                                if port.write(format!("{}\n", payload).as_bytes()).is_err() {
                                    connection_lost = true;
                                }

                                // 3. Always attempt to read incoming messages
                                if !connection_lost {
                                    let mut temp_buf: Vec<u8> = vec![0; SERIAL_READ_BUFFER_SIZE];
                                    while let Ok(bytes_read) = port.read(&mut temp_buf) {
                                        if bytes_read == 0 { break; }
                                        let incoming = String::from_utf8_lossy(&temp_buf[..bytes_read]);
                                        state.serial_buffer.lock().unwrap().push_str(&incoming);
                                    }
                                    let log_enabled = *state.log_reading_enabled.lock().unwrap();
                                    let mut buf = state.serial_buffer.lock().unwrap();
                                    while let Some(newline_offset) = buf.find('\n') {
                                        let line = buf[..newline_offset].trim().to_string();
                                        if line.starts_with("SYS_UPDATE:") {
                                            *state.latest_config.lock().unwrap() = line[11..].to_string();
                                        } else if !line.is_empty() && log_enabled {
                                            state.device_logs.lock().unwrap().push(line);
                                        }
                                        *buf = buf[newline_offset + 1..].to_string();
                                    }
                                    if buf.len() > MAX_SERIAL_BUFFER_SIZE { buf.clear(); }
                                }

                            } else {
                                connection_lost = true;
                            }

                            if connection_lost {
                                *port_guard = None;
                                *state.active_port_name.lock().unwrap() = String::new();
                                phase = AppPhase::InitScan; 
                            }
                        }

                        AppPhase::ConnectedWifi => {
                            if test_ticks >= WIFI_THROTTLE_TICKS {
                                let ip = state.target_wifi_ip.lock().unwrap().clone();
                                let url = format!("http://{}/pc-stats", ip);
                                
                                match agent.post(&url).set("Content-Type", "application/json").set("Connection", "close").send_string(&payload) {
                                    Ok(_) => { 
                                        wifi_failures = 0; 
                                        *state.status_msg.lock().unwrap() = "📶 Connected via WiFi".to_string(); 
                                    }
                                    Err(_) => {
                                        wifi_failures += 1;
                                        if wifi_failures >= MAX_WIFI_FAILURES {
                                            *state.status_msg.lock().unwrap() = "⏳ Connection lost, scanning...".to_string();
                                            *state.target_wifi_ip.lock().unwrap() = String::new();
                                            *state.active_port_name.lock().unwrap() = String::new();
                                            phase = AppPhase::InitScan; 
                                        } else {
                                            *state.status_msg.lock().unwrap() = format!("⏳ Reconnecting ({} / {})...", wifi_failures, MAX_WIFI_FAILURES);
                                        }
                                    }
                                }
                                test_ticks = 0;
                            } else {
                                test_ticks += 1;
                            }
                        }
                    }
                    thread::sleep(Duration::from_millis(LOOP_INTERVAL_MS));
                }
            });
            Ok(())
        })
        .on_window_event(|window, event| { 
            if let WindowEvent::CloseRequested { api, .. } = event { 
                window.hide().unwrap(); 
                api.prevent_close(); 
                #[cfg(target_os = "macos")]
                let _ = window.app_handle().set_activation_policy(tauri::ActivationPolicy::Accessory);
            } 
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}