#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::thread;
use std::time::Duration;
use std::sync::Mutex; 
use std::env; 
use tauri::menu::{Menu, MenuItem};
use tauri::tray::{TrayIconBuilder, TrayIconEvent, MouseButton};
use tauri::{Manager, WindowEvent, Size, LogicalSize}; 
use sysinfo::{System, Disks, Networks}; 
use serialport::{SerialPort, SerialPortType};
use tauri_plugin_autostart::ManagerExt;

struct AppState {
    stats: Mutex<String>,
    port: Mutex<Option<Box<dyn SerialPort>>>,
    active_port_name: Mutex<String>,
    manual_disconnect: Mutex<bool>,
    status_msg: Mutex<String>, 
}

#[derive(serde::Serialize)]
struct BridgeStats {
    cpu_percent: f32,
    net_down_kb: u64, 
    mem_percent: f64,
    disk_percent: u64,
}

#[derive(serde::Serialize)]
struct PortStatus {
    ports: Vec<String>,
    connected: Option<String>,
    status_text: String 
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
    let ports = serialport::available_ports()
        .map(|p| p.into_iter().map(|x| x.port_name).collect())
        .unwrap_or(vec![]);
    
    let active = state.active_port_name.lock().unwrap().clone();
    let connected = if active.is_empty() { None } else { Some(active) };
    let status_text = state.status_msg.lock().unwrap().clone();

    PortStatus { ports, connected, status_text }
}

#[tauri::command]
fn toggle_connection(state: tauri::State<AppState>, port_name: String, connect: bool) -> Result<String, String> {
    let mut port_guard = state.port.lock().unwrap();
    let mut name_guard = state.active_port_name.lock().unwrap();
    let mut manual_guard = state.manual_disconnect.lock().unwrap();
    let mut status_guard = state.status_msg.lock().unwrap();
    
    if !connect {
        *port_guard = None;
        *name_guard = String::new();
        *manual_guard = true;
        *status_guard = "Disconnected".to_string(); 
        return Ok("Disconnected".to_string());
    }

    match serialport::new(port_name.clone(), 115200)
        .timeout(Duration::from_millis(100))
        .open() 
    {
        Ok(p) => {
            *port_guard = Some(p);
            *name_guard = port_name;
            *manual_guard = false;
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

fn show_window_safely(window: tauri::WebviewWindow) {
    let _ = window.set_min_size(Some(Size::Logical(LogicalSize { width: 300.0, height: 400.0 })));
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
    };

    tauri::Builder::default()
        .plugin(tauri_plugin_autostart::init(tauri_plugin_autostart::MacosLauncher::LaunchAgent, Some(vec!["--minimized"])))
        .manage(app_state) 
        .invoke_handler(tauri::generate_handler![get_stats, get_ports, toggle_connection, set_autostart, check_autostart])
        .setup(|app| {
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

            let args: Vec<String> = env::args().collect();
            if !args.contains(&"--minimized".to_string()) {
                if let Some(w) = app.get_webview_window("main") { show_window_safely(w); }
            }

            let app_handle = app.handle().clone();
            thread::spawn(move || {
                let state = app_handle.state::<AppState>();
                let mut sys = System::new_all();
                let mut disks = Disks::new_with_refreshed_list();
                let mut networks = Networks::new_with_refreshed_list(); 
                let mut scan_counter = 0;

                loop {
                    // 1. REFRESH DATA
                    sys.refresh_cpu_usage(); 
                    sys.refresh_memory();
                    disks.refresh_list(); 
                    networks.refresh(); 

                    // 2. CALCULATE METRICS
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
                        cpu_percent: cpu, 
                        net_down_kb: download_kb, 
                        mem_percent: ram, 
                        disk_percent: disk_usage 
                    };
                    
                    let payload = serde_json::to_string(&data).unwrap_or("{}".to_string());
                    if let Ok(mut stats_lock) = state.stats.lock() { *stats_lock = payload.clone(); }

                    // 3. SEND TO ESP32
                    let mut needs_scan = false;
                    {
                        let mut port_guard = state.port.lock().unwrap();
                        if let Some(port) = port_guard.as_mut() {
                            if port.write(format!("{}\n", payload).as_bytes()).is_ok() {
                                scan_counter = 0; 
                            } else {
                                *port_guard = None;
                                *state.active_port_name.lock().unwrap() = String::new();
                                *state.status_msg.lock().unwrap() = String::new(); 
                                needs_scan = true;
                            }
                        } else {
                            if !*state.manual_disconnect.lock().unwrap() { needs_scan = true; }
                        }
                    }

                    // 4. AUTO-SCAN FOR DEVICE
                    if needs_scan {
                         scan_counter += 1;
                         if scan_counter > 2 {
                            scan_counter = 0;
                            let available = serialport::available_ports().unwrap_or(vec![]);
                            let mut target_port = String::new();
                            for p in available {
                                let name = p.port_name.to_lowercase();
                                let product = match p.port_type { 
                                    SerialPortType::UsbPort(info) => info.product.unwrap_or_default().to_lowercase(), 
                                    _ => String::new() 
                                };
                                
                                if name.contains("usb") || name.contains("acm") || name.contains("serial") || name.contains("jtag") || name.contains("com") ||
                                   product.contains("cp210") || product.contains("ch340") || product.contains("esp32") || product.contains("serial") || product.contains("jtag") {
                                    target_port = p.port_name;
                                    break;
                                }
                            }
                            if !target_port.is_empty() {
                                if let Ok(p) = serialport::new(target_port.clone(), 115200).timeout(Duration::from_millis(100)).open() {
                                    let mut port_guard = state.port.lock().unwrap();
                                    *port_guard = Some(p);
                                    *state.active_port_name.lock().unwrap() = target_port;
                                    *state.manual_disconnect.lock().unwrap() = false;
                                    *state.status_msg.lock().unwrap() = String::new(); 
                                }
                            }
                        }
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