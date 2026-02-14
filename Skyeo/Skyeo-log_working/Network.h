#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

// ============================================
// SKYEO - Network Management
// WiFi STA + AP Mode
// ============================================

class Network {
public:
    static void begin();
    static void loop();
    
    // WiFi Connection
    static bool connectSTA(const char* ssid, const char* password, bool dhcp = true);
    static bool connectSTA(const char* ssid, const char* password, IPAddress ip, IPAddress gateway, IPAddress subnet);
    static void startAP(const char* ssid = "Skyeo-Setup", const char* password = "skyeo123");
    static void disconnect();
    
    // Status
    static bool isConnected();
    static String getIPAddress();
    static int32_t getRSSI();
    static String getMAC();
    
    // Scan
    static int scanNetworks();
    static String getSSID(int index);
    static int32_t getNetworkRSSI(int index);
    static uint8_t getNetworkEncryption(int index);
    
    // mDNS
    static bool startMDNS(const char* hostname = "skyeo");
    static void stopMDNS();
    
    // Mode
    static bool isAPMode();
    
private:
    static bool apMode;
    static bool connected;
    static unsigned long lastReconnectAttempt;
    static const unsigned long RECONNECT_INTERVAL = 30000; // 30 Sekunden
};

#endif // NETWORK_H
