#include "Network.h"
#include "Config.h"
#include "Utils.h"

// ============================================
// SKYEO - Network Management Implementation
// ============================================

bool Network::apMode = false;
bool Network::connected = false;
unsigned long Network::lastReconnectAttempt = 0;

void Network::begin() {
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    delay(100);
    
    DEBUG_PRINTLN("[SKYEO] Netzwerk initialisiert");
}

void Network::loop() {
    if (apMode) return; // Im AP-Mode keine Reconnect-Logik
    
    if (!isConnected()) {
        if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = millis();
            DEBUG_PRINTLN("[SKYEO] WiFi getrennt, versuche Reconnect...");
            
            WiFi.reconnect();
            
            // Warte auf Verbindung
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                feedWatchdog();
                attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                connected = true;
                DEBUG_PRINTF("[SKYEO] WiFi verbunden: %s\n", WiFi.localIP().toString().c_str());
                startMDNS();
            } else {
                connected = false;
                DEBUG_PRINTLN("[SKYEO] WiFi Reconnect fehlgeschlagen");
            }
        }
    } else {
        connected = true;
    }
}

bool Network::connectSTA(const char* ssid, const char* password, bool dhcp) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    DEBUG_PRINTF("[SKYEO] Verbinde mit WiFi: %s\n", ssid);
    
    WiFi.begin(ssid, password);
    
    // Warte auf Verbindung (max 20 Sekunden)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        feedWatchdog();
        DEBUG_PRINT(".");
        attempts++;
    }
    DEBUG_PRINTLN();
    
    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        apMode = false;
        DEBUG_PRINTF("[SKYEO] WiFi verbunden, IP: %s\n", WiFi.localIP().toString().c_str());
        startMDNS();
        return true;
    }
    
    DEBUG_PRINTLN("[SKYEO] WiFi Verbindung fehlgeschlagen");
    connected = false;
    return false;
}

bool Network::connectSTA(const char* ssid, const char* password, IPAddress ip, IPAddress gateway, IPAddress subnet) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // Statische IP konfigurieren
    if (!WiFi.config(ip, gateway, subnet)) {
        DEBUG_PRINTLN("[SKYEO] Statische IP Konfiguration fehlgeschlagen");
        return false;
    }
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        feedWatchdog();
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        apMode = false;
        DEBUG_PRINTF("[SKYEO] WiFi verbunden (statisch), IP: %s\n", WiFi.localIP().toString().c_str());
        startMDNS();
        return true;
    }
    
    connected = false;
    return false;
}

void Network::startAP(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP);
    delay(100);
    
    // AP konfigurieren: IP 192.168.4.1
    IPAddress localIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.softAPConfig(localIP, gateway, subnet);
    
    if (strlen(password) >= 8) {
        WiFi.softAP(ssid, password);
    } else {
        WiFi.softAP(ssid);
    }
    
    apMode = true;
    connected = true;
    
    DEBUG_PRINTF("[SKYEO] AP-Mode gestartet: %s (192.168.4.1)\n", ssid);
}

void Network::disconnect() {
    WiFi.disconnect();
    connected = false;
    DEBUG_PRINTLN("[SKYEO] WiFi getrennt");
}

bool Network::isConnected() {
    return WiFi.status() == WL_CONNECTED || apMode;
}

String Network::getIPAddress() {
    if (apMode) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

int32_t Network::getRSSI() {
    return WiFi.RSSI();
}

String Network::getMAC() {
    return WiFi.macAddress();
}

// ============================================
// Scan Networks
// ============================================

int Network::scanNetworks() {
    DEBUG_PRINTLN("[SKYEO] Scanne WiFi Netzwerke...");
    int n = WiFi.scanNetworks();
    DEBUG_PRINTF("[SKYEO] %d Netzwerke gefunden\n", n);
    return n;
}

String Network::getSSID(int index) {
    return WiFi.SSID(index);
}

int32_t Network::getNetworkRSSI(int index) {
    return WiFi.RSSI(index);
}

uint8_t Network::getNetworkEncryption(int index) {
    return WiFi.encryptionType(index);
}

// ============================================
// mDNS
// ============================================

bool Network::startMDNS(const char* hostname) {
    if (MDNS.begin(hostname)) {
        MDNS.addService("http", "tcp", 80);
        DEBUG_PRINTF("[SKYEO] mDNS gestartet: %s.local\n", hostname);
        return true;
    }
    DEBUG_PRINTLN("[SKYEO] mDNS Start fehlgeschlagen");
    return false;
}

void Network::stopMDNS() {
    MDNS.end();
    DEBUG_PRINTLN("[SKYEO] mDNS gestoppt");
}

bool Network::isAPMode() {
    return apMode;
}
