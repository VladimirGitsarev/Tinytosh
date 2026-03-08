const { invoke } = window.__TAURI__.core;

let isConnected = false;
let isConfigLoaded = false;
let formDirty = false;
let currentDeviceId = "";
let currentDeviceIp = "";

const topStocks = [
  ["S&P 500 ETF", "SPY"], ["Invesco QQQ (Tech)", "QQQ"], ["Dow Jones ETF", "DIA"],
  ["Vanguard Total Stock", "VTI"], ["Vanguard S&P 500", "VOO"], ["Semiconductor ETF", "SMH"],
  ["Financial Select", "XLF"], ["Health Care Select", "XLV"], ["Energy Select", "XLE"],
  ["Apple Inc.", "AAPL"], ["Microsoft Corp.", "MSFT"], ["NVIDIA Corp.", "NVDA"],
  ["Alphabet Inc.", "GOOG"], ["Amazon.com Inc.", "AMZN"], ["Meta Platforms", "META"],
  ["Tesla Inc.", "TSLA"], ["Taiwan Semiconductor", "TSM"], ["Broadcom Inc.", "AVGO"],
  ["ASML Holding", "ASML"], ["Intel Corp.", "INTC"], ["Qualcomm Inc.", "QCOM"],
  ["Texas Instruments", "TXN"], ["Micron Technology", "MU"], ["ARM Holdings", "ARM"],
  ["Salesforce Inc.", "CRM"], ["Adobe Inc.", "ADBE"], ["ServiceNow", "NOW"],
  ["Snowflake Inc.", "SNOW"], ["CrowdStrike", "CRWD"], ["Palo Alto Networks", "PANW"],
  ["Fortinet", "FTNT"], ["Palantir Tech", "PLTR"], ["Datadog Inc.", "DDOG"],
  ["JPMorgan Chase", "JPM"], ["Visa Inc.", "V"], ["Mastercard Inc.", "MA"],
  ["Bank of America", "BAC"], ["Berkshire Hathaway", "BRK.B"], ["Wells Fargo", "WFC"],
  ["Goldman Sachs", "GS"], ["Morgan Stanley", "MS"], ["American Express", "AXP"],
  ["PayPal Holdings", "PYPL"], ["Block Inc. (Square)", "SQ"], ["Coinbase Global", "COIN"],
  ["MicroStrategy", "MSTR"], ["Walmart Inc.", "WMT"], ["Costco Wholesale", "COST"],
  ["The Home Depot", "HD"], ["Lowe's Companies", "LOW"], ["Target Corp.", "TGT"],
  ["McDonald's Corp.", "MCD"], ["Starbucks Corp.", "SBUX"], ["Nike Inc.", "NKE"],
  ["Lululemon", "LULU"], ["Procter & Gamble", "PG"], ["The Coca-Cola Co.", "KO"],
  ["PepsiCo Inc.", "PEP"], ["Eli Lilly and Co.", "LLY"], ["UnitedHealth Group", "UNH"],
  ["Johnson & Johnson", "JNJ"], ["AbbVie Inc.", "ABBV"], ["Merck & Co.", "MRK"],
  ["Pfizer Inc.", "PFE"], ["Novo Nordisk (ADR)", "NVO"], ["Thermo Fisher", "TMO"],
  ["Intuitive Surgical", "ISRG"], ["Exxon Mobil", "XOM"], ["Chevron Corp.", "CVX"],
  ["Caterpillar Inc.", "CAT"], ["General Electric", "GE"], ["Honeywell Intl", "HON"],
  ["The Boeing Company", "BA"], ["Union Pacific", "UNP"], ["Lockheed Martin", "LMT"],
  ["RTX Corporation", "RTX"], ["The Walt Disney Co.", "DIS"], ["Netflix Inc.", "NFLX"],
  ["Comcast Corp.", "CMCSA"], ["Spotify Technology", "SPOT"], ["AT&T Inc.", "T"],
  ["Verizon Comm.", "VZ"], ["T-Mobile US", "TMUS"], ["Alibaba Group", "BABA"],
  ["Sony Group Corp.", "SONY"], ["Shopify Inc.", "SHOP"], ["MercadoLibre", "MELI"],
  ["Toyota Motor Corp.", "TM"], ["Ferrari N.V.", "RACE"], ["Uber Technologies", "UBER"],
  ["Airbnb Inc.", "ABNB"]
];

const topCoins = [
  [90, "BTC"], [80, "ETH"], [518, "USDT"], [2710, "BNB"], [48543, "SOL"],
  [58, "XRP"], [33224, "USDC"], [257, "ADA"], [44857, "AVAX"], [2, "DOGE"],
  [45131, "DOT"], [2713, "TRX"], [2738, "LINK"], [33536, "MATIC"], [51334, "TON"],
  [44800, "SHIB"], [1, "LTC"], [2321, "BCH"], [33234, "WBTC"], [44265, "UNI"],
  [28557, "ATOM"], [47305, "NEAR"], [47214, "ICP"], [51469, "APT"], [51811, "PEPE"],
  [172, "XLM"], [29854, "OKB"], [118, "ETC"], [28, "XMR"], [32703, "LEO"],
  [45219, "FIL"], [33503, "HBAR"], [51745, "ARB"], [2741, "VET"], [2816, "MKR"],
  [42564, "CRO"], [33022, "QNT"], [33177, "ALGO"], [46427, "GRT"], [45088, "AAVE"],
  [44926, "STX"], [28014, "SNX"], [2679, "EOS"], [46087, "EGLD"], [45224, "SAND"],
  [28318, "THETA"], [2748, "MANA"], [2742, "XTZ"], [46990, "MINA"], [33309, "FTM"],
  [44365, "KAVA"], [1376, "NEO"], [46481, "FLOW"], [32785, "CHZ"], [44256, "KLAY"],
  [32729, "RPL"], [45435, "CRV"], [46682, "GALA"], [44866, "COMP"], [2770, "IOTA"],
  [33285, "DAI"], [33814, "PAXG"], [32684, "BUSD"], [33282, "TUSD"], [45204, "FRAX"],
  [44082, "USDP"], [33263, "ENJ"], [33190, "BAT"], [2734, "ZEC"], [2740, "DASH"],
  [46580, "LDO"], [51717, "OP"], [51859, "SUI"], [51608, "BLUR"], [51381, "GMX"]
];

const allCurrencies = [
  ["aed", "United Arab Emirates Dirham"], ["afn", "Afghan Afghani"], ["all", "Albanian Lek"],
  ["amd", "Armenian Dram"], ["ang", "Netherlands Antillean Guilder"], ["aoa", "Angolan Kwanza"],
  ["ars", "Argentine Peso"], ["aud", "Australian Dollar"], ["awg", "Aruban Florin"],
  ["azn", "Azerbaijani Manat"], ["bam", "Bosnia-Herzegovina Convertible Mark"], ["bbd", "Barbadian Dollar"],
  ["bdt", "Bangladeshi Taka"], ["bgn", "Bulgarian Lev"], ["bhd", "Bahraini Dinar"],
  ["bif", "Burundian Franc"], ["bmd", "Bermudan Dollar"], ["bnd", "Brunei Dollar"],
  ["bob", "Bolivian Boliviano"], ["brl", "Brazilian Real"], ["bsd", "Bahamian Dollar"],
  ["btn", "Bhutanese Ngultrum"], ["bwp", "Botswanan Pula"], ["byn", "New Belarusian Ruble"],
  ["bzd", "Belize Dollar"], ["cad", "Canadian Dollar"], ["cdf", "Congolese Franc"],
  ["chf", "Swiss Franc"], ["clp", "Chilean Peso"], ["cny", "Chinese Yuan"],
  ["cop", "Colombian Peso"], ["crc", "Costa Rican Colón"], ["cup", "Cuban Peso"],
  ["cve", "Cape Verdean Escudo"], ["czk", "Czech Republic Koruna"], ["djf", "Djiboutian Franc"],
  ["dkk", "Danish Krone"], ["dop", "Dominican Peso"], ["dzd", "Algerian Dinar"],
  ["egp", "Egyptian Pound"], ["ern", "Eritrean Nakfa"], ["etb", "Ethiopian Birr"],
  ["eur", "Euro"], ["fjd", "Fijian Dollar"], ["fkp", "Falkland Islands Pound"],
  ["gbp", "British Pound Sterling"], ["gel", "Georgian Lari"], ["ghs", "Ghanaian Cedi"],
  ["gip", "Gibraltar Pound"], ["gmd", "Gambian Dalasi"], ["gnf", "Guinean Franc"],
  ["gtq", "Guatemalan Quetzal"], ["gyd", "Guyanaese Dollar"], ["hkd", "Hong Kong Dollar"],
  ["hnl", "Honduran Lempira"], ["htg", "Haitian Gourde"], ["huf", "Hungarian Forint"],
  ["idr", "Indonesian Rupiah"], ["ils", "Israeli New Sheqel"], ["inr", "Indian Rupee"],
  ["iqd", "Iraqi Dinar"], ["irr", "Iranian Rial"], ["isk", "Icelandic Króna"],
  ["jmd", "Jamaican Dollar"], ["jod", "Jordanian Dinar"], ["jpy", "Japanese Yen"],
  ["kes", "Kenyan Shilling"], ["kgs", "Kyrgystani Som"], ["khr", "Cambodian Riel"],
  ["kmf", "Comorian Franc"], ["kpw", "North Korean Won"], ["krw", "South Korean Won"],
  ["kwd", "Kuwaiti Dinar"], ["kyd", "Cayman Islands Dollar"], ["kzt", "Kazakhstani Tenge"],
  ["lak", "Laotian Kip"], ["lbp", "Lebanese Pound"], ["lkr", "Sri Lankan Rupee"],
  ["lrd", "Liberian Dollar"], ["lsl", "Lesotho Loti"], ["lyd", "Libyan Dinar"],
  ["mad", "Moroccan Dirham"], ["mdl", "Moldovan Leu"], ["mga", "Malagasy Ariary"],
  ["mkd", "Macedonian Denar"], ["mmk", "Myanma Kyat"], ["mnt", "Mongolian Tugrik"],
  ["mop", "Macanese Pataca"], ["mru", "Mauritanian Ouguiya"], ["mur", "Mauritian Rupee"],
  ["mvr", "Maldivian Rufiyaa"], ["mwk", "Malawian Kwacha"], ["mxn", "Mexican Peso"],
  ["myr", "Malaysian Ringgit"], ["mzn", "Mozambican Metical"], ["nad", "Namibian Dollar"],
  ["ngn", "Nigerian Naira"], ["nio", "Nicaraguan Córdoba"], ["nok", "Norwegian Krone"],
  ["npr", "Nepalese Rupee"], ["nzd", "New Zealand Dollar"], ["omr", "Omani Rial"],
  ["pab", "Panamanian Balboa"], ["pen", "Peruvian Nuevo Sol"], ["pgk", "Papua New Guinean Kina"],
  ["php", "Philippine Peso"], ["pkr", "Pakistani Rupee"], ["pln", "Polish Zloty"],
  ["pyg", "Paraguayan Guarani"], ["qar", "Qatari Rial"], ["ron", "Romanian Leu"],
  ["rsd", "Serbian Dinar"], ["rub", "Russian Ruble"], ["rwf", "Rwandan Franc"],
  ["sar", "Saudi Riyal"], ["sbd", "Solomon Islands Dollar"], ["scr", "Seychellois Rupee"],
  ["sdg", "Sudanese Pound"], ["sek", "Swedish Krona"], ["sgd", "Singapore Dollar"],
  ["shp", "Saint Helena Pound"], ["sll", "Sierra Leonean Leone"], ["sos", "Somali Shilling"],
  ["srd", "Surinamese Dollar"], ["stn", "São Tomé and Príncipe Dobra"], ["svc", "Salvadoran Colón"],
  ["syp", "Syrian Pound"], ["szl", "Swazi Lilangeni"], ["thb", "Thai Baht"],
  ["tjs", "Tajikistani Somoni"], ["tmt", "Turkmenistani Manat"], ["tnd", "Tunisian Dinar"],
  ["top", "Tongan Pa'anga"], ["try", "Turkish Lira"], ["ttd", "Trinidad and Tobago Dollar"],
  ["twd", "New Taiwan Dollar"], ["tzs", "T Tanzanian Shilling"], ["uah", "Ukrainian Hryvnia"],
  ["ugx", "Ugandan Shilling"], ["usd", "US Dollar"], ["uyu", "Uruguayan Peso"],
  ["uzs", "Uzbekistan Som"], ["ves", "Venezuelan Bolívar"], ["vnd", "Vietnamese Dong"],
  ["vuv", "Vanuatu Vatu"], ["wst", "Samoan Tala"], ["xaf", "CFA Franc BEAC"],
  ["xcd", "East Caribbean Dollar"], ["xof", "CFA Franc BCEAO"], ["xpf", "CFP Franc"],
  ["yer", "Yemeni Rial"], ["zar", "South African Rand"], ["zmw", "Zambian Kwacha"],
  ["zwl", "Zimbabwean Dollar"]
];

function populateDropdowns() {
    const stockSelect = document.querySelector('select[name="stock_symbol"]');
    if (stockSelect) {
        topStocks.forEach(s => {
            let opt = document.createElement("option");
            opt.value = s[1];
            opt.text = s[0] + " - " + s[1];
            stockSelect.add(opt);
        });
    }
    
    const cryptoSelect = document.querySelector('select[name="crypto_id"]');
    if (cryptoSelect) {
        topCoins.forEach(c => {
            let opt = document.createElement("option");
            opt.value = c[0];
            opt.text = c[1];
            cryptoSelect.add(opt);
        });
    }
    
    const baseSelect = document.querySelector('select[name="currency_base"]');
    const targetSelect = document.querySelector('select[name="currency_target"]');
    if (baseSelect && targetSelect) {
        allCurrencies.forEach(c => {
            let opt1 = document.createElement("option");
            opt1.value = c[0];
            opt1.text = c[0].toUpperCase() + " - " + c[1];
            baseSelect.add(opt1);
            
            let opt2 = document.createElement("option");
            opt2.value = c[0];
            opt2.text = c[0].toUpperCase() + " - " + c[1];
            targetSelect.add(opt2);
        });
    }
}

function setUiStatus(text, color) {
    const status = document.getElementById("status-text");
    if(status) {
        status.innerText = text;
        status.style.color = color;
    }
}

async function updateStats() {
    try {
        const jsonStr = await invoke("get_stats");
        if (!jsonStr || jsonStr === "{}") return;
        
        const data = JSON.parse(jsonStr);
        
        if (data.pc_id !== undefined) {
            const idText = document.getElementById("device-id-text");
            if (idText) {
                idText.innerText = data.pc_id.split(':')[0]; 
            }
        }

        if (data.cpu_percent !== undefined) {
            let cpu = Math.round(data.cpu_percent) + "%";
            document.getElementById("cpu").innerText = cpu;
            if(document.getElementById("remote-pc-cpu")) document.getElementById("remote-pc-cpu").innerText = cpu;
        }
        if (data.net_down_kb !== undefined) {
            let val = data.net_down_kb >= 1024 ? (data.net_down_kb / 1024).toFixed(1) : data.net_down_kb;
            let unit = data.net_down_kb >= 1024 ? "MB/s" : "KB/s";
            document.getElementById("dl-val").innerText = val;
            document.getElementById("dl-unit").innerText = unit;
            if(document.getElementById("remote-pc-net")) document.getElementById("remote-pc-net").innerText = val + " " + unit;
        }
        if (data.mem_percent !== undefined) {
            let ram = Math.round(data.mem_percent) + "%";
            document.getElementById("ram").innerText = ram;
            if(document.getElementById("remote-pc-ram")) document.getElementById("remote-pc-ram").innerText = ram;
        }
        if (data.disk_percent !== undefined) {
            let disk = Math.round(data.disk_percent) + "%";
            document.getElementById("disk").innerText = disk;
            if(document.getElementById("remote-pc-disk")) document.getElementById("remote-pc-disk").innerText = disk;
        }
    } catch (e) { }
}

function handleDisconnectUI() {
    isConfigLoaded = false;
    const wrap = document.getElementById("config-wrapper");
    const ph = document.getElementById("config-placeholder");
    if(wrap) wrap.classList.add("hidden");
    if(ph) { 
        ph.classList.remove("hidden"); 
        ph.innerText = "Device configuration will be displayed after connection is established."; 
    }
}

async function loadPorts() {
    try {
        const statusObj = await invoke("get_ports"); 
        const select = document.getElementById("port-select");
        const currentVal = select.value;

        select.innerHTML = ""; 
        
        if (statusObj.ports.length === 0) {
            let opt = document.createElement("option");
            opt.text = "No Ports Found";
            select.add(opt);
        } else {
            statusObj.ports.forEach((p) => {
                let opt = document.createElement("option");
                opt.value = p;
                opt.text = p;
                select.add(opt);
            });
        }

        if (statusObj.connected) {
            select.value = statusObj.connected;
            if (!isConnected) {
                isConnected = true;
                const btn = document.getElementById("conn-btn");
                if(btn) { btn.innerText = "Disconnect"; btn.className = "btn-red"; }
                if(select) select.disabled = true;

                const ph = document.getElementById("config-placeholder");
                if(ph && !isConfigLoaded) { ph.innerText = "Connection established - waiting for configuration data..."; }

                setTimeout(fetchDeviceData, 800);
            }
        } else {
            if (isConnected) {
                isConnected = false;
                const btn = document.getElementById("conn-btn");
                if(btn) { btn.innerText = "Connect"; btn.className = "btn-blue"; }
                if(select) select.disabled = false;
                
                handleDisconnectUI();
            }
            if (currentVal && statusObj.ports.includes(currentVal)) {
                select.value = currentVal;
            }
        }

        if (statusObj.status_text) {
            const lowerText = statusObj.status_text.toLowerCase();
            if (lowerText.includes("failed") || lowerText.includes("error") || lowerText.includes("❌")) {
                setUiStatus(statusObj.status_text, "#ef4444"); 
            } else if (lowerText.includes("wifi")) {
                setUiStatus(statusObj.status_text, "#3b82f6"); 
            } else if (lowerText.includes("usb")) {
                setUiStatus(statusObj.status_text, "#10b981"); 
            } else {
                setUiStatus(statusObj.status_text, "#888");    
            }
        }

        const ipText = document.getElementById("device-ip-text");
        const linkStatus = document.getElementById("tinytosh-link-status");

        if (ipText && linkStatus) {
            let activeConn = statusObj.connected || "";
            
            if (activeConn.startsWith("Serial:")) {
                if (currentDeviceIp) {
                    ipText.innerText = `TINYTOSH IP: ${currentDeviceIp}`;
                } else {
                    ipText.innerText = "TINYTOSH IP: --";
                }
            } else {
                if (statusObj.target_ip) {
                    currentDeviceIp = statusObj.target_ip;
                    ipText.innerText = `TINYTOSH IP: ${statusObj.target_ip}`;
                } else if (isConnected && currentDeviceIp) {
                    ipText.innerText = `TINYTOSH IP: ${currentDeviceIp}`;
                } else {
                    ipText.innerText = "TINYTOSH IP: --";
                }
            }

            if (isConnected && statusObj.connected) {
                let target = statusObj.connected;
                
                if (target.startsWith("WiFi:")) {
                    let name = target.split(" ")[1]; 
                    linkStatus.innerText = "🔒 PAIRED TO " + (name ? name.toUpperCase() : "TINYTOSH");
                    linkStatus.style.color = "#10b981";
                } 
                else if (target.startsWith("Serial:")) {
                    let port = target.replace("Serial: ", "");
                    let devName = currentDeviceId ? currentDeviceId.toUpperCase() : "USB DEVICE";
                    linkStatus.innerText = "🔒 PAIRED TO " + devName;
                    linkStatus.style.color = "#10b981";
                }
            } else {
                linkStatus.innerText = "NOT CONNECTED";
                linkStatus.style.color = "#888";
                currentDeviceId = "";
                currentDeviceIp = "";
            }
        }

    } catch (e) { }
}

async function toggleConnection() {
    const select = document.getElementById("port-select");
    if (!isConnected) {
        const port = select.value;
        if (!port || port === "No Ports Found") return;
        try {
            const ph = document.getElementById("config-placeholder");
            if(ph && !isConfigLoaded) { ph.innerText = "Connection established - waiting for configuration data..."; }

            await invoke("toggle_connection", { portName: port, connect: true });
            isConnected = true; 
            const btn = document.getElementById("conn-btn");
            if(btn) { btn.innerText = "Disconnect"; btn.className = "btn-red"; }
            if(select) select.disabled = true;

            setTimeout(fetchDeviceData, 800);

        } catch (error) {
            setUiStatus(error, "#ef4444");
        }
    } else {
        try {
            await invoke("toggle_connection", { portName: "", connect: false });
            isConnected = false;
            const btn = document.getElementById("conn-btn");
            if(btn) { btn.innerText = "Connect"; btn.className = "btn-blue"; }
            if(select) select.disabled = false;
            
            handleDisconnectUI();
        } catch(e) {}
    }
}

async function initAutostart() {
    const cb = document.getElementById("autostart-cb");
    if (!cb) return;
    try {
        const isEnabled = await invoke("check_autostart");
        cb.checked = isEnabled;
        cb.addEventListener("change", async (e) => {
            try { await invoke("set_autostart", { enable: e.target.checked }); } 
            catch (err) { e.target.checked = !e.target.checked; }
        });
    } catch (e) { }
}

function updateVisibility() {
  var pairs = [
      ['autoDetect','manualFields',true], ['nightMode','nightFields',false], 
      ['showTime', 'timeContent',false], ['showWeather','weatherContent',false], 
      ['showPc','pcContent',false], ['showCrypto','cryptoContent',false], 
      ['showCurrency','currencyContent',false], ['showStock','stockContent',false], 
      ['showAQI','aqiContent',false]
  ];
  pairs.forEach(p => {
    var ch = document.getElementById(p[0]); if(!ch) return;
    var target = document.getElementById(p[1]);
    var shouldHide = p[2] ? ch.checked : !ch.checked;
    target.className = shouldHide ? 'collapsible hidden' : 'collapsible';
    target.querySelectorAll('input, select').forEach(el => el.disabled = shouldHide);
  });

  var ac = document.getElementById('autoCycle');
  var si = document.getElementById('screenIntInput');
  if(ac && si) si.disabled = !ac.checked;
}

function reorderPhysicalPanels(orderCsv) {
    const container = document.getElementById('dynamic-panels-container');
    if (!container || !orderCsv) return;
    
    const orderArr = orderCsv.split(',');
    orderArr.forEach(id => {
        const panel = document.getElementById(`panel-${id}`);
        if (panel) {
            container.appendChild(panel);
        }
    });
}

function syncScreenOrder(isUserInput = false) {
  const list = document.getElementById('sortable-list');
  const orderInput = document.getElementById('screenOrderInput');
  const items = [...list.querySelectorAll('.sortable-item')];
  let enabled = [], disabled = [];
  
  items.forEach(item => {
    const targetId = item.getAttribute('data-target');
    const cb = document.getElementById(targetId);
    if (cb && cb.checked) {
      item.classList.remove('disabled'); item.setAttribute('draggable', 'true'); enabled.push(item);
    } else {
      item.classList.add('disabled'); item.removeAttribute('draggable'); disabled.push(item);
    }
  });
  list.innerHTML = '';
  enabled.forEach(el => list.appendChild(el)); 
  disabled.forEach(el => list.appendChild(el)); 
  
  const currentOrder = [...list.querySelectorAll('.sortable-item')].map(item => item.getAttribute('data-id')).join(',');
  orderInput.value = currentOrder;
  
  reorderPhysicalPanels(currentOrder);
  
  if (isUserInput) formDirty = true;
}

function getDragAfterEl(y) {
  const list = document.getElementById('sortable-list');
  return [...list.querySelectorAll('.sortable-item:not(.dragging):not(.disabled)')].reduce((closest, child) => {
    const box = child.getBoundingClientRect();
    const offset = y - box.top - box.height / 2;
    if (offset < 0 && offset > closest.offset) return { offset: offset, element: child };
    else return closest;
  }, { offset: Number.NEGATIVE_INFINITY }).element;
}

function moveItem(y) {
  const list = document.getElementById('sortable-list');
  const draggable = document.querySelector('.dragging');
  if (!draggable) return;
  const afterEl = getDragAfterEl(y);
  if (afterEl == null) {
    const firstDis = list.querySelector('.disabled');
    if (firstDis) list.insertBefore(draggable, firstDis);
    else list.appendChild(draggable);
  } else { list.insertBefore(draggable, afterEl); }
}

function toggleNone() {
  const noneBox = document.getElementById('animNone');
  const others = document.querySelectorAll('.anim-chk');
  others.forEach(cb => {
    cb.disabled = noneBox.checked;
    if(noneBox.checked) cb.checked = false;
    cb.parentElement.style.opacity = noneBox.checked ? '0.5' : '1';
  });
}

function checkSafetyNet() {
  if(!document.getElementById('animNone').checked) {
    let count = 0;
    document.querySelectorAll('.anim-chk').forEach(cb => { if(cb.checked) count++; });
    if(count === 0) {
      document.getElementById('animNone').checked = true;
      toggleNone();
    }
  }
}

async function fetchDeviceData() {
    try {
        const jsonStr = await invoke("fetch_device_data");
        const d = JSON.parse(jsonStr);

        const set = (id, val, html=false) => { const el = document.getElementById(id); if(el && val !== undefined) { if(html) el.innerHTML = val; else el.innerText = val; return true; } return false; };
        const setVal = (name, val) => { const el = document.querySelector('[name="'+name+'"]'); if(el && document.activeElement !== el && val !== undefined) el.value = val; };
        const setCb = (id, val, byName=false) => { 
            const el = byName ? document.querySelector('[name="'+id+'"]') : document.getElementById(id); 
            if(el) el.checked = (val == 1 || val === true || val === "1" || val === "true"); 
        };
        const setRadio = (name, val) => { const el = document.querySelector('[name="'+name+'"][value="'+val+'"]'); if(el) el.checked = true; };

        if (d.device_id !== undefined) {
          currentDeviceId = d.device_id;
        }
        if (d.ip_address !== undefined) {
          currentDeviceIp = d.ip_address;
        }
        if (d.device_id !== undefined || d.ip_address !== undefined) {
          loadPorts();
        }
        
        if (d.refresh_min !== undefined && !formDirty) {
            isConfigLoaded = true;
            const wrap = document.getElementById("config-wrapper");
            const ph = document.getElementById("config-placeholder");
            if(wrap) wrap.classList.remove("hidden");
            if(ph) ph.classList.add("hidden");

            setVal('refresh_min', d.refresh_min);
            setCb('autoCycle', d.auto_cycle);
            setVal('screen_int', d.screen_int);
            setRadio('time_format', d.time_format);
            
            setCb('autoDetect', d.auto_detect);
            setVal('city', d.city);
            setVal('latitude', d.latitude);
            setVal('longitude', d.longitude);
            setVal('timezone', d.timezone);
            
            setCb('nightMode', d.night_mode);
            setVal('night_start', d.night_start);
            setVal('night_end', d.night_end);
            setVal('night_action', d.night_action);
            
            setCb('showTime', d.show_time);
            setCb('date_display', d.date_display, true);
            setCb('showWeather', d.show_weather);
            setRadio('temp_unit', d.temp_unit);
            setCb('round_temps', d.round_temps, true);
            setCb('showAQI', d.show_aqi);
            setRadio('aqi_type', d.aqi_type);
            setCb('showPc', d.show_pc);
            setCb('showStock', d.show_stock);
            setVal('stock_symbol', d.stock_symbol);
            setCb('stock_fn', d.stock_fn, true);
            setCb('showCrypto', d.show_crypto);
            setVal('crypto_id', d.crypto_id);
            setCb('crypto_fn', d.crypto_fn, true);
            setCb('showCurrency', d.show_currency);
            setVal('currency_base', d.currency_base);
            setVal('currency_target', d.currency_target);
            setVal('currency_multiplier', d.currency_multiplier);
            setCb('currency_fn', d.currency_fn, true);

            if (d.anim_mask !== undefined) {
                const mask = d.anim_mask;
                document.querySelectorAll('.anim-chk').forEach(cb => { cb.checked = (mask & parseInt(cb.value)) !== 0; });
                const noneBox = document.getElementById('animNone');
                if (noneBox) { noneBox.checked = (mask === 0); toggleNone(); }
            }

            if (d.screen_order && !document.querySelector('.dragging')) {
                const orderArr = d.screen_order.split(',');
                const list = document.getElementById('sortable-list');
                if (list) {
                    const items = [...list.querySelectorAll('.sortable-item')];
                    orderArr.forEach(id => { const item = items.find(el => el.getAttribute('data-id') === String(id)); if(item) list.appendChild(item); });
                    document.getElementById('screenOrderInput').value = d.screen_order;
                    reorderPhysicalPanels(d.screen_order);
                }
            }

            updateVisibility();
            syncScreenOrder(false);
            formDirty = false;
        }

        let timeStr = d.time;
        let dateStr = d.date;

        if (!timeStr) {
            const now = new Date();
            const formatRadio = document.querySelector('input[name="time_format"][value="12"]');
            const use12Hour = formatRadio ? formatRadio.checked : false;
            timeStr = now.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', hour12: use12Hour});
            dateStr = now.toLocaleDateString([], {weekday: 'long', month: 'short', day: 'numeric'});
        }

        set('time-display', timeStr);
        set('preview-time', timeStr);
        set('preview-date', dateStr);

        const city = d.city || document.querySelector('input[name="city"]')?.value || "--";
        const tz = d.timezone || document.querySelector('select[name="timezone"]')?.value || "--";
        const locInfo = document.getElementById("location-info");
        if (locInfo) locInfo.innerText = `📍 ${city} (${tz})`;

        if (d.temp !== undefined) {
            let nd = document.getElementById('weather-no-data'); if(nd) nd.style.display = 'none';
            let gr = document.getElementById('weather-grid'); if(gr) gr.classList.remove('hidden');

            set('value-temp', d.temp + ' °' + d.temp_unit);
            set('value-feels', d.apparent_temperature + ' °' + d.temp_unit);
            set('value-hum', d.humidity + '%');
            set('value-wind', d.wind_speed + ' km/h');
            set('weather-upd', 'Last Update: ' + d.update_time);
        }

        if (d.aqi !== undefined && d.aqi !== -1) {
            let nd = document.getElementById('aqi-no-data'); if(nd) nd.style.display = 'none';
            let gr = document.getElementById('aqi-grid'); if(gr) gr.classList.remove('hidden');

            set('value-aqi', d.aqi);
            const aqiLabel = document.querySelector('#value-aqi + .tile-label'); if(aqiLabel) aqiLabel.innerText = d.aqi_status + ' Index';
            set('value-pm25', d.pm25 + ' <small>µg</small>', true);
            set('value-pm10', d.pm10 + ' <small>µg</small>', true);
            set('value-no2', d.no2 + ' <small>µg</small>', true);
            set('aqi-upd', 'Last Update: ' + d.update_time);
        }

        if (d.crypto_price !== undefined) {
            let nd = document.getElementById('crypto-no-data'); if(nd) nd.style.display = 'none';
            let gr = document.getElementById('crypto-grid'); if(gr) gr.classList.remove('hidden');

            set('crypto-price', d.crypto_price + '$');
            set('crypto-change', d.crypto_change + '%');
            set('crypto-trend-icon', d.crypto_change >= 0 ? '📈' : '📉');
            set('crypto-upd', 'Last Update: ' + d.update_time);
        }

        if (d.currency_base_text !== undefined) {
            let nd = document.getElementById('currency-no-data'); if(nd) nd.style.display = 'none';
            let gr = document.getElementById('currency-grid'); if(gr) gr.classList.remove('hidden');

            set('currency-base-val', d.currency_base_text);
            set('currency-target-val', d.currency_target_text);
            set('currency-upd', 'Last Update: ' + d.update_time);
        }
        
        if (d.stock_price !== undefined) {
            let nd = document.getElementById('stock-no-data'); if(nd) nd.style.display = 'none';
            let gr = document.getElementById('stock-grid'); if(gr) gr.classList.remove('hidden');

            set('stock-price', '$' + d.stock_price);
            set('stock-change', d.stock_change + '%');
            set('stock-trend-icon', d.stock_change >= 0 ? '📈' : '📉');
            set('stock-sym', d.stock_symbol + ' Price');
            set('stock-upd', 'Last Update: ' + d.update_time);
        }

    } catch (e) {
        console.error("Config Sync Error:", e.message); 
    }
}

window.addEventListener("DOMContentLoaded", () => {
    populateDropdowns();
    
    const btn = document.getElementById("conn-btn");
    if(btn) btn.addEventListener("click", toggleConnection);
    initAutostart();
    loadPorts();
    
    setInterval(loadPorts, 2000); 
    setInterval(updateStats, 1000); 
    setInterval(fetchDeviceData, 15000); 
    setTimeout(fetchDeviceData, 2000); 

    ['autoDetect', 'nightMode', 'showTime', 'showWeather', 'showPc', 'showCrypto', 'showCurrency', 'showStock', 'showAQI', 'autoCycle'].forEach(id => { 
        var el = document.getElementById(id); 
        if(el) el.addEventListener('change', () => { updateVisibility(); syncScreenOrder(true); }); 
    });
    updateVisibility();

    const nb = document.getElementById('animNone');
    if(nb) nb.addEventListener('change', toggleNone);
    document.querySelectorAll('.anim-chk').forEach(cb => cb.addEventListener('change', checkSafetyNet));
    toggleNone();

    const list = document.getElementById('sortable-list');
    list.addEventListener('dragstart', e => { 
        const item = e.target.closest('.sortable-item');
        if (!item || item.classList.contains('disabled')) { e.preventDefault(); return; }
        item.classList.add('dragging'); 
        if(e.dataTransfer) {
            e.dataTransfer.setData('text/plain', item.dataset.id);
            e.dataTransfer.effectAllowed = 'move';
        }
    });
    list.addEventListener('dragend', e => { 
        const item = e.target.closest('.sortable-item');
        if(item) item.classList.remove('dragging'); 
        syncScreenOrder(true); 
    }); 
    list.addEventListener('dragover', e => { e.preventDefault(); moveItem(e.clientY); });
    syncScreenOrder();

    document.getElementById('settings-form').addEventListener('input', () => formDirty = true);
    document.getElementById('settings-form').addEventListener('change', () => formDirty = true);
    
    document.getElementById('save-settings-btn').addEventListener('click', async (e) => {
        e.preventDefault(); 
        
        const saveBtn = document.getElementById('save-settings-btn');
        saveBtn.innerText = "⏳ Saving...";
        saveBtn.style.opacity = "0.7";

        try {
            let mask = 0;
            document.querySelectorAll('.anim-chk').forEach(cb => { if(cb.checked) mask += parseInt(cb.value); });
            document.getElementById('finalMask').value = mask;

            const form = document.getElementById('settings-form');
            const formData = new FormData(form);
            
            const queryStr = new URLSearchParams(formData).toString();
            const jsonObj = {};
            
            formData.forEach((value, key) => {
                if (value === "on") jsonObj[key] = 1;
                else if (!isNaN(value) && value.trim() !== "") jsonObj[key] = Number(value);
                else jsonObj[key] = value;
            });
            
            form.querySelectorAll('input[type="checkbox"]').forEach(cb => { jsonObj[cb.name] = cb.checked ? 1 : 0; });
            jsonObj['anim_mask'] = parseInt(document.getElementById('finalMask').value);
            jsonObj['screen_order'] = document.getElementById('screenOrderInput').value;

            const jsonPayload = JSON.stringify(jsonObj);
            
            await invoke("save_device_settings", { query: queryStr, jsonPayload: jsonPayload });
            
            saveBtn.innerText = "✅ Saved Successfully!";
            saveBtn.style.backgroundColor = "#10b981";
            formDirty = false;
            setTimeout(fetchDeviceData, 1000);
            
        } catch (err) {
            console.error("Save Error:", err);
            alert("Backend Error: " + err); 
            saveBtn.innerText = "❌ Failed to Save";
            saveBtn.style.backgroundColor = "#ef4444";
        }

        setTimeout(() => {
            saveBtn.innerText = "💾 Save & Apply All Settings";
            saveBtn.style.backgroundColor = "var(--accent)";
            saveBtn.style.opacity = "1";
        }, 3000);
    });
});