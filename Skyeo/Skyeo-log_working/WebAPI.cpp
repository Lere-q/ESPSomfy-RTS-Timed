#include "WebAPI.h"
#include "WebUI.h"
#include "Config.h"
#include "Somfy.h"
#include "Scheduler.h"
#include "Network.h"
#include "Utils.h"

// ============================================
// SKYEO - Web API Implementation
// ============================================

WebServer WebAPI::server(80);
WebSocketsServer WebAPI::ws(8080);
bool WebAPI::wsEnabled = false;

void WebAPI::begin() {
    // HTTP Endpoints
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/shades", HTTP_GET, handleShades);
    server.on("/api/shades", HTTP_POST, handleShadeCreate);
    server.on("/api/shades/command", HTTP_POST, handleShadeCommand);
    server.on("/api/shades/", HTTP_DELETE, handleShadeDelete);
    server.on("/api/schedules", HTTP_GET, handleSchedules);
    server.on("/api/schedules", HTTP_POST, handleScheduleCreate);
    server.on("/api/schedules/", HTTP_DELETE, handleScheduleDelete);
    server.on("/api/config/wifi", HTTP_POST, handleWifiConfig);
    server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
    server.on("/api/info", HTTP_GET, handleDeviceInfo);
    server.on("/api/reboot", HTTP_POST, handleReboot);
    
    // Antenna / Receiver
    server.on("/api/antenna/enable", HTTP_POST, handleAntennaEnable);
    server.on("/api/antenna/disable", HTTP_POST, handleAntennaDisable);
    server.on("/api/antenna/logs", HTTP_GET, handleAntennaLogs);
    server.on("/api/antenna/status", HTTP_GET, handleAntennaStatus);
    server.on("/api/antenna/config", HTTP_GET, handleAntennaConfig);
    server.on("/api/antenna/config", HTTP_POST, handleAntennaSaveConfig);
    
    // Pairing
    server.on("/api/pairing/start", HTTP_POST, handlePairingStart);
    server.on("/api/pairing/stop", HTTP_POST, handlePairingStop);
    server.on("/api/pairing/status", HTTP_GET, handlePairingStatus);
    
    // LED Control
    server.on("/api/led/status", HTTP_GET, handleLEDStatus);
    server.on("/api/led/on", HTTP_POST, handleLEDOn);
    server.on("/api/led/off", HTTP_POST, handleLEDOff);
    server.on("/api/led/pattern", HTTP_POST, handleLEDSetPattern);
    server.on("/api/led/brightness", HTTP_POST, handleLEDSetBrightness);
    server.on("/api/led/patterns", HTTP_GET, handleLEDPatternsInfo);
    
    server.onNotFound(handleNotFound);
    
    // CORS für API
    server.on("/api/", HTTP_OPTIONS, handleCORS);
    
    server.begin();
    DEBUG_PRINTLN("[SKYEO] HTTP Server gestartet (Port 80)");
    
    // WebSocket
    ws.begin();
    ws.onEvent(handleWebSocketEvent);
    wsEnabled = true;
    DEBUG_PRINTLN("[SKYEO] WebSocket Server gestartet (Port 8080)");
}

void WebAPI::loop() {
    server.handleClient();
    if (wsEnabled) {
        ws.loop();
    }
}

// ============================================
// HTTP Handlers
// ============================================

void WebAPI::handleRoot() {
    server.sendHeader("Content-Type", "text/html");
    server.send_P(200, "text/html", WebUI::getIndexHTML(), WebUI::getIndexHTMLSize());
}

void WebAPI::handleShades() {
    String json = "[";
    bool first = true;
    for (uint8_t i = 0; i < MAX_SHADES; i++) {
        ShadeConfig cfg;
        if (Config::loadShade(i, cfg)) {
            ShadeState* state = Somfy.getShade(i);
            if (!first) json += ",";
            first = false;
            
            json += "{";
            json += "\"id\":" + String(cfg.id) + ",";
            json += "\"name\":\"" + jsonEscape(String(cfg.name)) + "\",";
            json += "\"position\":" + String(state ? state->position : 50) + ",";
            json += "\"target\":" + String(state ? state->target : 50) + ",";
            json += "\"moving\":" + String(state && state->moving ? "true" : "false") + ",";
            json += "\"remoteAddress\":\"" + String(cfg.remoteAddress, HEX) + "\",";
            json += "\"upTime\":" + String(cfg.upTime) + ",";
            json += "\"downTime\":" + String(cfg.downTime);
            json += "}";
        }
    }
    json += "]";
    sendJSON(200, json);
}

void WebAPI::handleShadeCreate() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    
    // Simple JSON Parsing
    String name = "";
    uint32_t remoteAddress = 0;
    uint16_t upTime = 20000;
    uint16_t downTime = 20000;
    
    // Parse name
    int nameStart = body.indexOf("\"name\":\"") + 8;
    int nameEnd = body.indexOf("\"", nameStart);
    if (nameStart > 7 && nameEnd > nameStart) {
        name = body.substring(nameStart, nameEnd);
    }
    
    // Parse address
    int addrStart = body.indexOf("\"remoteAddress\":\"") + 17;
    int addrEnd = body.indexOf("\"", addrStart);
    if (addrStart > 16 && addrEnd > addrStart) {
        remoteAddress = strtoul(body.substring(addrStart, addrEnd).c_str(), NULL, 16);
    }
    
    // Parse times
    int upStart = body.indexOf("\"upTime\":") + 9;
    if (upStart > 8) {
        upTime = body.substring(upStart).toInt();
    }
    
    int downStart = body.indexOf("\"downTime\":") + 11;
    if (downStart > 10) {
        downTime = body.substring(downStart).toInt();
    }
    
    // Finde freie ID
    uint8_t id = 255;
    for (uint8_t i = 0; i < MAX_SHADES; i++) {
        ShadeConfig cfg;
        if (!Config::loadShade(i, cfg)) {
            id = i;
            break;
        }
    }
    
    if (id == 255) {
        sendError(400, "Maximale Anzahl an Rolläden erreicht");
        return;
    }
    
    if (Somfy.addShade(id, name.c_str(), remoteAddress, upTime, downTime)) {
        broadcastShadeUpdate(id);
        sendJSON(200, "{\"success\":true,\"id\":" + String(id) + "}");
    } else {
        sendError(500, "Fehler beim Hinzufügen");
    }
}

void WebAPI::handleShadeDelete() {
    String uri = server.uri();
    int lastSlash = uri.lastIndexOf('/');
    if (lastSlash < 0) {
        sendError(400, "Ungültige ID");
        return;
    }
    
    uint8_t id = uri.substring(lastSlash + 1).toInt();
    
    if (id >= MAX_SHADES) {
        sendError(400, "Ungültige ID");
        return;
    }
    
    if (Somfy.removeShade(id)) {
        broadcastShadeUpdate(id);
        sendJSON(200, "{\"success\":true}");
    } else {
        sendError(404, "Rolladen nicht gefunden");
    }
}

void WebAPI::handleShadeCommand() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    
    // Parse ID
    int idStart = body.indexOf("\"id\":") + 5;
    uint8_t id = body.substring(idStart).toInt();
    
    // Parse command
    int cmdStart = body.indexOf("\"cmd\":\"") + 7;
    int cmdEnd = body.indexOf("\"", cmdStart);
    String cmd = body.substring(cmdStart, cmdEnd);
    
    ShadeState* shade = Somfy.getShade(id);
    if (!shade) {
        sendError(404, "Rolladen nicht gefunden");
        return;
    }
    
    bool success = false;
    
    if (cmd == "up") {
        success = Somfy.sendCommand(id, SOMFY_UP);
    } else if (cmd == "down") {
        success = Somfy.sendCommand(id, SOMFY_DOWN);
    } else if (cmd == "my") {
        success = Somfy.sendCommand(id, SOMFY_MY);
    } else if (cmd == "stop") {
        Somfy.stop(id);
        success = true;
    } else if (cmd == "target") {
        int targetStart = body.indexOf("\"target\":") + 9;
        uint8_t target = body.substring(targetStart).toInt();
        success = Somfy.sendPosition(id, target);
    }
    
    if (success) {
        broadcastShadeUpdate(id);
        sendJSON(200, "{\"success\":true}");
    } else {
        sendError(500, "Befehl fehlgeschlagen");
    }
}

void WebAPI::handleSchedules() {
    String json = "[";
    bool first = true;
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        Schedule sched;
        if (Config::loadSchedule(i, sched)) {
            if (!first) json += ",";
            first = false;
            
            ShadeState* shade = Somfy.getShade(sched.shadeId);
            
            json += "{";
            json += "\"id\":" + String(sched.id) + ",";
            json += "\"shadeId\":" + String(sched.shadeId) + ",";
            json += "\"shadeName\":\"" + (shade ? jsonEscape(String(shade->name)) : String("Unbekannt")) + "\",";
            json += "\"hour\":" + String(sched.hour) + ",";
            json += "\"minute\":" + String(sched.minute) + ",";
            json += "\"days\":" + String(sched.days) + ",";
            json += "\"command\":" + String(sched.command) + ",";
            json += "\"target\":" + String(sched.target) + ",";
            json += "\"enabled\":" + String(sched.enabled ? "true" : "false");
            json += "}";
        }
    }
    json += "]";
    sendJSON(200, json);
}

void WebAPI::handleScheduleCreate() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    
    // Parse schedule data
    Schedule sched;
    sched.enabled = true;
    sched.lastRun = 0;
    
    int shadeStart = body.indexOf("\"shadeId\":") + 10;
    sched.shadeId = body.substring(shadeStart).toInt();
    
    int hourStart = body.indexOf("\"hour\":") + 7;
    sched.hour = body.substring(hourStart).toInt();
    
    int minStart = body.indexOf("\"minute\":") + 9;
    sched.minute = body.substring(minStart).toInt();
    
    int cmdStart = body.indexOf("\"command\":") + 10;
    sched.command = body.substring(cmdStart).toInt();
    
    int targetStart = body.indexOf("\"target\":") + 9;
    sched.target = body.substring(targetStart).toInt();
    
    int daysStart = body.indexOf("\"days\":") + 7;
    sched.days = body.substring(daysStart).toInt();
    
    if (Scheduler::addSchedule(sched)) {
        sendJSON(200, "{\"success\":true}");
    } else {
        sendError(500, "Fehler beim Erstellen");
    }
}

void WebAPI::handleScheduleDelete() {
    String uri = server.uri();
    int lastSlash = uri.lastIndexOf('/');
    if (lastSlash < 0) {
        sendError(400, "Ungültige ID");
        return;
    }
    
    uint8_t id = uri.substring(lastSlash + 1).toInt();
    
    if (Scheduler::deleteSchedule(id)) {
        sendJSON(200, "{\"success\":true}");
    } else {
        sendError(404, "Zeitplan nicht gefunden");
    }
}

void WebAPI::handleWifiConfig() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    
    // Parse SSID and password
    int ssidStart = body.indexOf("\"ssid\":\"") + 8;
    int ssidEnd = body.indexOf("\"", ssidStart);
    String ssid = body.substring(ssidStart, ssidEnd);
    
    int passStart = body.indexOf("\"password\":\"") + 12;
    int passEnd = body.indexOf("\"", passStart);
    String password = body.substring(passStart, passEnd);
    
    WiFiConfig wifi;
    strncpy(wifi.ssid, ssid.c_str(), sizeof(wifi.ssid));
    strncpy(wifi.password, password.c_str(), sizeof(wifi.password));
    wifi.dhcp = true;
    wifi.configured = true;
    
    if (Config::saveWiFi(wifi)) {
        sendJSON(200, "{\"success\":true,\"message\":\"WLAN gespeichert. Gerät startet neu...\"}");
        delay(2000);
        ESP.restart();
    } else {
        sendError(500, "Fehler beim Speichern");
    }
}

void WebAPI::handleWifiScan() {
    int n = Network::scanNetworks();
    
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + jsonEscape(Network::getSSID(i)) + "\",";
        json += "\"rssi\":" + String(Network::getNetworkRSSI(i)) + ",";
        json += "\"encrypted\":" + String(Network::getNetworkEncryption(i) != WIFI_AUTH_OPEN ? "true" : "false");
        json += "}";
    }
    json += "]";
    
    sendJSON(200, json);
}

void WebAPI::handleDeviceInfo() {
    String json = "{";
    json += "\"name\":\"Skyeo\",";
    json += "\"version\":\"" + String(SKYEO_VERSION) + "\",";
    json += "\"shades\":" + String(Somfy.getShadeCount()) + ",";
    json += "\"maxShades\":" + String(MAX_SHADES) + ",";
    json += "\"ip\":\"" + Network::getIPAddress() + "\",";
    json += "\"mac\":\"" + Network::getMAC() + "\",";
    json += "\"rssi\":" + String(Network::getRSSI()) + ",";
    json += "\"schedules\":" + String(Scheduler::getScheduleCount()) + ",";
    json += "\"apMode\":" + String(Network::isAPMode() ? "true" : "false");
    json += "}";
    sendJSON(200, json);
}

void WebAPI::handleReboot() {
    sendJSON(200, "{\"success\":true,\"message\":\"Gerät startet neu...\"}");
    delay(1000);
    ESP.restart();
}

void WebAPI::handleNotFound() {
    if (server.uri().startsWith("/api/")) {
        sendError(404, "API Endpoint nicht gefunden");
    } else {
        server.sendHeader("Content-Type", "text/html");
        server.send_P(200, "text/html", WebUI::getIndexHTML(), WebUI::getIndexHTMLSize());
    }
}

void WebAPI::handleCORS() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200);
}

void WebAPI::sendJSON(int code, const String& json) {
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(code, "application/json", json);
}

void WebAPI::sendError(int code, const String& message) {
    String json = "{\"success\":false,\"error\":\"" + jsonEscape(message) + "\"}";
    sendJSON(code, json);
}

// ============================================
// WebSocket Handlers
// ============================================

void WebAPI::handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_PRINTF("[SKYEO] WS Client %d getrennt\n", num);
            break;
        case WStype_CONNECTED:
            DEBUG_PRINTF("[SKYEO] WS Client %d verbunden\n", num);
            // Send initial data
            break;
        case WStype_TEXT:
            // Handle incoming messages if needed
            break;
        default:
            break;
    }
}

void WebAPI::broadcastMessage(const String& msg) {
    if (wsEnabled) {
        String mutableMsg = msg;
        ws.broadcastTXT(mutableMsg);
    }
}

void WebAPI::broadcastShadeUpdate(uint8_t shadeId) {
    ShadeState* state = Somfy.getShade(shadeId);
    if (!state) return;
    
    String json = "{\"type\":\"shadeUpdate\",\"data\":{";
    json += "\"id\":" + String(state->id) + ",";
    json += "\"position\":" + String(state->position) + ",";
    json += "\"target\":" + String(state->target) + ",";
    json += "\"moving\":" + String(state->moving ? "true" : "false") + ",";
    json += "\"direction\":" + String(state->direction);
    json += "}}";
    
    broadcastMessage(json);
}

void WebAPI::broadcastScheduleUpdate(uint8_t scheduleId) {
    String json = "{\"type\":\"scheduleUpdate\",\"data\":{";
    json += "\"id\":" + String(scheduleId);
    json += "}}";
    
    broadcastMessage(json);
}

// Antenna / Receiver Handlers
void WebAPI::handleAntennaEnable() {
    Somfy.enableReceive();
    sendJSON(200, "{\"success\":true,\"message\":\"Empfangsmodus aktiviert\"}");
}

void WebAPI::handleAntennaDisable() {
    Somfy.disableReceive();
    sendJSON(200, "{\"success\":true,\"message\":\"Empfangsmodus deaktiviert\"}");
}

void WebAPI::handleAntennaLogs() {
    auto frames = Somfy.getReceivedFrames();
    
    String json = "[";
    bool first = true;
    for (auto &frame : frames) {
        if (!first) json += ",";
        first = false;
        
        json += "{";
        json += "\"timestamp\":" + String(frame.timestamp) + ",";
        json += "\"address\":\"" + String(frame.remoteAddress, HEX) + "\",";
        json += "\"rollingCode\":" + String(frame.rollingCode) + ",";
        json += "\"command\":" + String(frame.command) + ",";
        json += "\"rssi\":" + String(frame.rssi) + ",";
        json += "\"direction\":\"" + String(frame.direction) + "\",";
        json += "\"valid\":" + String(frame.valid ? "true" : "false");
        json += "}";
    }
    json += "]";
    
    sendJSON(200, json);
}

void WebAPI::handleAntennaStatus() {
    AntennaStatus status = Somfy.getAntennaStatus();
    
    String json = "{";
    json += "\"initialized\":" + String(status.initialized ? "true" : "false") + ",";
    json += "\"version\":" + String(status.version) + ",";
    json += "\"versionHex\":\"0x" + String(status.version, HEX) + "\",";
    json += "\"rssi\":" + String(status.rssi) + ",";
    json += "\"receiveMode\":" + String(status.receiveMode ? "true" : "false") + ",";
    json += "\"framesReceived\":" + String(status.framesReceived) + ",";
    json += "\"framesSent\":" + String(status.framesSent) + ",";
    json += "\"frequency\":\"433.42 MHz\",";
    json += "\"lastError\":\"" + jsonEscape(String(status.lastError)) + "\"";
    json += "}";
    
    sendJSON(200, json);
}

void WebAPI::handleAntennaConfig() {
    uint8_t sclk, csn, mosi, miso, tx, rx;
    Somfy.getPins(sclk, csn, mosi, miso, tx, rx);
    
    String json = "{";
    json += "\"sclk\":" + String(sclk) + ",";
    json += "\"csn\":" + String(csn) + ",";
    json += "\"mosi\":" + String(mosi) + ",";
    json += "\"miso\":" + String(miso) + ",";
    json += "\"tx\":" + String(tx) + ",";
    json += "\"rx\":" + String(rx);
    json += "}";
    
    sendJSON(200, json);
}

void WebAPI::handleAntennaSaveConfig() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    
    int sclkStart = body.indexOf("\"sclk\":") + 7;
    int csnStart = body.indexOf("\"csn\":") + 6;
    int mosiStart = body.indexOf("\"mosi\":") + 7;
    int misoStart = body.indexOf("\"miso\":") + 7;
    int txStart = body.indexOf("\"tx\":") + 5;
    int rxStart = body.indexOf("\"rx\":") + 5;
    
    uint8_t sclk = body.substring(sclkStart).toInt();
    uint8_t csn = body.substring(csnStart).toInt();
    uint8_t mosi = body.substring(mosiStart).toInt();
    uint8_t miso = body.substring(misoStart).toInt();
    uint8_t tx = body.substring(txStart).toInt();
    uint8_t rx = body.substring(rxStart).toInt();
    
    Somfy.setPins(sclk, csn, mosi, miso, tx, rx);
    Somfy.savePinConfig();
    
    sendJSON(200, "{\"success\":true,\"message\":\"Antennen-Konfiguration gespeichert (433.42 MHz). Gerät startet neu...\"}");
    delay(2000);
    ESP.restart();
}

// Pairing Handlers
void WebAPI::handlePairingStart() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    int idStart = body.indexOf("\"shadeId\":") + 10;
    uint8_t shadeId = body.substring(idStart).toInt();
    
    if (Somfy.startPairing(shadeId)) {
        sendJSON(200, "{\"success\":true,\"message\":\"Pairing gestartet\"}");
    } else {
        sendError(400, "Fehler beim Starten des Pairing");
    }
}

void WebAPI::handlePairingStop() {
    Somfy.stopPairing();
    sendJSON(200, "{\"success\":true,\"message\":\"Pairing gestoppt\"}");
}

void WebAPI::handlePairingStatus() {
    PairingStep step = Somfy.getPairingStep();
    String instructions = Somfy.getPairingInstructions();
    bool isPairing = Somfy.isPairingMode();
    
    String json = "{";
    json += "\"isPairing\":" + String(isPairing ? "true" : "false") + ",";
    json += "\"step\":" + String((int)step) + ",";
    json += "\"instructions\":\"" + jsonEscape(instructions) + "\"";
    json += "}";
    
    sendJSON(200, json);
}

String WebAPI::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    return "text/plain";
}

// LED Control Handlers
void WebAPI::handleLEDStatus() {
    String json = "{";
    json += "\"enabled\":" + String(Somfy.getLEDEnabled() ? "true" : "false") + ",";
    json += "\"brightness\":" + String(Somfy.getLEDBrightness()) + ",";
    json += "\"currentPattern\":\"" + Somfy.getLEDCurrentPattern() + "\",";
    json += "\"availablePatterns\":[";
    
    const char* patterns[] = {
        "OFF", "ON", "BLINK_SLOW", "BLINK_FAST", 
        "BLINK_PATTERN_WIFI", "BLINK_PATTERN_DATA", 
        "BLINK_PATTERN_PAIRING", "BLINK_PATTERN_ERROR"
    };
    
    for (int i = 0; i < 8; i++) {
        if (i > 0) json += ",";
        json += "\"" + String(patterns[i]) + "\"";
    }
    json += "]";
    json += "}";
    
    sendJSON(200, json);
}

void WebAPI::handleLEDOn() {
    Somfy.setLEDEnabled(true);
    sendJSON(200, "{\"success\":true,\"message\":\"LED aktiviert\"}");
}

void WebAPI::handleLEDOff() {
    Somfy.setLEDEnabled(false);
    sendJSON(200, "{\"success\":true,\"message\":\"LED deaktiviert\"}");
}

void WebAPI::handleLEDSetPattern() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    int patternStart = body.indexOf("\"pattern\":\"") + 11;
    int patternEnd = body.indexOf("\"", patternStart);
    
    if (patternStart > 10 && patternEnd > patternStart) {
        String pattern = body.substring(patternStart, patternEnd);
        Somfy.setLEDPattern(pattern);
        sendJSON(200, "{\"success\":true,\"pattern\":\"" + pattern + "\"}");
    } else {
        sendError(400, "Ungültiges Pattern");
    }
}

void WebAPI::handleLEDSetBrightness() {
    if (!server.hasArg("plain")) {
        sendError(400, "Keine Daten");
        return;
    }
    
    String body = server.arg("plain");
    int brightnessStart = body.indexOf("\"brightness\":") + 13;
    uint8_t brightness = body.substring(brightnessStart).toInt();
    
    if (brightness >= 0 && brightness <= 255) {
        Somfy.setLEDBrightness(brightness);
        sendJSON(200, "{\"success\":true,\"brightness\":" + String(brightness) + "}");
    } else {
        sendError(400, "Helligkeit muss zwischen 0 und 255 liegen");
    }
}

void WebAPI::handleLEDPatternsInfo() {
    String json = "{";
    json += "\"patterns\":[";
    json += "{\"name\":\"OFF\",\"description\":\"LED aus\",\"blinkPattern\":\"Aus\"},";
    json += "{\"name\":\"ON\",\"description\":\"LED dauerhaft an\",\"blinkPattern\":\"Dauerhaft an\"},";
    json += "{\"name\":\"BLINK_SLOW\",\"description\":\"Langsames Blinken\",\"blinkPattern\":\"1s an, 1s aus\"},";
    json += "{\"name\":\"BLINK_FAST\",\"description\":\"Schnelles Blinken\",\"blinkPattern\":\"200ms an, 200ms aus\"},";
    json += "{\"name\":\"BLINK_PATTERN_WIFI\",\"description\":\"WLAN-Verbindung wird hergestellt\",\"blinkPattern\":\"Doppelblinken\"},";
    json += "{\"name\":\"BLINK_PATTERN_DATA\",\"description\":\"Daten werden gesendet/empfangen\",\"blinkPattern\":\"Kurzes Aufblitzen\"},";
    json += "{\"name\":\"BLINK_PATTERN_PAIRING\",\"description\":\"Pairing-Modus aktiv\",\"blinkPattern\":\"3x schnell blinken, Pause\"},";
    json += "{\"name\":\"BLINK_PATTERN_ERROR\",\"description\":\"Fehler aufgetreten\",\"blinkPattern\":\"Langes An, kurzes Aus\"}";
    json += "]";
    json += "}";
    
    sendJSON(200, json);
}
