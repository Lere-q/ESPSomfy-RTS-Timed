#ifndef WEBAPI_H
#define WEBAPI_H

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ============================================
// SKYEO - Web API (REST + WebSocket)
// ============================================

class WebAPI {
public:
    static void begin();
    static void loop();
    
    // HTTP Server
    static void handleRoot();
    static void handleShades();
    static void handleShadeCommand();
    static void handleShadeCreate();
    static void handleShadeDelete();
    static void handleSchedules();
    static void handleScheduleCreate();
    static void handleScheduleDelete();
    static void handleWifiConfig();
    static void handleWifiScan();
    static void handleDeviceInfo();
    static void handleReboot();
    static void handleNotFound();
    
    // Antenna / Receiver
    static void handleAntennaEnable();
    static void handleAntennaDisable();
    static void handleAntennaLogs();
    static void handleAntennaStatus();
    static void handleAntennaConfig();
    static void handleAntennaSaveConfig();
    
    // Pairing
    static void handlePairingStart();
    static void handlePairingStop();
    static void handlePairingStatus();
    
    // LED Control
    static void handleLEDStatus();
    static void handleLEDOn();
    static void handleLEDOff();
    static void handleLEDSetPattern();
    static void handleLEDSetBrightness();
    static void handleLEDPatternsInfo();
    
    // WebSocket
    static void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    static void broadcastMessage(const String& msg);
    static void broadcastShadeUpdate(uint8_t shadeId);
    static void broadcastScheduleUpdate(uint8_t scheduleId);
    
    // Utility
    static String getContentType(String filename);
    
private:
    static WebServer server;
    static WebSocketsServer ws;
    static bool wsEnabled;
    
    static void handleCORS();
    static void sendJSON(int code, const String& json);
    static void sendError(int code, const String& message);
};

#endif // WEBAPI_H
