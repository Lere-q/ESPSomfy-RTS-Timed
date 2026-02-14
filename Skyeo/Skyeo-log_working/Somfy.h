#ifndef SOMFY_H
#define SOMFY_H

#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "Config.h"
#include <vector>

// ============================================
// ORIGINAL SOMFY RTS CONSTANTS
// ============================================

#define SYMBOL 640                    // 640us symbol period
#define RECEIVE_ATTR IRAM_ATTR        // Interrupt im RAM für ESP32

// Timing Konstanten mit Toleranz
#define TOLERANCE_MIN 0.7
#define TOLERANCE_MAX 1.3

#define tempo_wakeup_pulse 9415
#define tempo_wakeup_min 6590         // 9415 * 0.7
#define tempo_wakeup_max 12239        // 9415 * 1.3
#define tempo_wakeup_silence 89565
#define tempo_wakeup_silence_min 62695
#define tempo_wakeup_silence_max 116434
#define tempo_synchro_hw_min 1792     // SYMBOL * 4 * 0.7
#define tempo_synchro_hw_max 3328     // SYMBOL * 4 * 1.3
#define tempo_synchro_sw_min 3395     // 4850 * 0.7
#define tempo_synchro_sw_max 6305     // 4850 * 1.3
#define tempo_half_symbol_min 448     // SYMBOL * 0.7
#define tempo_half_symbol_max 832     // SYMBOL * 1.3
#define tempo_symbol_min 896          // SYMBOL * 2 * 0.7
#define tempo_symbol_max 1664         // SYMBOL * 2 * 1.3
#define tempo_if_gap 30415            // Gap between frames

#define bitMin 448                    // SYMBOL * 0.7

#define MAX_RX_BUFFER 3               // RX FIFO Größe
#define MAX_TX_BUFFER 5               // TX FIFO Größe
#define MAX_TIMINGS 300               // Max Pulse pro Frame

#define TX_QUEUE_DELAY 100

// Debug LED Pin
#define DEBUG_LED_PIN 2

// ============================================
// SOMFY PROTOCOL DEFINITIONS
// ============================================

enum class radio_proto : byte {
    RTS = 0x00,
    RTW = 0x01,
    RTV = 0x02
};

enum class somfy_commands : byte {
    Unknown0 = 0x0,
    My = 0x1,
    Up = 0x2,
    MyUp = 0x3,
    Down = 0x4,
    MyDown = 0x5,
    UpDown = 0x6,
    MyUpDown = 0x7,
    Prog = 0x8,
    SunFlag = 0x9,
    Flag = 0xA,
    StepDown = 0xB,
    Toggle = 0xC,
    UnknownD = 0xD,
    Sensor = 0xE,
    RTWProto = 0xF,
    StepUp = 0x8B,
    Favorite = 0xC1,
    Stop = 0xF1
};

// Legacy Somfy Commands (für Kompatibilität)
enum SomfyCommand {
    SOMFY_MY = 0x1,
    SOMFY_UP = 0x2,
    SOMFY_MY_UP = 0x3,
    SOMFY_DOWN = 0x4,
    SOMFY_MY_DOWN = 0x5,
    SOMFY_UP_DOWN = 0x6,
    SOMFY_PROG = 0x8,
    SOMFY_SUNFLAG = 0x9,
    SOMFY_FLAG = 0xA
};

// ============================================
// FRAME STRUCTURES (from original)
// ============================================

typedef enum {
    waiting_synchro = 0,
    receiving_data = 1,
    complete = 2
} t_status;

struct somfy_rx_t {
    void clear() {
        this->status = t_status::waiting_synchro;
        this->bit_length = 56;
        this->cpt_synchro_hw = 0;
        this->cpt_bits = 0;
        this->previous_bit = 0;
        this->waiting_half_symbol = false;
        memset(this->payload, 0, sizeof(this->payload));
        memset(this->pulses, 0, sizeof(this->pulses));
        this->pulseCount = 0;
        this->timestamp = 0;
        this->rssi = 0;
    }
    t_status status;
    uint8_t bit_length = 56;
    uint8_t cpt_synchro_hw = 0;
    uint8_t cpt_bits = 0;
    uint8_t previous_bit = 0;
    bool waiting_half_symbol;
    uint8_t payload[10];
    unsigned int pulses[MAX_TIMINGS];
    uint16_t pulseCount = 0;
    uint32_t timestamp;
    int8_t rssi;
};

struct somfy_tx_t {
    void clear() {
        this->hwsync = 0;
        this->bit_length = 0;
        memset(this->payload, 0x00, sizeof(this->payload));
    }
    uint8_t hwsync = 0;
    uint8_t bit_length = 0;
    uint8_t payload[10] = {};
};

struct somfy_rx_queue_t {
    void init();
    uint8_t length = 0;
    uint8_t index[MAX_RX_BUFFER];
    somfy_rx_t items[MAX_RX_BUFFER];
    void push(somfy_rx_t *rx);
    bool pop(somfy_rx_t *rx);
};

struct somfy_tx_queue_t {
    somfy_tx_queue_t() { this->clear(); }
    void clear() {
        for (uint8_t i = 0; i < MAX_TX_BUFFER; i++) {
            this->index[i] = 255;
            this->items[i].clear();
        }
        this->length = 0;
    }
    unsigned long delay_time = 0;
    uint8_t length = 0;
    uint8_t index[MAX_TX_BUFFER] = {255};
    somfy_tx_t items[MAX_TX_BUFFER];
    bool pop(somfy_tx_t *tx);
    void push(somfy_rx_t *rx);
    void push(uint8_t hwsync, uint8_t *payload, uint8_t bit_length);
};

struct somfy_frame_t {
    bool valid = false;
    bool processed = false;
    bool synonym = false;
    radio_proto proto = radio_proto::RTS;
    int rssi = 0;
    byte lqi = 0x0;
    somfy_commands cmd;
    uint32_t remoteAddress = 0;
    uint16_t rollingCode = 0;
    uint8_t encKey = 0xA7;
    uint8_t checksum = 0;
    uint8_t hwsync = 0;
    uint8_t repeats = 0;
    uint32_t await = 0;
    uint8_t bitLength = 56;
    uint16_t pulseCount = 0;
    uint8_t stepSize = 0;
    
    void print();
    void encode80BitFrame(uint8_t *frame, uint8_t repeat);
    uint8_t calc80Checksum(uint8_t b0, uint8_t b1, uint8_t b2);
    uint8_t encode80Byte7(uint8_t start, uint8_t repeat);
    void encodeFrame(uint8_t *frame);
    void decodeFrame(uint8_t* frame);
    void decodeFrame(somfy_rx_t *rx);
    bool isRepeat(somfy_frame_t &f);
    bool isSynonym(somfy_frame_t &f);
    void copy(somfy_frame_t &f);
};

// Transceiver Configuration
struct transceiver_config_t {
    bool printBuffer = false;
    bool enabled = false;
    uint8_t type = 56;
    radio_proto proto = radio_proto::RTS;
    uint8_t SCKPin = 18;
    uint8_t TXPin = 13;
    uint8_t RXPin = 12;
    uint8_t MOSIPin = 23;
    uint8_t MISOPin = 19;
    uint8_t CSNPin = 5;
    bool radioInit = false;
    float frequency = 433.42;
    float deviation = 47.60;
    float rxBandwidth = 99.97;
    int8_t txPower = 10;
    
    void save();
    void load();
    void apply();
    void removeNVSKey(const char *key);
};

// ============================================
// SKYEO STRUCTURES
// ============================================

struct ShadeState {
    uint8_t id;
    char name[32];
    uint8_t position;
    uint8_t target;
    uint32_t remoteAddress;
    uint16_t rollingCode;
    uint16_t upTime;
    uint16_t downTime;
    bool moving;
    int8_t direction;
    uint32_t moveStartTime;
    ShadeType type;
};

struct ReceivedFrame {
    uint32_t timestamp;
    uint32_t remoteAddress;
    uint16_t rollingCode;
    uint8_t command;
    int8_t rssi;
    bool valid;
    char direction[3];
};

struct AntennaStatus {
    bool initialized;
    uint8_t version;
    int8_t rssi;
    bool receiveMode;
    uint32_t framesReceived;
    uint32_t framesSent;
    char lastError[64];
};

enum PairingStep { 
    PAIR_NONE=0, 
    PAIR_WAIT_PROG=1, 
    PAIR_SEND_PROG=2, 
    PAIR_WAIT_CONFIRM=3, 
    PAIR_SUCCESS=4, 
    PAIR_FAILED=5 
};

// Blinkmuster
enum LedPattern {
    LED_OFF = 0,
    LED_ON = 1,
    LED_SLOW_BLINK = 2,
    LED_FAST_BLINK = 3,
    LED_DOUBLE_BLINK = 4,
    LED_TRIPLE_BLINK = 5,
    LED_ERROR = 6,
    LED_WIFI_CONNECTING = 7,
    LED_WIFI_CONNECTED = 8
};

// ============================================
// TRANSLATION FUNCTIONS
// ============================================

String translateSomfyCommand(const somfy_commands cmd);
somfy_commands translateSomfyCommand(const String& string);

// ============================================
// LED CONTROLLER
// ============================================

class DebugLED {
private:
    uint8_t pin;
    LedPattern currentPattern;
    uint32_t lastUpdate;
    uint8_t brightness;
    bool enabled;
public:
    DebugLED(uint8_t ledPin=DEBUG_LED_PIN) 
        : pin(ledPin), currentPattern(LED_OFF), lastUpdate(0), brightness(255), enabled(true) {}
    void begin() { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
    void setPattern(LedPattern p) { if(currentPattern!=p){currentPattern=p;lastUpdate=0;} }
    void setBrightness(uint8_t b) { brightness=b; }
    void setEnabled(bool e) { enabled=e; if(!e)digitalWrite(pin,LOW); }
    bool isEnabled() { return enabled; }
    uint8_t getBrightness() { return brightness; }
    LedPattern getPattern() { return currentPattern; }
    const char* getPatternName(LedPattern p);
    void update();
};

// ============================================
// TRANSCEIVER CLASS (Original implementation)
// ============================================

class Transceiver {
private:
    static void handleReceive();
    bool _received = false;
    somfy_frame_t frame;
public:
    transceiver_config_t config;
    bool printBuffer = false;
    
    bool save();
    bool begin();
    void loop();
    bool end();
    bool receive(somfy_rx_t *rx);
    void clearReceived();
    void enableReceive();
    void disableReceive();
    somfy_frame_t& lastFrame();
    void sendFrame(uint8_t *frame, uint8_t sync, uint8_t bitLength = 56);
    void beginTransmit();
    void endTransmit();
    void emitFrame(somfy_frame_t *frame, somfy_rx_t *rx = nullptr);
    bool usesPin(uint8_t pin);
};

// ============================================
// SOMFY TRANSCEIVER (Skyeo API)
// ============================================

class SomfyTransceiver {
public:
    DebugLED led;
    Transceiver transceiver;
    
    SomfyTransceiver(uint8_t sclk=18, uint8_t csn=5, uint8_t mosi=23, uint8_t miso=19, uint8_t tx=17, uint8_t rx=16);
    
    // Initialization
    bool begin();
    void end();
    void update();
    
    // LED Control
    void setLedPattern(LedPattern p) { led.setPattern(p); }
    void setLedBrightness(uint8_t b) { led.setBrightness(b); }
    void setLedEnabled(bool e) { led.setEnabled(e); }
    bool getLEDEnabled() { return led.isEnabled(); }
    void setLEDEnabled(bool enabled) { led.setEnabled(enabled); }
    uint8_t getLEDBrightness() { return led.getBrightness(); }
    void setLEDBrightness(uint8_t brightness) { led.setBrightness(brightness); }
    void setLEDPattern(const String& pattern);
    String getLEDCurrentPattern();
    
    // Send Commands
    bool sendCommand(uint8_t shadeId, SomfyCommand cmd);
    bool sendCommand(uint8_t shadeId, SomfyCommand cmd, uint8_t repeats);
    bool sendPosition(uint8_t shadeId, uint8_t position);
    void sendFrame(uint8_t *frame, uint8_t sync, uint8_t bitLength);
    
    // Shade Management
    bool addShade(uint8_t id, const char* name, uint32_t remoteAddr, uint16_t upTime, uint16_t downTime);
    bool removeShade(uint8_t id);
    ShadeState* getShade(uint8_t id);
    uint8_t getShadeCount();
    uint8_t getPosition(uint8_t id);
    bool isMoving(uint8_t id);
    void stop(uint8_t id);
    void setMyPosition(uint8_t id, uint8_t position);
    
    // Receiver
    void enableReceive();
    void disableReceive();
    bool isReceiving();
    std::vector<ReceivedFrame> getReceivedFrames();
    void clearReceivedFrames();
    
    // Pairing
    bool startPairing(uint8_t shadeId);
    void stopPairing();
    PairingStep getPairingStep();
    String getPairingInstructions();
    void pairingLoop();
    bool isPairingMode();
    bool enterPairingMode(uint8_t shadeId) { return startPairing(shadeId); }
    void exitPairingMode() { stopPairing(); }
    
    // Configuration
    void setPins(uint8_t sclk, uint8_t csn, uint8_t mosi, uint8_t miso, uint8_t tx, uint8_t rx);
    void getPins(uint8_t &sclk, uint8_t &csn, uint8_t &mosi, uint8_t &miso, uint8_t &tx, uint8_t &rx);
    void savePinConfig();
    bool loadPinConfig();
    
    // Status
    uint8_t getLastRSSI() { return lastRSSI; }
    AntennaStatus getAntennaStatus();
    bool checkAntennaConnection();
    bool isInitialized() { return initialized; }
    String getLastError() { return lastError; }
    
    // Frame Processing
    void processFrame(somfy_frame_t &frame, bool internal = false);
    void processWaitingFrame();
    
private:
    // Pin Configuration
    uint8_t sclkPin, csnPin, mosiPin, misoPin, txPin, rxPin;
    
    // Statistics
    uint32_t framesReceived, framesSent;
    
    // Shades
    ShadeState shades[MAX_SHADES];
    uint8_t shadeCount;
    
    // Pairing
    bool pairingMode;
    uint8_t pairingShadeId;
    PairingStep pairingStep;
    unsigned long pairingStartTime;
    unsigned long lastProgTime;
    
    // Receiver State
    bool receiveMode;
    std::vector<ReceivedFrame> receivedFrames;
    unsigned long lastReceiveTime;
    uint8_t lastRSSI;
    bool initialized;
    String lastError;
    
    // Frame Buffer
    somfy_frame_t lastFrame;
    
    // Internal Methods
    bool initCC1101();
    void processReceivedFrames();
    void handlePairingFrame(ReceivedFrame& frame);
    void updateShadePosition(uint8_t shadeId);
    void saveShadeState(uint8_t shadeId);
    void buildFrame(uint8_t shadeId, SomfyCommand cmd, uint8_t* frame);
    void sendRawFrame(uint8_t* frame, uint8_t len);
    
    // Command Translation
    somfy_commands toSomfyCommand(SomfyCommand cmd);
    SomfyCommand toLegacyCommand(somfy_commands cmd);
};

extern SomfyTransceiver Somfy;

#endif
