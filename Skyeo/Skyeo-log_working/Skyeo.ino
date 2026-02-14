/*
  ============================================
  SKYEO - Minimalistischer ESP32 Somfy Controller
  ============================================
  
  Ein schlanker ESP32-basierter Somfy RTS Controller mit:
  - Web-Interface (eingebettet im Flash)
  - REST API + WebSocket
  - Zeitgesteuerung (Scheduler)
  - Keine externen Abhängigkeiten (kein MQTT, kein SSDP, kein GitOTA)
  
  Hardware:
  - ESP32 (Standard, S3, C3)
  - CC1101 Transceiver Modul
  
  Pinout (Standard ESP32):
  - CS:   GPIO 5
  - MOSI: GPIO 23
  - MISO: GPIO 19
  - SCK:  GPIO 18
  
  Version: 1.0.0
  ============================================
*/

#include "Utils.h"
#include "Config.h"
#include "Network.h"
#include "Somfy.h"
#include "Scheduler.h"
#include "WebAPI.h"

// Build configuration
#define SKYEO_DEBUG  // Kommentiere aus für Release

// Timing
unsigned long lastStatusUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 5000; // 5 Sekunden

// Serielle Konfiguration
bool serialConfigMode = false;
unsigned long serialPromptTime = 0;
String serialBuffer = "";

// Funktionsdeklarationen
void handleSerialConfig();

// Setup
void setup() {
    // Serial für Debug
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║              SKYEO v1.0.0              ║");
    Serial.println("║     Minimalistischer Somfy Controller  ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("\n");
    
    // Watchdog
    setupWatchdog(10000); // 10 Sekunden Timeout
    
    // Konfiguration initialisieren
    Serial.println("[SKYEO] Initialisiere Konfiguration...");
    Config::begin();
    
    // Netzwerk initialisieren
    Serial.println("[SKYEO] Initialisiere Netzwerk...");
    Network::begin();
    
    // Prüfe ob WiFi konfiguriert ist
    if (Config::isFirstRun() || !Config::hasWiFiConfig()) {
        Serial.println("[SKYEO] Erstinstallation erkannt - Starte AP-Mode");
        Network::startAP("Skyeo-Setup", "skyeo123");
        Config::setFirstRun(false);
    } else {
        WiFiConfig wifi;
        if (Config::loadWiFi(wifi)) {
            if (!Network::connectSTA(wifi.ssid, wifi.password, wifi.dhcp)) {
                Serial.println("[SKYEO] WiFi Verbindung fehlgeschlagen - Starte AP-Mode");
                Network::startAP("Skyeo-Setup", "skyeo123");
            }
        }
    }
    
    // Somfy Transceiver initialisieren
    Serial.println("[SKYEO] Initialisiere Somfy Transceiver...");
    if (!Somfy.begin()) {
        Serial.println("[SKYEO] WARNUNG: Somfy Transceiver konnte nicht initialisiert werden!");
    } else {
        Serial.printf("[SKYEO] %d Shades geladen\n", Somfy.getShadeCount());
    }
    
    // Scheduler initialisieren
    Serial.println("[SKYEO] Initialisiere Scheduler...");
    Scheduler::begin();
    
    // Web API initialisieren
    Serial.println("[SKYEO] Initialisiere Web API...");
    WebAPI::begin();
    
    // System bereit
    Serial.println("\n[SKYEO] System bereit!");
    
    // AP-Modus Info
    if (Network::isAPMode()) {
        Serial.println("╔════════════════════════════════════════╗");
        Serial.println("║           AP aktiv!                    ║");
        Serial.print("║   IP: ");
        Serial.print(Network::getIPAddress());
        Serial.println("                ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.println("\nVerbinde dich mit dem WLAN 'Skyeo-Setup'");
        Serial.println("und öffne http://192.168.4.1 im Browser\n");
        
        // Serielle Konfiguration anbieten
        Serial.println("═══ Serielle WLAN Konfiguration (optional) ═══");
        Serial.println("SSID: (warte auf Eingabe...)");
        serialConfigMode = true;
    } else {
        Serial.print("[SKYEO] IP-Adresse: ");
        Serial.println(Network::getIPAddress());
        Serial.print("[SKYEO] Modus: Station");
        Serial.println("\n[SKYEO] Web-Interface: http://" + Network::getIPAddress());
    }
    Serial.println("\n");
}

// Main Loop
void loop() {
    // Watchdog füttern
    feedWatchdog();
    
    // Netzwerk-Loop
    Network::loop();
    
    // Somfy State Updates
    Somfy.update();
    
    // Scheduler
    Scheduler::loop();
    
    // Web API
    WebAPI::loop();
    
    // Status Update (alle 5 Sekunden)
    if (millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
        lastStatusUpdate = millis();
        
        // Sende Status Updates an verbundene WebSocket Clients
        for (uint8_t i = 0; i < MAX_SHADES; i++) {
            ShadeState* shade = Somfy.getShade(i);
            if (shade && shade->moving) {
                WebAPI::broadcastShadeUpdate(i);
            }
        }
    }
    
    // Serielle Konfiguration verarbeiten (nur im AP-Modus)
    if (serialConfigMode && Network::isAPMode()) {
        handleSerialConfig();
    }
    
    // Kleine Verzögerung um CPU zu entlasten
    delay(1);
}

// Serielle WLAN Konfiguration
void handleSerialConfig() {
    static int configStep = 0; // 0: SSID, 1: Passwort
    static String ssid = "";
    
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        // Enter-Taste (CR oder LF)
        if (c == '\r' || c == '\n') {
            if (serialBuffer.length() > 0) {
                if (configStep == 0) {
                    // SSID eingegeben
                    ssid = serialBuffer;
                    serialBuffer = "";
                    configStep = 1;
                    Serial.println("");
                    Serial.println("SSID: " + ssid);
                    Serial.println("PASSWORT (optional): ");
                } else if (configStep == 1) {
                    // Passwort eingegeben (oder leer)
                    String password = serialBuffer;
                    serialBuffer = "";
                    
                    Serial.println("");
                    Serial.println("Konfiguration wird gespeichert...");
                    
                    // Speichern
                    WiFiConfig wifi;
                    strncpy(wifi.ssid, ssid.c_str(), sizeof(wifi.ssid));
                    strncpy(wifi.password, password.c_str(), sizeof(wifi.password));
                    wifi.dhcp = true;
                    wifi.configured = true;
                    
                    if (Config::saveWiFi(wifi)) {
                        Serial.println("✓ WLAN Konfiguration gespeichert!");
                        Serial.println("Gerät startet neu...");
                        delay(2000);
                        ESP.restart();
                    } else {
                        Serial.println("✗ Fehler beim Speichern!");
                        configStep = 0;
                        ssid = "";
                        Serial.println("\nSSID: ");
                    }
                }
            }
        } else if (c == 127 || c == 8) {
            // Backspace
            if (serialBuffer.length() > 0) {
                serialBuffer.remove(serialBuffer.length() - 1);
                Serial.print("\b \b");
            }
        } else if (c >= 32 && c <= 126) {
            // Normales Zeichen
            serialBuffer += c;
            // Zeige Sternchen für Passwort
            if (configStep == 1) {
                Serial.print("*");
            } else {
                Serial.print(c);
            }
        }
    }
}
