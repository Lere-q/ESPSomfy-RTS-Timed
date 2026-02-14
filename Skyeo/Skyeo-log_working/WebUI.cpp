#include "WebUI.h"

// ============================================
// SKYEO - Embedded Web UI Implementation
// Mit allen Features: Scrollbarer WLAN-Scan,
// Antennen-Konfiguration, Logs-Tab
// ============================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Skyeo - Somfy Controller</title>
    <style>
:root{--primary:#2196F3;--success:#4CAF50;--danger:#f44336;--warning:#ff9800;--bg:#f5f5f5;--card:#fff;--text:#333;--border:#ddd}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);line-height:1.6}
.header{background:var(--primary);color:#fff;padding:1rem;text-align:center;box-shadow:0 2px 4px rgba(0,0,0,0.1);position:sticky;top:0;z-index:100}
.header h1{font-size:1.5rem;margin-bottom:0.25rem}
.header .version{font-size:0.75rem;opacity:0.8}
.container{max-width:1000px;margin:0 auto;padding:1rem;padding-bottom:100px}
.card{background:var(--card);border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);margin-bottom:1rem;padding:1rem}
.card h2{font-size:1.1rem;margin-bottom:0.75rem;color:var(--primary);border-bottom:2px solid var(--primary);padding-bottom:0.5rem}
.btn{display:inline-block;padding:0.6rem 1.2rem;border:none;border-radius:4px;cursor:pointer;font-size:0.9rem;transition:all 0.3s;margin:0.25rem}
.btn-primary{background:var(--primary);color:#fff}
.btn-success{background:var(--success);color:#fff}
.btn-danger{background:var(--danger);color:#fff}
.btn-warning{background:var(--warning);color:#fff}
.btn:hover{opacity:0.9;transform:translateY(-1px)}
.btn-sm{padding:0.4rem 0.8rem;font-size:0.8rem}
.tabs{display:flex;gap:0.3rem;margin-bottom:1rem;border-bottom:2px solid #e0e0e0;flex-wrap:wrap;position:sticky;top:60px;background:var(--bg);z-index:99;padding-top:0.5rem}
.tab-btn{padding:0.6rem 1rem;background:none;border:none;cursor:pointer;border-bottom:3px solid transparent;transition:all 0.3s;font-size:0.9rem}
.tab-btn.active{border-bottom-color:var(--primary);color:var(--primary);font-weight:600}
.tab-content{display:none}
.tab-content.active{display:block}
.subtabs{display:flex;gap:0.3rem;margin-bottom:1rem;border-bottom:1px solid #ddd;padding-bottom:0.5rem;flex-wrap:wrap}
.subtab-btn{padding:0.5rem 0.8rem;background:#f5f5f5;border:1px solid #ddd;border-radius:4px;cursor:pointer;font-size:0.85rem;transition:all 0.2s}
.subtab-btn.active{background:var(--primary);color:#fff;border-color:var(--primary)}
.form-group{margin-bottom:0.8rem}
.form-group label{display:block;margin-bottom:0.3rem;font-weight:500;font-size:0.9rem}
.form-group input,.form-group select{width:100%;padding:0.6rem;border:1px solid #ddd;border-radius:4px;font-size:0.9rem}
.form-group input:focus,.form-group select:focus{outline:none;border-color:var(--primary)}
.grid-3{display:grid;grid-template-columns:repeat(3,1fr);gap:0.8rem}
.grid-2{display:grid;grid-template-columns:1fr 1fr;gap:0.8rem}
@media(max-width:600px){.grid-2,.grid-3{grid-template-columns:1fr}}
.status-indicator{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:0.4rem}
.status-online{background:var(--success);box-shadow:0 0 5px var(--success)}
.status-offline{background:var(--danger)}
.status-connecting{background:var(--warning)}

/* Scrollbarer WLAN-Container */
.wifi-scan-container{max-height:300px;overflow-y:auto;border:1px solid #ddd;border-radius:4px;padding:0.5rem;background:#fafafa}
.wifi-scan-container::-webkit-scrollbar{width:8px}
.wifi-scan-container::-webkit-scrollbar-track{background:#f1f1f1;border-radius:4px}
.wifi-scan-container::-webkit-scrollbar-thumb{background:#888;border-radius:4px}
.wifi-scan-container::-webkit-scrollbar-thumb:hover{background:#555}
.wifi-network{padding:0.6rem;border:1px solid #e0e0e0;border-radius:4px;margin-bottom:0.4rem;cursor:pointer;background:#fff;transition:all 0.2s;display:flex;justify-content:space-between;align-items:center}
.wifi-network:hover{background:#e3f2fd;border-color:var(--primary);transform:translateX(2px)}
.wifi-signal{font-weight:600;font-size:0.85rem}
.wifi-signal.strong{color:var(--success)}
.wifi-signal.medium{color:var(--warning)}
.wifi-signal.weak{color:var(--danger)}

/* Logs Container */
.logs-container{background:#1e1e1e;color:#d4d4d4;padding:0.8rem;border-radius:4px;font-family:'Courier New',monospace;font-size:0.8rem;height:400px;overflow-y:auto;margin-top:0.5rem;border:2px solid #333}
.logs-container::-webkit-scrollbar{width:10px}
.logs-container::-webkit-scrollbar-track{background:#2d2d2d}
.logs-container::-webkit-scrollbar-thumb{background:#555;border-radius:5px}
.log-entry{padding:0.3rem 0;border-bottom:1px solid #333;display:flex;gap:0.8rem;align-items:center}
.log-entry:last-child{border-bottom:none}
.log-time{color:#858585;min-width:60px;font-size:0.75rem}
.log-address{color:#4fc1ff;min-width:80px}
.log-cmd{color:#ce9178;min-width:60px}
.log-rc{color:#b5cea8;min-width:70px}
.log-rssi{color:#dcdcaa;min-width:60px}
.log-direction{font-size:0.7rem;padding:0.1rem 0.3rem;border-radius:3px;margin-left:auto}
.log-direction.tx{background:#2196F3;color:#fff}
.log-direction.rx{background:#4CAF50;color:#fff}

/* Pairing Steps */
.pairing-container{background:#f5f5f5;padding:1rem;border-radius:8px;margin-top:1rem}
.pairing-step{display:flex;align-items:flex-start;gap:1rem;padding:1rem;margin-bottom:0.5rem;background:#fff;border-radius:6px;border-left:4px solid #ccc;transition:all 0.3s}
.pairing-step.active{border-left-color:var(--primary);background:#e3f2fd;box-shadow:0 2px 8px rgba(33,150,243,0.2)}
.pairing-step.completed{border-left-color:var(--success);background:#e8f5e9;opacity:0.7}
.pairing-step.error{border-left-color:var(--danger);background:#ffebee}
.step-number{width:30px;height:30px;border-radius:50%;background:#ccc;color:#fff;display:flex;align-items:center;justify-content:center;font-weight:bold;flex-shrink:0}
.pairing-step.active .step-number{background:var(--primary)}
.pairing-step.completed .step-number{background:var(--success)}
.pairing-step.error .step-number{background:var(--danger)}
.step-content{flex:1}
.step-title{font-weight:600;margin-bottom:0.3rem}
.step-desc{font-size:0.85rem;color:#666}

.modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.5);z-index:1000;justify-content:center;align-items:center}
.modal.active{display:flex}
.modal-content{background:#fff;padding:1.5rem;border-radius:8px;max-width:450px;width:90%;max-height:85vh;overflow-y:auto}
.modal-header{margin-bottom:1rem}
.modal-header h3{margin:0;color:var(--primary)}
.modal-footer{display:flex;justify-content:flex-end;gap:0.5rem;margin-top:1rem}
.toast{position:fixed;bottom:1rem;right:1rem;background:#333;color:#fff;padding:0.8rem 1rem;border-radius:4px;box-shadow:0 4px 12px rgba(0,0,0,0.3);transform:translateX(150%);transition:transform 0.3s;z-index:2000;max-width:300px;font-size:0.9rem}
.toast.show{transform:translateX(0)}
.toast.success{background:var(--success)}
.toast.error{background:var(--danger)}

.shade-item{display:flex;align-items:center;justify-content:space-between;padding:0.8rem;border:1px solid #e0e0e0;border-radius:6px;margin-bottom:0.5rem;background:#fafafa}
.shade-controls{display:flex;gap:0.4rem;align-items:center;flex-wrap:wrap}
.position-slider{width:80px;margin:0 0.3rem}
.empty-state{text-align:center;padding:2rem;color:#999;font-style:italic}
.frequency-badge{display:inline-block;background:var(--primary);color:#fff;padding:0.2rem 0.6rem;border-radius:12px;font-size:0.75rem;font-weight:600;margin-left:0.5rem}
    </style>
</head>
<body>
    <div class="header">
        <h1>Skyeo <span class="frequency-badge">433 MHz</span></h1>
        <div class="version">v1.0.0 | <span id="connectionStatus"><span class="status-indicator status-connecting"></span>Verbinden...</span></div>
    </div>
    
    <!-- Antenna Status Warning Banner -->
    <div id="antennaWarning" style="display:none;background:var(--danger);color:#fff;padding:1rem;text-align:center;font-weight:600">
        <span style="font-size:1.2rem">⚠️</span> CC1101 Antenne NICHT initialisiert! <br>
        <span style="font-size:0.85rem;font-weight:normal">Gehe zu Einstellungen → Antenne um die Pins zu konfigurieren</span>
    </div>

    <div class="container">
        <div class="tabs">
            <button class="tab-btn active" onclick="showTab('shades')">🏠 Rolläden</button>
            <button class="tab-btn" onclick="showTab('schedules')">⏰ Zeitpläne</button>
            <button class="tab-btn" onclick="showTab('settings')">⚙️ Einstellungen</button>
        </div>

        <!-- Rolläden Tab -->
        <div id="tab-shades" class="tab-content active">
            <div class="card">
                <h2>Meine Rolläden</h2>
                <div id="shadesList"></div>
                <div style="text-align:center;margin-top:1rem">
                    <button class="btn btn-primary" onclick="showAddShadeModal()">+ Rolladen hinzufügen</button>
                </div>
            </div>
        </div>

        <!-- Zeitpläne Tab -->
        <div id="tab-schedules" class="tab-content">
            <div class="card">
                <h2>Zeitgesteuerte Aktionen</h2>
                <div id="schedulesList"></div>
                <div style="text-align:center;margin-top:1rem">
                    <button class="btn btn-primary" onclick="showAddScheduleModal()">+ Zeitplan erstellen</button>
                </div>
            </div>
        </div>

        <!-- Einstellungen Tab -->
        <div id="tab-settings" class="tab-content">
            <div class="subtabs">
                <button class="subtab-btn active" onclick="showSubTab('settings-wifi')">📶 WLAN</button>
                <button class="subtab-btn" onclick="showSubTab('settings-antenna')">📡 Antenne</button>
                <button class="subtab-btn" onclick="showSubTab('settings-logs')">📋 Logs</button>
                <button class="subtab-btn" onclick="showSubTab('settings-pairing')">🔗 Pairing</button>
                <button class="subtab-btn" onclick="showSubTab('settings-led')">💡 LED</button>
                <button class="subtab-btn" onclick="showSubTab('settings-system')">⚙️ System</button>
            </div>
            
            <!-- WLAN Sub-Tab -->
            <div id="subtab-settings-wifi" class="subtab-content active">
            <div class="card">
                <h2>WLAN Konfiguration</h2>
                <div style="margin-bottom:1rem">
                    <button class="btn btn-primary" onclick="scanWifiNetworks()">
                        <span id="scanBtnText">📶 Nach Netzwerken suchen</span>
                    </button>
                </div>
                
                <!-- Scrollbarer Container für WLAN-Ergebnisse -->
                <div id="wifiScanResults" class="wifi-scan-container" style="display:none;margin-bottom:1rem">
                    <!-- WLAN-Netzwerke werden hier eingefügt -->
                </div>
                
                <form id="wifiForm" onsubmit="saveWifi(event)">
                    <div class="form-group">
                        <label>SSID (Netzwerkname)</label>
                        <input type="text" id="wifiSsid" placeholder="MeinWLAN" required>
                    </div>
                    <div class="form-group">
                        <label>Passwort (optional für offene Netzwerke)</label>
                        <input type="password" id="wifiPass" placeholder="••••••••">
                    </div>
                    <button type="submit" class="btn btn-primary">💾 Speichern & Neustarten</button>
                </form>
            </div>
            </div>
            
            <!-- Antenne Sub-Tab -->
            <div id="subtab-settings-antenna" class="subtab-content" style="display:none">
            <div class="card">
                <h2>CC1101 Antennen-Konfiguration</h2>
                <div id="antennaStatusInfo" style="margin-bottom:1rem;padding:0.8rem;border-radius:4px;font-size:0.9rem">
                    Lade Status...
                </div>
                
                <p style="margin-bottom:1rem;color:#666;font-size:0.9rem">Konfiguriere die GPIO-Pins für den CC1101 Transceiver.</p>
                
                <div class="grid-3">
                    <div class="form-group">
                        <label>SCLK (Clock)</label>
                        <input type="number" id="pinSclk" value="18" min="0" max="39">
                    </div>
                    <div class="form-group">
                        <label>CSN (Chip Select)</label>
                        <input type="number" id="pinCsn" value="5" min="0" max="39">
                    </div>
                    <div class="form-group">
                        <label>MOSI (Data Out)</label>
                        <input type="number" id="pinMosi" value="23" min="0" max="39">
                    </div>
                    <div class="form-group">
                        <label>MISO (Data In)</label>
                        <input type="number" id="pinMiso" value="19" min="0" max="39">
                    </div>
                    <div class="form-group">
                        <label>GDO0 (TX)</label>
                        <input type="number" id="pinTx" value="22" min="0" max="39">
                    </div>
                    <div class="form-group">
                        <label>GDO2 (RX)</label>
                        <input type="number" id="pinRx" value="21" min="0" max="39">
                    </div>
                </div>
                
                <div style="display:flex;gap:0.5rem;margin-top:1rem;flex-wrap:wrap">
                    <button class="btn btn-primary" onclick="saveAntennaConfig()">💾 Konfiguration speichern</button>
                    <button class="btn" onclick="loadDefaultPins()">🔄 Standardwerte</button>
                    <button class="btn btn-success" onclick="testAntennaConnection()">🔄 Verbindung testen</button>
                </div>
                
                <div style="margin-top:1rem;padding:0.8rem;background:#fff3cd;border-radius:4px;border-left:4px solid #ffc107;font-size:0.85rem">
                    <strong>⚠️ Hinweis:</strong> Nach dem Speichern startet das Gerät neu. Stelle sicher, dass die Pins korrekt mit dem CC1101 Modul verbunden sind.
                </div>
                
                <div style="margin-top:1rem;padding:0.8rem;background:#e3f2fd;border-radius:4px;font-size:0.85rem">
                    <strong>📡 Frequenz:</strong> 433.42 MHz (für Somfy RTS)<br>
                    <strong>🔧 Modulation:</strong> OOK (On-Off Keying)<br>
                    <strong>📶 Datenrate:</strong> 40 kbps
                </div>
            </div>
            </div>
            
            <!-- Logs Sub-Tab -->
            <div id="subtab-settings-logs" class="subtab-content" style="display:none">
            <div class="card">
                <h2>Signalempfangs-Logs <small style="font-weight:normal;color:#666">(433 MHz)</small></h2>
                <div style="display:flex;gap:0.5rem;margin-bottom:0.5rem;flex-wrap:wrap">
                    <button class="btn btn-sm" onclick="clearLogs()">🗑️ Löschen</button>
                    <button class="btn btn-primary btn-sm" onclick="exportLogs()">💾 Export</button>
                </div>
                <div style="padding:0.5rem;background:#e8f5e9;border-radius:4px;margin-bottom:0.5rem;font-size:0.85rem">
                    <span class="status-indicator status-online" style="animation:pulse 1s infinite"></span> 
                    <strong>Empfangsmodus immer aktiv</strong> - Die Antenne empfängt ständig Signale
                </div>
                <div class="logs-container" id="antennaLogs">
                    <div class="log-entry" style="color:#858585;font-style:italic">Warte auf empfangene Signale... Drücke eine Taste an deiner Fernbedienung.</div>
                </div>
                <div style="margin-top:0.5rem;font-size:0.8rem;color:#666;display:flex;gap:1rem;flex-wrap:wrap">
                    <span>📊 <strong id="rxCount">0</strong> Signale empfangen</span>
                    <span>📡 RSSI: <strong id="lastRssi">-</strong> dBm</span>
                    <span>⏱️ Letztes: <strong id="lastRxTime">-</strong></span>
                </div>
            </div>
            </div>
            
            <!-- Pairing Sub-Tab -->
            <div id="subtab-settings-pairing" class="subtab-content" style="display:none">
            <div class="card">
                <h2>Rolladen Pairing-Assistent</h2>
                <div id="pairingIntro">
                    <p style="margin-bottom:1rem;color:#666">Verbinde deinen Rolladen mit Skyeo. Folge den Anweisungen Schritt für Schritt.</p>
                    <div class="form-group">
                        <label>Rolladen auswählen</label>
                        <select id="pairingShadeSelect" style="width:100%"></select>
                    </div>
                    <button class="btn btn-primary btn-lg" onclick="startPairing()" style="width:100%;margin-top:0.5rem">🚀 Pairing starten</button>
                </div>
                
                <div id="pairingProgress" class="pairing-container" style="display:none">
                    <div id="step1" class="pairing-step">
                        <div class="step-number">1</div>
                        <div class="step-content">
                            <div class="step-title">PROG-Taste drücken</div>
                            <div class="step-desc">Halte die PROG-Taste an deiner Original-Fernbedienung <strong>3 Sekunden</strong> gedrückt, bis der Rolladen kurz juckelt.</div>
                        </div>
                    </div>
                    <div id="step2" class="pairing-step">
                        <div class="step-number">2</div>
                        <div class="step-content">
                            <div class="step-title">Signal senden</div>
                            <div class="step-desc">Skyeo sendet automatisch das PROG-Signal. Warte auf die Bestätigung...</div>
                        </div>
                    </div>
                    <div id="step3" class="pairing-step">
                        <div class="step-number">3</div>
                        <div class="step-content">
                            <div class="step-title">Bestätigen</div>
                            <div class="step-desc">Drücke nochmals kurz die PROG-Taste an der Fernbedienung um das Pairing zu bestätigen.</div>
                        </div>
                    </div>
                    <div style="text-align:center;margin-top:1rem">
                        <button class="btn btn-danger" onclick="stopPairing()">❌ Abbrechen</button>
                    </div>
                </div>
                
                <div id="pairingStatus" style="margin-top:1rem;padding:0.8rem;background:#f5f5f5;border-radius:4px;font-size:0.9rem">
                    Bereit für Pairing
                </div>
            </div>
            </div>
            
            <!-- LED Sub-Tab -->
            <div id="subtab-settings-led" class="subtab-content" style="display:none">
            <div class="card">
                <h2>Debug LED Steuerung</h2>
                <div id="ledStatusInfo" style="margin-bottom:1rem;padding:0.8rem;border-radius:4px;font-size:0.9rem;background:#e3f2fd;border-left:4px solid #2196F3">
                    Lade LED Status...
                </div>
                
                <div class="form-group">
                    <label>LED Status</label>
                    <div style="display:flex;gap:0.5rem">
                        <button class="btn btn-success" onclick="ledOn()">💡 LED An</button>
                        <button class="btn btn-danger" onclick="ledOff()">🌑 LED Aus</button>
                    </div>
                </div>
                
                <div class="form-group">
                    <label>Helligkeit</label>
                    <div style="display:flex;align-items:center;gap:1rem">
                        <input type="range" id="ledBrightness" min="0" max="255" value="128" style="flex:1" onchange="ledSetBrightness(this.value)">
                        <span id="ledBrightnessValue" style="min-width:50px">128</span>
                    </div>
                </div>
                
                <div class="form-group">
                    <label>Blinkmuster</label>
                    <select id="ledPatternSelect" onchange="ledSetPattern(this.value)" style="width:100%">
                        <option value="OFF">Aus</option>
                        <option value="ON">Dauerhaft an</option>
                        <option value="BLINK_SLOW">Langsames Blinken (1s)</option>
                        <option value="BLINK_FAST">Schnelles Blinken (200ms)</option>
                        <option value="BLINK_PATTERN_WIFI">WLAN-Verbindung</option>
                        <option value="BLINK_PATTERN_DATA">Daten aktiv</option>
                        <option value="BLINK_PATTERN_PAIRING">Pairing-Modus</option>
                        <option value="BLINK_PATTERN_ERROR">Fehler</option>
                    </select>
                </div>
                
                <div style="margin-top:1rem">
                    <button class="btn btn-primary" onclick="showLEDPatternInfo()">ℹ️ Blinkmuster anzeigen</button>
                </div>
                
                <div id="ledPatternInfo" style="display:none;margin-top:1rem;padding:1rem;background:#f5f5f5;border-radius:4px;font-size:0.85rem">
                    <h3 style="margin-bottom:0.5rem;font-size:1rem">Bedeutung der Blinkmuster:</h3>
                    <div style="display:grid;gap:0.5rem">
                        <div><strong>Aus:</strong> LED ist komplett aus</div>
                        <div><strong>Dauerhaft an:</strong> LED leuchtet konstant</div>
                        <div><strong>Langsames Blinken:</strong> 1 Sekunde an, 1 Sekunde aus</div>
                        <div><strong>Schnelles Blinken:</strong> 200ms an, 200ms aus</div>
                        <div><strong>WLAN:</strong> Doppelblinken - Verbindung wird hergestellt</div>
                        <div><strong>Daten:</strong> Kurzes Aufblitzen - Signale werden gesendet/empfangen</div>
                        <div><strong>Pairing:</strong> 3x schnell blinken, Pause - Pairing-Modus aktiv</div>
                        <div><strong>Fehler:</strong> Langes An, kurzes Aus - Fehler aufgetreten</div>
                    </div>
                </div>
            </div>
            </div>
            
            <!-- System Sub-Tab -->
            <div id="subtab-settings-system" class="subtab-content" style="display:none">
            <div class="card">
                <h2>System-Informationen</h2>
                <div class="grid-2">
                    <div id="deviceInfo" style="font-size:0.85rem;color:#666;line-height:1.8">
                        Lade...
                    </div>
                    <div style="display:flex;flex-direction:column;gap:0.5rem">
                        <button class="btn btn-danger" onclick="rebootDevice()">🔄 System neustarten</button>
                        <button class="btn btn-warning" onclick="factoryReset()">⚠️ Werksreset</button>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Add Shade Modal -->
    <div id="addShadeModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><h3>Rolladen hinzufügen</h3></div>
            
            <!-- Schritt 1: Basis-Informationen -->
            <div id="addShadeStep1">
                <form id="addShadeForm" onsubmit="showAddShadeStep2(event)">
                    <div class="form-group">
                        <label>Name</label>
                        <input type="text" id="newShadeName" placeholder="z.B. Wohnzimmer" required>
                    </div>
                    <div class="form-group">
                        <label>Remote Adresse (Hex, z.B. A1B2C3) - Oder via Pairing ermitteln</label>
                        <input type="text" id="newShadeAddr" placeholder="A1B2C3" pattern="[0-9A-Fa-f]{6}">
                    </div>
                    <div class="grid-2">
                        <div class="form-group">
                            <label>Fahrzeit hoch (ms)</label>
                            <input type="number" id="newShadeUpTime" value="20000" min="1000" max="120000">
                        </div>
                        <div class="form-group">
                            <label>Fahrzeit runter (ms)</label>
                            <input type="number" id="newShadeDownTime" value="20000" min="1000" max="120000">
                        </div>
                    </div>
                    <div style="margin:1rem 0;padding:0.8rem;background:#e3f2fd;border-radius:4px;font-size:0.9rem">
                        <strong>💡 Tipp:</strong> Du kannst die Remote-Adresse entweder manuell eingeben oder durch Pairing automatisch ermitteln lassen.
                    </div>
                    <div class="modal-footer">
                        <button type="button" class="btn" onclick="closeModal('addShadeModal')">Abbrechen</button>
                        <button type="submit" class="btn btn-success" id="btnAddWithPairing">🔗 Mit Pairing fortfahren</button>
                        <button type="button" class="btn btn-primary" onclick="addShadeDirect()">✓ Direkt hinzufügen</button>
                    </div>
                </form>
            </div>
            
            <!-- Schritt 2: Pairing-Assistent -->
            <div id="addShadeStep2" style="display:none">
                <div class="form-group">
                    <label>Name (wird übernommen)</label>
                    <input type="text" id="pairingShadeName" readonly style="background:#f5f5f5">
                </div>
                
                <div id="addShadePairingProgress" class="pairing-container">
                    <div id="addStep1" class="pairing-step active">
                        <div class="step-number">1</div>
                        <div class="step-content">
                            <div class="step-title">PROG-Taste drücken</div>
                            <div class="step-desc">Halte die PROG-Taste an deiner Original-Fernbedienung <strong>3 Sekunden</strong> gedrückt, bis der Rolladen kurz juckelt.</div>
                        </div>
                    </div>
                    <div id="addStep2" class="pairing-step">
                        <div class="step-number">2</div>
                        <div class="step-content">
                            <div class="step-title">Signal erkannt</div>
                            <div class="step-desc">Die Adresse wurde erkannt! Drücke jetzt in Skyeo auf "Bestätigen".</div>
                        </div>
                    </div>
                    <div id="addStep3" class="pairing-step">
                        <div class="step-number">3</div>
                        <div class="step-content">
                            <div class="step-title">Bestätigen</div>
                            <div class="step-desc">Drücke nochmals kurz die PROG-Taste an der Fernbedienung um das Pairing zu bestätigen.</div>
                        </div>
                    </div>
                </div>
                
                <div id="addShadePairingStatus" style="margin:1rem 0;padding:0.8rem;background:#f5f5f5;border-radius:4px;font-size:0.9rem">
                    Bereit für Pairing...
                </div>
                
                <div id="addShadeAddressDisplay" style="display:none;margin:1rem 0;padding:0.8rem;background:#e8f5e9;border-radius:4px;font-size:0.9rem">
                    <strong>✓ Erkannte Adresse:</strong> <span id="detectedAddress">-</span>
                </div>
                
                <div class="modal-footer">
                    <button type="button" class="btn" onclick="cancelAddShadePairing()">❌ Abbrechen</button>
                    <button type="button" class="btn btn-primary" id="btnConfirmAddress" onclick="confirmDetectedAddress()" style="display:none">✓ Adresse übernehmen</button>
                </div>
            </div>
        </div>
    </div>

    <!-- Add Schedule Modal -->
    <div id="addScheduleModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><h3>Zeitplan erstellen</h3></div>
            <form onsubmit="addSchedule(event)">
                <div class="form-group">
                    <label>Rolladen</label>
                    <select id="scheduleShade" required></select>
                </div>
                <div class="grid-2">
                    <div class="form-group">
                        <label>Uhrzeit</label>
                        <input type="time" id="scheduleTime" required>
                    </div>
                    <div class="form-group">
                        <label>Aktion</label>
                        <select id="scheduleAction" required>
                            <option value="0">⬆️ Hoch</option>
                            <option value="1">⬇️ Runter</option>
                            <option value="2">⏹️ MY (Stop)</option>
                            <option value="3">🎯 Position</option>
                        </select>
                    </div>
                </div>
                <div class="form-group" id="scheduleTargetGroup" style="display:none">
                    <label>Zielposition (0-100%)</label>
                    <input type="number" id="scheduleTarget" min="0" max="100" value="50">
                </div>
                <div class="form-group">
                    <label>Wochentage</label>
                    <div style="display:flex;gap:0.5rem;flex-wrap:wrap">
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="0"> So</label>
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="1"> Mo</label>
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="2"> Di</label>
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="3"> Mi</label>
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="4"> Do</label>
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="5"> Fr</label>
                        <label style="display:flex;align-items:center;gap:0.3rem;font-weight:normal;font-size:0.9rem"><input type="checkbox" class="scheduleDay" value="6"> Sa</label>
                    </div>
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn" onclick="closeModal('addScheduleModal')">Abbrechen</button>
                    <button type="submit" class="btn btn-primary">Erstellen</button>
                </div>
            </form>
        </div>
    </div>

    <div id="toast" class="toast"></div>

    <script>
// Globale Variablen
let ws=null,shades=[],schedules=[],pairingInterval=null,logInterval=null,logs=[];
const WS_URL='ws://'+window.location.hostname+':8080';
const cmdNames={0:'UNKNOWN',1:'MY',2:'UP',3:'MY_UP',4:'DOWN',5:'MY_DOWN',6:'UP_DOWN',8:'PROG',9:'SUNFLAG',10:'FLAG'};

// Initialisierung
function init(){connectWebSocket();loadShades();loadSchedules();loadDeviceInfo();loadAntennaStatus();setupEventListeners();}

// Antenna Status laden und Warnung anzeigen
async function loadAntennaStatus(){
    try{
        const r=await fetch('/api/antenna/status');
        const status=await r.json();
        
        // Warnung anzeigen wenn nicht initialisiert
        const warningEl=document.getElementById('antennaWarning');
        if(warningEl){
            warningEl.style.display=status.initialized?'none':'block';
        }
        
        // Status Info in den Settings anzeigen
        const statusEl=document.getElementById('antennaStatusInfo');
        if(statusEl){
            if(status.initialized){
                statusEl.style.background='#e8f5e9';
                statusEl.style.borderLeft='4px solid #4CAF50';
                statusEl.innerHTML='<strong>✓ CC1101 verbunden</strong><br>Version: 0x'+status.versionHex+' | RSSI: '+status.rssi+' dBm | Frames: '+status.framesReceived+' empfangen / '+status.framesSent+' gesendet';
            }else{
                statusEl.style.background='#ffebee';
                statusEl.style.borderLeft='4px solid #f44336';
                statusEl.innerHTML='<strong>✗ CC1101 nicht verbunden</strong><br>Fehler: '+status.lastError+'<br>Prüfe die Verkabelung und ob die Pins korrekt konfiguriert sind.';
            }
        }
        
        // Aktuelle Pin-Werte laden
        loadAntennaConfig();
    }catch(e){
        console.error('Fehler beim Laden des Antennen-Status:',e);
    }
}

// Antenna Config laden
async function loadAntennaConfig(){
    try{
        const r=await fetch('/api/antenna/config');
        const config=await r.json();
        document.getElementById('pinSclk').value=config.sclk;
        document.getElementById('pinCsn').value=config.csn;
        document.getElementById('pinMosi').value=config.mosi;
        document.getElementById('pinMiso').value=config.miso;
        document.getElementById('pinTx').value=config.tx;
        document.getElementById('pinRx').value=config.rx;
    }catch(e){
        console.error('Fehler beim Laden der Antennen-Konfiguration:',e);
    }
}

// Verbindung testen
async function testAntennaConnection(){
    try{
        const r=await fetch('/api/antenna/status');
        const status=await r.json();
        if(status.initialized){
            showToast('✓ CC1101 verbunden! Version: 0x'+status.versionHex,'success');
        }else{
            showToast('✗ CC1101 nicht erkannt: '+status.lastError,'error');
        }
        loadAntennaStatus();
    }catch(e){
        showToast('Fehler beim Testen','error');
    }
}

// WebSocket
function connectWebSocket(){
    try{
        ws=new WebSocket(WS_URL);
        ws.onopen=()=>{updateConnectionStatus(true);showToast('Verbunden','success');};
        ws.onmessage=(e)=>{handleMessage(JSON.parse(e.data));};
        ws.onclose=()=>{updateConnectionStatus(false);setTimeout(connectWebSocket,3000);};
        ws.onerror=(e)=>{updateConnectionStatus(false);};
    }catch(e){updateConnectionStatus(false);}
}

function updateConnectionStatus(connected){
    const el=document.getElementById('connectionStatus');
    if(connected){el.innerHTML='<span class="status-indicator status-online"></span>Online';}
    else{el.innerHTML='<span class="status-indicator status-offline"></span>Offline';}
}

function handleMessage(msg){
    if(msg.type==='shadeUpdate'){updateShadeInList(msg.data);}
    else if(msg.type==='scheduleUpdate'){loadSchedules();}
    else if(msg.type==='logUpdate'){addLogEntry(msg.data);}
}

// Tabs
function showTab(tab){
    document.querySelectorAll('.tab-btn').forEach(t=>t.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(t=>t.classList.remove('active'));
    event.target.classList.add('active');
    document.getElementById('tab-'+tab).classList.add('active');
}

function showSubTab(tab){
    document.querySelectorAll('.subtab-btn').forEach(t=>t.classList.remove('active'));
    document.querySelectorAll('.subtab-content').forEach(t=>t.style.display='none');
    event.target.classList.add('active');
    document.getElementById('subtab-'+tab).style.display='block';
    
    // Automatisch Logs laden wenn Logs-Tab geöffnet wird
    if(tab === 'settings-logs'){
        startLogRefresh();
    } else {
        stopLogRefresh();
    }
    
    // LED Status laden wenn LED-Tab geöffnet wird
    if(tab === 'settings-led'){
        loadLEDStatus();
    }
}

// WLAN Scan mit scrollbarem Container
async function scanWifiNetworks(){
    const btn=document.getElementById('scanBtnText');
    const results=document.getElementById('wifiScanResults');
    btn.textContent='🔍 Suche...';
    results.style.display='none';
    results.innerHTML='<div style="padding:1rem;text-align:center;color:#666">Suche nach Netzwerken...</div>';
    results.style.display='block';
    
    try{
        const r=await fetch('/api/wifi/scan');
        const networks=await r.json();
        
        if(networks.length===0){
            results.innerHTML='<div style="padding:1rem;text-align:center;color:#666">Keine Netzwerke gefunden</div>';
            return;
        }
        
        // Sortiere nach Signalstärke
        networks.sort((a,b)=>b.rssi-a.rssi);
        
        let html='';
        networks.forEach(n=>{
            let signalClass='weak';
            let signalIcon='📶';
            if(n.rssi>=-50){signalClass='strong';signalIcon='📶';}
            else if(n.rssi>=-70){signalClass='medium';signalIcon='📶';}
            
            html+=`<div class="wifi-network" onclick="selectNetwork('${n.ssid.replace(/'/g,"\\'")}')">`;
            html+=`<div style="display:flex;align-items:center;gap:0.5rem">`;
            html+=`<span style="font-size:1.2rem">${signalIcon}</span>`;
            html+=`<div><div style="font-weight:600">${n.ssid}</div>`;
            html+=`<div style="font-size:0.75rem;color:#666">${n.encrypted?'🔒 Gesichert':'🔓 Offen'}</div></div>`;
            html+=`</div>`;
            html+=`<span class="wifi-signal ${signalClass}">${n.rssi} dBm</span>`;
            html+=`</div>`;
        });
        
        results.innerHTML=html;
    }catch(e){
        results.innerHTML='<div style="padding:1rem;text-align:center;color:#f44336">❌ Fehler beim Scannen</div>';
    }
    btn.textContent='📶 Nach Netzwerken suchen';
}

function selectNetwork(ssid){
    document.getElementById('wifiSsid').value=ssid;
    document.getElementById('wifiPass').focus();
    showToast('SSID ausgewählt: '+ssid,'success');
}

// Antennen-Config
function loadDefaultPins(){
    document.getElementById('pinSclk').value='18';
    document.getElementById('pinCsn').value='5';
    document.getElementById('pinMosi').value='23';
    document.getElementById('pinMiso').value='19';
    document.getElementById('pinTx').value='22';
    document.getElementById('pinRx').value='21';
    showToast('Standardwerte geladen','success');
}

async function saveAntennaConfig(){
    const config={
        sclk:parseInt(document.getElementById('pinSclk').value),
        csn:parseInt(document.getElementById('pinCsn').value),
        mosi:parseInt(document.getElementById('pinMosi').value),
        miso:parseInt(document.getElementById('pinMiso').value),
        tx:parseInt(document.getElementById('pinTx').value),
        rx:parseInt(document.getElementById('pinRx').value)
    };
    
    try{
        await fetch('/api/antenna/config',{
            method:'POST',
            headers:{'Content-Type':'application/json'},
            body:JSON.stringify(config)
        });
        showToast('Antennen-Konfiguration gespeichert. Gerät startet neu...','success');
    }catch(e){
        showToast('Fehler beim Speichern','error');
    }
}

// Logs - Antenne ist immer aktiv, wir zeigen nur die empfangenen Frames an
let logRefreshInterval = null;

function startLogRefresh() {
    if(logRefreshInterval) clearInterval(logRefreshInterval);
    logRefreshInterval = setInterval(loadAntennaLogs, 500);
    loadAntennaLogs(); // Sofort laden
}

function stopLogRefresh() {
    if(logRefreshInterval) {
        clearInterval(logRefreshInterval);
        logRefreshInterval = null;
    }
}

async function loadAntennaLogs(){
    try{
        const r=await fetch('/api/antenna/logs');
        if(!r.ok){
            console.error('Fehler beim Laden der Logs:', r.status);
            return;
        }
        
        const newLogs=await r.json();
        console.log('Empfangene Logs:', newLogs.length, newLogs);
        
        if(newLogs.length===0)return;
        
        const container=document.getElementById('antennaLogs');
        const wasAtBottom=container.scrollHeight-container.scrollTop<=container.clientHeight+50;
        
        let newCount = 0;
        newLogs.forEach(log=>{
            // Prüfe ob Log bereits existiert (basierend auf timestamp + rollingCode + address)
            const exists = logs.find(l=>l.timestamp===log.timestamp && 
                                       l.rollingCode===log.rollingCode &&
                                       l.remoteAddress===log.address);
            if(!exists){
                // Korrigiere Feldnamen falls nötig
                const normalizedLog = {
                    timestamp: log.timestamp,
                    remoteAddress: log.address || log.remoteAddress,
                    rollingCode: log.rollingCode,
                    command: log.command,
                    rssi: log.rssi,
                    direction: log.direction || 'RX',
                    valid: log.valid !== undefined ? log.valid : true
                };
                logs.push(normalizedLog);
                addLogEntry(normalizedLog);
                newCount++;
            }
        });
        
        // Max 100 Logs
        if(logs.length>100){
            logs=logs.slice(-100);
            container.innerHTML='';
            logs.forEach(l=>addLogEntry(l,false));
        }
        
        if(wasAtBottom && newCount > 0){
            container.scrollTop=container.scrollHeight;
        }
        
        document.getElementById('rxCount').textContent=logs.length;
        if(logs.length>0){
            const last=logs[logs.length-1];
            document.getElementById('lastRssi').textContent=last.rssi+' dBm';
            document.getElementById('lastRxTime').textContent=new Date(last.timestamp).toLocaleTimeString();
        }
    }catch(e){
        console.error('Fehler in loadAntennaLogs:', e);
    }
}

function addLogEntry(log,animate=true){
    const container=document.getElementById('antennaLogs');
    const cmdName=cmdNames[log.command]||'CMD'+log.command;
    const dir=log.direction||'RX';
    
    const entry=document.createElement('div');
    entry.className='log-entry';
    if(animate)entry.style.animation='fadeIn 0.3s';
    
    entry.innerHTML=`
        <span class="log-time">${new Date(log.timestamp).toLocaleTimeString('de-DE',{hour12:false,hour:'2-digit',minute:'2-digit',second:'2-digit'})}</span>
        <span class="log-direction ${dir.toLowerCase()}">${dir}</span>
        <span class="log-address">0x${log.remoteAddress.toString(16).toUpperCase().padStart(6,'0')}</span>
        <span class="log-cmd">${cmdName}</span>
        <span class="log-rc">RC:${log.rollingCode}</span>
        <span class="log-rssi">${log.rssi}dBm</span>
    `;
    
    container.appendChild(entry);
    if(container.children.length>100){
        container.removeChild(container.firstChild);
    }
}

function clearLogs(){
    logs=[];
    document.getElementById('antennaLogs').innerHTML='<div class="log-entry" style="color:#858585;font-style:italic">Logs gelöscht...</div>';
    document.getElementById('rxCount').textContent='0';
    document.getElementById('lastRssi').textContent='-';
    showToast('Logs gelöscht','success');
}

function exportLogs(){
    if(logs.length===0){
        showToast('Keine Logs zum Exportieren','error');
        return;
    }
    
    let csv='Time,Direction,Address,Command,RollingCode,RSSI\n';
    logs.forEach(l=>{
        csv+=`${new Date(l.timestamp).toISOString()},${l.direction||'RX'},0x${l.remoteAddress.toString(16)},${cmdNames[l.command]||l.command},${l.rollingCode},${l.rssi}\n`;
    });
    
    const blob=new Blob([csv],{type:'text/csv'});
    const url=URL.createObjectURL(blob);
    const a=document.createElement('a');
    a.href=url;
    a.download='skyeo-logs-'+new Date().toISOString().slice(0,10)+'.csv';
    a.click();
    showToast('Logs exportiert','success');
}

// Pairing
function updatePairingShadeSelect(){
    const sel=document.getElementById('pairingShadeSelect');
    if(shades.length===0){
        sel.innerHTML='<option value="">Keine Rolläden vorhanden</option>';
        return;
    }
    sel.innerHTML=shades.map(s=>`<option value="${s.id}">${s.name} (0x${s.remoteAddress.toString(16).toUpperCase()})</option>`).join('');
}

async function startPairing(){
    const shadeId=document.getElementById('pairingShadeSelect').value;
    if(!shadeId){showToast('Bitte einen Rolladen auswählen','error');return;}
    
    try{
        await fetch('/api/pairing/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({shadeId:parseInt(shadeId)})});
        document.getElementById('pairingIntro').style.display='none';
        document.getElementById('pairingProgress').style.display='block';
        document.getElementById('step1').classList.add('active');
        showToast('Pairing gestartet','success');
        
        if(pairingInterval)clearInterval(pairingInterval);
        pairingInterval=setInterval(updatePairingStatus,1000);
        
        // Antenne ist immer aktiv - kein Start nötig
        showToast('Pairing gestartet - drücke PROG an der Fernbedienung','success');
    }catch(e){showToast('Fehler beim Starten','error');}
}

async function stopPairing(){
    try{
        await fetch('/api/pairing/stop',{method:'POST'});
        document.getElementById('pairingIntro').style.display='block';
        document.getElementById('pairingProgress').style.display='none';
        document.querySelectorAll('.pairing-step').forEach(el=>{el.classList.remove('active','completed','error');});
        if(pairingInterval)clearInterval(pairingInterval);
        showToast('Pairing gestoppt','success');
    }catch(e){showToast('Fehler','error');}
}

async function updatePairingStatus(){
    try{
        const r=await fetch('/api/pairing/status');
        const status=await r.json();
        
        document.getElementById('pairingStatus').innerHTML='<strong>Status:</strong> '+status.instructions;
        
        const steps=['step1','step2','step3'];
        steps.forEach((id,idx)=>{
            const el=document.getElementById(id);
            el.classList.remove('active','completed');
            if(idx+1<status.step)el.classList.add('completed');
            else if(idx+1===status.step)el.classList.add('active');
        });
        
        if(status.step===4){
            setTimeout(()=>{stopPairing();showToast('✓ Pairing erfolgreich!','success');},2000);
        }else if(status.step===5){
            document.querySelectorAll('.pairing-step').forEach(el=>el.classList.add('error'));
        }
    }catch(e){}
}

// Bestehende Funktionen (vereinfacht)
async function loadShades(){try{const r=await fetch('/api/shades');shades=await r.json();renderShades();updateScheduleShadeSelect();updatePairingShadeSelect();}catch(e){}}
function renderShades(){const list=document.getElementById('shadesList');if(shades.length===0){list.innerHTML='<div class="empty-state">Keine Rolläden konfiguriert</div>';return;}list.innerHTML=shades.map(s=>`<div class="shade-item"><div><strong>${s.name}</strong><br><small style="color:#666">Pos: ${s.position}%${s.moving?' (fährt)':''} | Addr: 0x${s.remoteAddress.toString(16).toUpperCase()}</small></div><div class="shade-controls"><button class="btn btn-sm btn-success" onclick="sendCommand(${s.id},'up')">⬆️</button><button class="btn btn-sm" onclick="sendCommand(${s.id},'my')" title="MY/Stop">⏹️</button><button class="btn btn-sm btn-danger" onclick="sendCommand(${s.id},'down')">⬇️</button><input type="range" class="position-slider" min="0" max="100" value="${s.position}" onchange="setPosition(${s.id},this.value)" title="Position"><button class="btn btn-sm" onclick="deleteShade(${s.id})" title="Löschen">🗑️</button></div></div>`).join('');}
async function sendCommand(id,cmd){try{await fetch('/api/shades/command',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({id,cmd})});showToast('Befehl gesendet','success');}catch(e){showToast('Fehler','error');}}
async function setPosition(id,pos){try{await fetch('/api/shades/command',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({id,cmd:'target',target:parseInt(pos)})});showToast('Position: '+pos+'%','success');}catch(e){showToast('Fehler','error');}}
// Add Shade mit Pairing Integration
let addShadeTempData = null;
let addShadePairingInterval = null;

function showAddShadeStep2(e) {
    e.preventDefault();
    const name = document.getElementById('newShadeName').value;
    const addr = document.getElementById('newShadeAddr').value;
    const upTime = parseInt(document.getElementById('newShadeUpTime').value);
    const downTime = parseInt(document.getElementById('newShadeDownTime').value);
    
    // Speichere temporär
    addShadeTempData = { name, addr, upTime, downTime };
    
    // Zeige Schritt 2
    document.getElementById('addShadeStep1').style.display = 'none';
    document.getElementById('addShadeStep2').style.display = 'block';
    document.getElementById('pairingShadeName').value = name;
    
    // Starte automatisch Empfang
    startAddShadePairing();
}

async function addShadeDirect() {
    const name = document.getElementById('newShadeName').value;
    const addr = document.getElementById('newShadeAddr').value;
    
    if (!name || !addr) {
        showToast('Bitte Name und Adresse eingeben', 'error');
        return;
    }
    
    const upTime = parseInt(document.getElementById('newShadeUpTime').value);
    const downTime = parseInt(document.getElementById('newShadeDownTime').value);
    
    try {
        await fetch('/api/shades', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({name, remoteAddress: parseInt(addr, 16), upTime, downTime})
        });
        closeModal('addShadeModal');
        loadShades();
        showToast('Rolladen hinzugefügt', 'success');
        resetAddShadeForm();
    } catch(e) {
        showToast('Fehler beim Hinzufügen', 'error');
    }
}

async function startAddShadePairing() {
    document.getElementById('addShadePairingStatus').textContent = 'Warte auf PROG-Signal... Die Antenne ist aktiv.';
    
    // Antenne ist immer aktiv - kein explizites Aktivieren nötig
    
    // Starte Intervall zum Prüfen auf empfangene Frames
    if (addShadePairingInterval) clearInterval(addShadePairingInterval);
    addShadePairingInterval = setInterval(checkForPairingSignal, 500);
}

async function checkForPairingSignal() {
    try {
        const r = await fetch('/api/antenna/logs');
        const logs = await r.json();
        
        if (logs.length > 0) {
            // Nimm das neueste Log
            const lastLog = logs[logs.length - 1];
            
            if (lastLog.command === 0x8) { // PROG Befehl
                // Adresse gefunden!
                const detectedAddr = '0x' + lastLog.remoteAddress.toString(16).toUpperCase().padStart(6, '0');
                document.getElementById('detectedAddress').textContent = detectedAddr;
                document.getElementById('addShadeAddressDisplay').style.display = 'block';
                document.getElementById('btnConfirmAddress').style.display = 'inline-block';
                
                // Speichere die erkannte Adresse
                addShadeTempData.detectedAddr = lastLog.remoteAddress;
                
                // Aktualisiere Status
                document.getElementById('addShadePairingStatus').innerHTML = '<strong>✓ Adresse erkannt!</strong> Klicke auf "Adresse übernehmen" um fortzufahren.';
                
                // Markiere Schritt 2 als abgeschlossen
                document.getElementById('addStep1').classList.remove('active');
                document.getElementById('addStep1').classList.add('completed');
                document.getElementById('addStep2').classList.add('active');
                
                // Stoppe Intervall
                if (addShadePairingInterval) {
                    clearInterval(addShadePairingInterval);
                    addShadePairingInterval = null;
                }
            }
        }
    } catch(e) {
        console.error('Fehler beim Prüfen auf Pairing-Signal:', e);
    }
}

async function confirmDetectedAddress() {
    if (!addShadeTempData || !addShadeTempData.detectedAddr) {
        showToast('Keine Adresse erkannt!', 'error');
        return;
    }
    
    try {
        await fetch('/api/shades', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
                name: addShadeTempData.name,
                remoteAddress: addShadeTempData.detectedAddr,
                upTime: addShadeTempData.upTime,
                downTime: addShadeTempData.downTime
            })
        });
        
        // Antenne bleibt aktiv
        
        closeModal('addShadeModal');
        loadShades();
        showToast('✓ Rolladen erfolgreich gepairt!', 'success');
        resetAddShadeForm();
    } catch(e) {
        showToast('Fehler beim Hinzufügen', 'error');
    }
}

function cancelAddShadePairing() {
    // Stoppe Intervall
    if (addShadePairingInterval) {
        clearInterval(addShadePairingInterval);
        addShadePairingInterval = null;
    }
    
    // Antenne bleibt aktiv - kein Deaktivieren nötig
    
    // Zurück zu Schritt 1
    document.getElementById('addShadeStep2').style.display = 'none';
    document.getElementById('addShadeStep1').style.display = 'block';
    
    // Reset Status
    document.getElementById('addShadeAddressDisplay').style.display = 'none';
    document.getElementById('btnConfirmAddress').style.display = 'none';
    document.getElementById('addShadePairingStatus').textContent = 'Bereit für Pairing...';
    
    // Reset Steps
    document.getElementById('addStep1').classList.add('active');
    document.getElementById('addStep1').classList.remove('completed');
    document.getElementById('addStep2').classList.remove('active', 'completed');
    document.getElementById('addStep3').classList.remove('active', 'completed');
}

function resetAddShadeForm() {
    document.getElementById('addShadeForm').reset();
    document.getElementById('addShadeStep2').style.display = 'none';
    document.getElementById('addShadeStep1').style.display = 'block';
    document.getElementById('addShadeAddressDisplay').style.display = 'none';
    document.getElementById('btnConfirmAddress').style.display = 'none';
    document.getElementById('addShadePairingStatus').textContent = 'Bereit für Pairing...';
    
    // Reset Steps
    document.getElementById('addStep1').classList.add('active');
    document.getElementById('addStep1').classList.remove('completed');
    document.getElementById('addStep2').classList.remove('active', 'completed');
    document.getElementById('addStep3').classList.remove('active', 'completed');
    
    addShadeTempData = null;
}

// Alte Funktion für Kompatibilität
async function addShade(e) {
    e.preventDefault();
    await addShadeDirect();
}
async function deleteShade(id){if(!confirm('Rolladen wirklich löschen?'))return;try{await fetch('/api/shades/'+id,{method:'DELETE'});loadShades();showToast('Gelöscht','success');}catch(e){showToast('Fehler','error');}}
async function loadSchedules(){try{const r=await fetch('/api/schedules');schedules=await r.json();renderSchedules();}catch(e){}}
function renderSchedules(){const list=document.getElementById('schedulesList');if(schedules.length===0){list.innerHTML='<div class="empty-state">Keine Zeitpläne erstellt</div>';return;}const days=['So','Mo','Di','Mi','Do','Fr','Sa'];list.innerHTML=schedules.map(s=>`<div class="shade-item"><div><strong>${String(s.hour).padStart(2,'0')}:${String(s.minute).padStart(2,'0')}</strong> - ${s.shadeName}<br><small style="color:#666">${days.filter((d,i)=>s.days&(1<<i)).join(', ')}</small></div><button class="btn btn-sm" onclick="deleteSchedule(${s.id})" title="Löschen">🗑️</button></div>`).join('');}
function updateScheduleShadeSelect(){const sel=document.getElementById('scheduleShade');sel.innerHTML=shades.map(s=>`<option value="${s.id}">${s.name}</option>`).join('');}
async function addSchedule(e){e.preventDefault();const shadeId=parseInt(document.getElementById('scheduleShade').value);const time=document.getElementById('scheduleTime').value.split(':');const hour=parseInt(time[0]);const minute=parseInt(time[1]);const command=parseInt(document.getElementById('scheduleAction').value);const target=command===3?parseInt(document.getElementById('scheduleTarget').value):0;let days=0;document.querySelectorAll('.scheduleDay:checked').forEach(cb=>{days|=1<<parseInt(cb.value);});try{await fetch('/api/schedules',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({shadeId,hour,minute,command,target,days})});closeModal('addScheduleModal');loadSchedules();showToast('Zeitplan erstellt','success');}catch(e){showToast('Fehler','error');}}
async function deleteSchedule(id){if(!confirm('Zeitplan löschen?'))return;try{await fetch('/api/schedules/'+id,{method:'DELETE'});loadSchedules();showToast('Gelöscht','success');}catch(e){showToast('Fehler','error');}}
async function saveWifi(e){e.preventDefault();const ssid=document.getElementById('wifiSsid').value;const password=document.getElementById('wifiPass').value;try{await fetch('/api/config/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password})});showToast('WLAN gespeichert. Gerät startet neu...','success');setTimeout(()=>location.reload(),5000);}catch(e){showToast('Fehler','error');}}
async function loadDeviceInfo(){try{const r=await fetch('/api/info');const info=await r.json();document.getElementById('deviceInfo').innerHTML=`<strong>Skyeo v${info.version}</strong><br>📡 ${info.shades}/${info.maxShades} Rolläden<br>🌐 IP: ${info.ip}<br>🔌 MAC: ${info.mac}<br>📶 Signal: ${info.rssi} dBm<br>⏰ ${info.schedules} Zeitpläne<br>🔄 Modus: ${info.apMode?'AP':'Station'}`;}catch(e){}}
async function rebootDevice(){if(!confirm('System wirklich neustarten?'))return;try{await fetch('/api/reboot',{method:'POST'});showToast('System startet neu...','success');}catch(e){showToast('Fehler','error');}}
async function factoryReset(){if(!confirm('⚠️ ALLE Daten werden gelöscht! Fortfahren?'))return;try{await fetch('/api/reset',{method:'POST'});showToast('Werksreset durchgeführt','success');setTimeout(()=>location.reload(),3000);}catch(e){showToast('Fehler','error');}}
function showModal(id){document.getElementById(id).classList.add('active');}
function closeModal(id){document.getElementById(id).classList.remove('active');}
function showAddShadeModal(){showModal('addShadeModal');}
function showAddScheduleModal(){showModal('addScheduleModal');}
function showToast(msg,type='info'){const t=document.getElementById('toast');t.textContent=msg;t.className='toast '+type;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),4000);}
function updateShadeInList(shade){const idx=shades.findIndex(s=>s.id===shade.id);if(idx!==-1){shades[idx]=shade;renderShades();}}
function setupEventListeners(){document.getElementById('scheduleAction').addEventListener('change',function(){document.getElementById('scheduleTargetGroup').style.display=this.value==='3'?'block':'none';});}

// LED Control Functions
async function loadLEDStatus() {
    try {
        const r = await fetch('/api/led/status');
        const status = await r.json();
        
        // Update UI
        document.getElementById('ledBrightness').value = status.brightness;
        document.getElementById('ledBrightnessValue').textContent = status.brightness;
        document.getElementById('ledPatternSelect').value = status.currentPattern;
        
        // Update status info
        const statusEl = document.getElementById('ledStatusInfo');
        if (status.enabled) {
            statusEl.style.background = '#e8f5e9';
            statusEl.style.borderLeft = '4px solid #4CAF50';
            statusEl.innerHTML = `<strong>✓ LED aktiviert</strong><br>Muster: ${status.currentPattern} | Helligkeit: ${status.brightness}/255`;
        } else {
            statusEl.style.background = '#ffebee';
            statusEl.style.borderLeft = '4px solid #f44336';
            statusEl.innerHTML = '<strong>✗ LED deaktiviert</strong><br>Die Debug-LED ist ausgeschaltet';
        }
    } catch(e) {
        console.error('Fehler beim Laden des LED-Status:', e);
    }
}

async function ledOn() {
    try {
        await fetch('/api/led/on', {method: 'POST'});
        showToast('LED aktiviert', 'success');
        loadLEDStatus();
    } catch(e) {
        showToast('Fehler beim Aktivieren', 'error');
    }
}

async function ledOff() {
    try {
        await fetch('/api/led/off', {method: 'POST'});
        showToast('LED deaktiviert', 'success');
        loadLEDStatus();
    } catch(e) {
        showToast('Fehler beim Deaktivieren', 'error');
    }
}

async function ledSetBrightness(value) {
    document.getElementById('ledBrightnessValue').textContent = value;
    try {
        await fetch('/api/led/brightness', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({brightness: parseInt(value)})
        });
    } catch(e) {
        showToast('Fehler beim Setzen der Helligkeit', 'error');
    }
}

async function ledSetPattern(pattern) {
    try {
        await fetch('/api/led/pattern', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({pattern: pattern})
        });
        showToast('Muster gesetzt: ' + pattern, 'success');
        loadLEDStatus();
    } catch(e) {
        showToast('Fehler beim Setzen des Musters', 'error');
    }
}

function showLEDPatternInfo() {
    const infoEl = document.getElementById('ledPatternInfo');
    if (infoEl.style.display === 'none') {
        infoEl.style.display = 'block';
    } else {
        infoEl.style.display = 'none';
    }
}

document.addEventListener('DOMContentLoaded',init);
    </script>
</body>
</html>
)rawliteral";

const char* WebUI::getIndexHTML() {
    return INDEX_HTML;
}

size_t WebUI::getIndexHTMLSize() {
    return strlen_P((const char*)INDEX_HTML);
}

const char* WebUI::getMainCSS() {
    return "";
}

size_t WebUI::getMainCSSSize() {
    return 0;
}

const char* WebUI::getMainJS() {
    return "";
}

size_t WebUI::getMainJSSize() {
    return 0;
}
