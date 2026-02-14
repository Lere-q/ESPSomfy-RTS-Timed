#include "Somfy.h"
#include "Config.h"
#include "Utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <Preferences.h>

// ============================================
// GLOBALS
// ============================================

extern Preferences pref;
SomfyTransceiver Somfy;

// RX/TX Queue Globals
static somfy_rx_queue_t rx_queue;
static somfy_tx_queue_t tx_queue;
static somfy_rx_t somfy_rx;
static int interruptPin = 0;
static volatile uint8_t rxmode = 0;

// ============================================
// COMMAND TRANSLATION
// ============================================

String translateSomfyCommand(const somfy_commands cmd) {
    switch (cmd) {
    case somfy_commands::Up: return "Up";
    case somfy_commands::Down: return "Down";
    case somfy_commands::My: return "My";
    case somfy_commands::MyUp: return "My+Up";
    case somfy_commands::UpDown: return "Up+Down";
    case somfy_commands::MyDown: return "My+Down";
    case somfy_commands::MyUpDown: return "My+Up+Down";
    case somfy_commands::Prog: return "Prog";
    case somfy_commands::SunFlag: return "Sun Flag";
    case somfy_commands::Flag: return "Flag";
    case somfy_commands::StepUp: return "Step Up";
    case somfy_commands::StepDown: return "Step Down";
    case somfy_commands::Sensor: return "Sensor";
    case somfy_commands::Toggle: return "Toggle";
    case somfy_commands::Favorite: return "Favorite";
    case somfy_commands::Stop: return "Stop";
    default: return "Unknown(" + String((uint8_t)cmd) + ")";
    }
}

somfy_commands translateSomfyCommand(const String& string) {
    if (string.equalsIgnoreCase("My")) return somfy_commands::My;
    else if (string.equalsIgnoreCase("Up")) return somfy_commands::Up;
    else if (string.equalsIgnoreCase("MyUp")) return somfy_commands::MyUp;
    else if (string.equalsIgnoreCase("Down")) return somfy_commands::Down;
    else if (string.equalsIgnoreCase("MyDown")) return somfy_commands::MyDown;
    else if (string.equalsIgnoreCase("UpDown")) return somfy_commands::UpDown;
    else if (string.equalsIgnoreCase("MyUpDown")) return somfy_commands::MyUpDown;
    else if (string.equalsIgnoreCase("Prog")) return somfy_commands::Prog;
    else if (string.equalsIgnoreCase("SunFlag")) return somfy_commands::SunFlag;
    else if (string.equalsIgnoreCase("StepUp")) return somfy_commands::StepUp;
    else if (string.equalsIgnoreCase("StepDown")) return somfy_commands::StepDown;
    else if (string.equalsIgnoreCase("Flag")) return somfy_commands::Flag;
    else if (string.equalsIgnoreCase("Sensor")) return somfy_commands::Sensor;
    else if (string.equalsIgnoreCase("Toggle")) return somfy_commands::Toggle;
    else if (string.equalsIgnoreCase("Favorite")) return somfy_commands::Favorite;
    else if (string.equalsIgnoreCase("Stop")) return somfy_commands::Stop;
    else if (string.startsWith("p") || string.startsWith("P")) return somfy_commands::Prog;
    else if (string.startsWith("u") || string.startsWith("U")) return somfy_commands::Up;
    else if (string.startsWith("d") || string.startsWith("D")) return somfy_commands::Down;
    else if (string.startsWith("m") || string.startsWith("M")) return somfy_commands::My;
    else if (string.startsWith("f") || string.startsWith("F")) return somfy_commands::Flag;
    else if (string.startsWith("s") || string.startsWith("S")) return somfy_commands::SunFlag;
    else if (string.startsWith("t") || string.startsWith("T")) return somfy_commands::Toggle;
    else if (string.length() == 1) return static_cast<somfy_commands>(strtol(string.c_str(), nullptr, 16));
    else return somfy_commands::My;
}

// ============================================
// SOMFY_FRAME_T IMPLEMENTATION
// ============================================

void somfy_frame_t::print() {
    Serial.println("----------- Receiving -------------");
    Serial.print("RSSI:");
    Serial.print(this->rssi);
    Serial.print(" LQI:");
    Serial.println(this->lqi);
    Serial.print("CMD:");
    Serial.print(translateSomfyCommand(this->cmd));
    Serial.print(" ADDR:");
    Serial.print(this->remoteAddress, HEX);
    Serial.print(" RCODE:");
    Serial.println(this->rollingCode);
    Serial.print("KEY:");
    Serial.print(this->encKey, HEX);
    Serial.print(" CS:");
    Serial.println(this->checksum);
}

uint8_t somfy_frame_t::calc80Checksum(uint8_t b0, uint8_t b1, uint8_t b2) {
    uint8_t cs80 = 0;
    cs80 = (((b0 & 0xF0) >> 4) ^ ((b1 & 0xF0) >> 4));
    cs80 ^= ((b2 & 0xF0) >> 4);
    cs80 ^= (b0 & 0x0F);
    cs80 ^= (b1 & 0x0F);
    return cs80;
}

void somfy_frame_t::decodeFrame(uint8_t* frame) {
    uint8_t decoded[10];
    decoded[0] = frame[0];
    decoded[7] = frame[7];
    decoded[8] = frame[8];
    decoded[9] = frame[9];
    
    for (uint8_t i = 1; i < 7; i++) {
        decoded[i] = frame[i] ^ frame[i - 1];
    }
    
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < 7; i++) {
        if (i == 1) checksum = checksum ^ (decoded[i] >> 4);
        else checksum = checksum ^ decoded[i] ^ (decoded[i] >> 4);
    }
    checksum &= 0b1111;

    this->checksum = decoded[1] & 0b1111;
    this->encKey = decoded[0];
    this->cmd = (somfy_commands)((decoded[1] >> 4));
    
    if(this->cmd == somfy_commands::RTWProto) {
        if(this->encKey >= 160) {
            this->proto = radio_proto::RTS;
            if(this->encKey == 164) this->cmd = somfy_commands::Toggle;
        }
        else if(this->encKey > 148) {
            this->proto = radio_proto::RTV;
            this->cmd = (somfy_commands)(this->encKey - 148);
        }
        else if(this->encKey > 133) {
            this->proto = radio_proto::RTW;
            this->cmd = (somfy_commands)(this->encKey - 133);
        }
    }
    else this->proto = radio_proto::RTS;
    
    this->processed = false;
    this->rollingCode = decoded[3] + (decoded[2] << 8);
    this->remoteAddress = (decoded[6] + (decoded[5] << 8) + (decoded[4] << 16));
    this->valid = this->checksum == checksum && this->remoteAddress > 0 && this->remoteAddress < 16777215;
    
    if (this->cmd != somfy_commands::Sensor && this->valid) 
        this->valid = (this->rollingCode > 0);
    
    // 80-bit frame validation
    if(this->valid && this->proto == radio_proto::RTS && this->bitLength == 80) {
        if((decoded[9] & 0x0F) != this->calc80Checksum(decoded[7], decoded[8], decoded[9])) 
            this->valid = false;
        if(this->valid) {
            if(this->cmd == somfy_commands::My) 
                this->cmd = (somfy_commands)((decoded[1] >> 4) | ((decoded[8] & 0x0F) << 4));
            else if(this->cmd == somfy_commands::StepDown) 
                this->cmd = (somfy_commands)((decoded[1] >> 4) | ((decoded[8] & 0x08) << 4));
        }
    }
    
    // Validate command
    if (this->valid) {
        switch (this->cmd) {
        case somfy_commands::My:
        case somfy_commands::Up:
        case somfy_commands::MyUp:
        case somfy_commands::Down:
        case somfy_commands::MyDown:
        case somfy_commands::UpDown:
        case somfy_commands::MyUpDown:
        case somfy_commands::Prog:
        case somfy_commands::Flag:
        case somfy_commands::SunFlag:
        case somfy_commands::Sensor:
            break;
        case somfy_commands::UnknownD:
        case somfy_commands::RTWProto:
            this->valid = false;
            break;
        case somfy_commands::StepUp:
        case somfy_commands::StepDown:
            this->stepSize = ((decoded[8] & 0x07) << 4) | ((decoded[9] & 0xF0) >> 4);
            break;
        case somfy_commands::Toggle:
        case somfy_commands::Favorite:
        case somfy_commands::Stop:
            break;
        default:
            this->valid = false;
            break;
        }
    }
    
    if(this->valid && this->encKey == 0) this->valid = false;
}

void somfy_frame_t::decodeFrame(somfy_rx_t *rx) {
    this->hwsync = rx->cpt_synchro_hw;
    this->pulseCount = rx->pulseCount;
    this->bitLength = rx->bit_length;
    this->rssi = rx->rssi;
    this->decodeFrame(rx->payload);
}

uint8_t somfy_frame_t::encode80Byte7(uint8_t start, uint8_t repeat) {
    while((repeat * 4) + start > 255) repeat -= 15;
    return start + (repeat * 4);
}

void somfy_frame_t::encode80BitFrame(uint8_t *frame, uint8_t repeat) {
    switch(this->cmd) {
    case somfy_commands::StepUp:
        if(repeat == 0) frame[1] = (static_cast<uint8_t>(somfy_commands::StepDown) << 4) | (frame[1] & 0x0F);
        if(this->stepSize == 0) this->stepSize = 1;
        frame[7] = 132;
        frame[8] = ((this->stepSize & 0x70) >> 4) | 0x38;
        frame[9] = ((this->stepSize & 0x0F) << 4);
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::StepDown:
        if(repeat == 0) frame[1] = (static_cast<uint8_t>(somfy_commands::StepDown) << 4) | (frame[1] & 0x0F);
        if(this->stepSize == 0) this->stepSize = 1;
        frame[7] = 132;
        frame[8] = ((this->stepSize & 0x70) >> 4) | 0x30;
        frame[9] = ((this->stepSize & 0x0F) << 4);
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::Favorite:
        if(repeat == 0) frame[1] = (static_cast<uint8_t>(somfy_commands::My) << 4) | (frame[1] & 0x0F);
        frame[7] = repeat > 0 ? 132 : 196;
        frame[8] = 44;
        frame[9] = 0x90;
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::Stop:
        if(repeat == 0) frame[1] = (static_cast<uint8_t>(somfy_commands::My) << 4) | (frame[1] & 0x0F);
        frame[7] = repeat > 0 ? 132 : 196;
        frame[8] = 47;
        frame[9] = 0xF0;
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::Toggle:
        frame[0] = 164;
        frame[1] |= 0xF0;
        frame[7] = this->encode80Byte7(196, repeat);
        frame[8] = 0;
        frame[9] = 0x10;
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::Up:
        frame[7] = this->encode80Byte7(196, repeat);
        frame[8] = 32;
        frame[9] = 0x00;
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::Down:
        frame[7] = this->encode80Byte7(196, repeat);
        frame[8] = 44;
        frame[9] = 0x80;
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    case somfy_commands::Prog:
    case somfy_commands::UpDown:
    case somfy_commands::MyDown:
    case somfy_commands::MyUp:
    case somfy_commands::MyUpDown:
    case somfy_commands::My:
        frame[7] = this->encode80Byte7(196, repeat);
        frame[8] = 0x00;
        frame[9] = 0x10;
        frame[9] |= this->calc80Checksum(frame[7], frame[8], frame[9]);
        break;
    default:
        break;
    }
}

void somfy_frame_t::encodeFrame(uint8_t *frame) {
    const uint8_t btn = static_cast<uint8_t>(cmd);
    this->valid = true;
    frame[0] = this->encKey;
    frame[1] = (btn & 0x0F) << 4;
    frame[2] = this->rollingCode >> 8;
    frame[3] = this->rollingCode;
    frame[4] = this->remoteAddress >> 16;
    frame[5] = this->remoteAddress >> 8;
    frame[6] = this->remoteAddress;
    frame[7] = 132;
    frame[8] = 0;
    frame[9] = 29;
    
    // RTW Protocol
    if(this->proto == radio_proto::RTW) {
        frame[1] = 0xF0;
        switch(this->cmd) {
        case somfy_commands::My: frame[0] = 133; break;
        case somfy_commands::Up: frame[0] = 134; break;
        case somfy_commands::MyUp: frame[0] = 135; break;
        case somfy_commands::Down: frame[0] = 136; break;
        case somfy_commands::MyDown: frame[0] = 137; break;
        case somfy_commands::UpDown: frame[0] = 138; break;
        case somfy_commands::MyUpDown: frame[0] = 139; break;
        case somfy_commands::Prog: frame[0] = 140; break;
        case somfy_commands::SunFlag: frame[0] = 141; break;
        case somfy_commands::Flag: frame[0] = 142; break;
        default: break;
        }
    }
    else if(this->proto == radio_proto::RTV) {
        frame[1] = 0xF0;
        switch(this->cmd) {
        case somfy_commands::My: frame[0] = 149; break;
        case somfy_commands::Up: frame[0] = 150; break;
        case somfy_commands::MyUp: frame[0] = 151; break;
        case somfy_commands::Down: frame[0] = 152; break;
        case somfy_commands::MyDown: frame[0] = 153; break;
        case somfy_commands::UpDown: frame[0] = 154; break;
        case somfy_commands::MyUpDown: frame[0] = 155; break;
        case somfy_commands::Prog: frame[0] = 156; break;
        case somfy_commands::SunFlag: frame[0] = 157; break;
        case somfy_commands::Flag: frame[0] = 158; break;
        default: break;
        }
    }
    else {
        if(this->bitLength == 80) this->encode80BitFrame(&frame[0], this->repeats);
    }
    
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < 7; i++) {
        checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
    }
    checksum &= 0b1111;
    frame[1] |= checksum;
    
    for (uint8_t i = 1; i < 7; i++) {
        frame[i] ^= frame[i - 1];
    }
}

bool somfy_frame_t::isSynonym(somfy_frame_t &frame) { 
    return this->remoteAddress == frame.remoteAddress && this->cmd != frame.cmd && this->rollingCode == frame.rollingCode; 
}

bool somfy_frame_t::isRepeat(somfy_frame_t &frame) { 
    return this->remoteAddress == frame.remoteAddress && this->cmd == frame.cmd && this->rollingCode == frame.rollingCode; 
}

void somfy_frame_t::copy(somfy_frame_t &frame) {
    if(this->isRepeat(frame)) {
        this->repeats++;
        this->rssi = frame.rssi;
        this->lqi = frame.lqi;
    }
    else {
        this->synonym = this->isSynonym(frame);
        this->valid = frame.valid;
        if(!this->synonym) this->processed = frame.processed;
        this->rssi = frame.rssi;
        this->lqi = frame.lqi;
        this->cmd = frame.cmd;
        this->remoteAddress = frame.remoteAddress;
        this->rollingCode = frame.rollingCode;
        this->encKey = frame.encKey;
        this->checksum = frame.checksum;
        this->hwsync = frame.hwsync;
        this->repeats = frame.repeats;
    }
}

// ============================================
// QUEUE IMPLEMENTATIONS
// ============================================

void somfy_rx_queue_t::init() {
    Serial.println("Initializing RX Queue");
    for (uint8_t i = 0; i < MAX_RX_BUFFER; i++)
        this->items[i].clear();
    memset(&this->index[0], 0xFF, MAX_RX_BUFFER);
    this->length = 0;
}

bool somfy_rx_queue_t::pop(somfy_rx_t *rx) {
    for(int8_t i = MAX_RX_BUFFER - 1; i >= 0; i--) {
        if(this->index[i] < MAX_RX_BUFFER) {
            uint8_t ndx = this->index[i];
            memcpy(rx, &this->items[this->index[i]], sizeof(somfy_rx_t));
            this->items[ndx].clear();
            if(this->length > 0) this->length--;
            this->index[i] = 255;
            return true;
        }
    }
    return false;
}

void somfy_rx_queue_t::push(somfy_rx_t *rx) {
    if(this->length >= MAX_RX_BUFFER) {
        uint8_t ndx = this->index[MAX_RX_BUFFER - 1];
        if(ndx < MAX_RX_BUFFER) this->items[ndx].pulseCount = 0;
        this->index[MAX_RX_BUFFER - 1] = 255;
        this->length--;
    }
    
    uint8_t first = 0;
    for(uint8_t i = 0; i < MAX_RX_BUFFER; i++) {
        if(this->items[i].pulseCount == 0) {
            first = i;
            memcpy(&this->items[i], rx, sizeof(somfy_rx_t));
            break;
        }
    }
    
    for(uint8_t i = MAX_RX_BUFFER - 1; i > 0; i--) {
        this->index[i] = this->index[i - 1];
    }
    
    this->length++;
    this->index[0] = first;
}

bool somfy_tx_queue_t::pop(somfy_tx_t *tx) {
    for(int8_t i = MAX_TX_BUFFER - 1; i >= 0; i--) {
        if(this->index[i] < MAX_TX_BUFFER) {
            uint8_t ndx = this->index[i];
            memcpy(tx, &this->items[ndx], sizeof(somfy_tx_t));
            this->items[ndx].clear();
            if(this->length > 0) this->length--;
            this->index[i] = 255;
            return true;
        }
    }
    return false;
}

void somfy_tx_queue_t::push(somfy_rx_t *rx) {
    this->push(rx->cpt_synchro_hw, rx->payload, rx->bit_length);
}

void somfy_tx_queue_t::push(uint8_t hwsync, uint8_t *payload, uint8_t bit_length) {
    if(this->length >= MAX_TX_BUFFER) {
        uint8_t ndx = this->index[MAX_TX_BUFFER - 1];
        if(ndx < MAX_TX_BUFFER) this->items[ndx].clear();
        this->index[MAX_TX_BUFFER - 1] = 255;
        this->length--;
    }
    
    uint8_t first = 0;
    for(uint8_t i = 0; i < MAX_TX_BUFFER; i++) {
        if(this->items[i].bit_length == 0) {
            first = i;
            this->items[i].bit_length = bit_length;
            this->items[i].hwsync = hwsync;
            memcpy(&this->items[i].payload, payload, sizeof(this->items[i].payload));
            break;
        }
    }
    
    for(uint8_t i = MAX_TX_BUFFER - 1; i > 0; i--) {
        this->index[i] = this->index[i - 1];
    }
    
    this->length++;
    this->index[0] = first;
    this->delay_time = millis() + TX_QUEUE_DELAY;
}

// ============================================
// INTERRUPT HANDLER (CRITICAL - ORIGINAL CODE)
// ============================================

void RECEIVE_ATTR Transceiver::handleReceive() {
    static unsigned long last_time = 0;
    const long time = micros();
    const unsigned int duration = time - last_time;
    
    if (duration < bitMin) {
        if(somfy_rx.pulseCount < MAX_TIMINGS && somfy_rx.cpt_synchro_hw > 0) 
            somfy_rx.pulses[somfy_rx.pulseCount++] = duration;
        return;
    }
    
    last_time = time;
    
    switch (somfy_rx.status) {
    case waiting_synchro:
        if(somfy_rx.pulseCount < MAX_TIMINGS) 
            somfy_rx.pulses[somfy_rx.pulseCount++] = duration;
        
        if (duration > tempo_synchro_hw_min && duration < tempo_synchro_hw_max) {
            ++somfy_rx.cpt_synchro_hw;
        }
        else if (duration > tempo_synchro_sw_min && duration < tempo_synchro_sw_max && somfy_rx.cpt_synchro_hw >= 4) {
            memset(somfy_rx.payload, 0x00, sizeof(somfy_rx.payload));
            somfy_rx.previous_bit = 0x00;
            somfy_rx.waiting_half_symbol = false;
            somfy_rx.cpt_bits = 0;
            
            if (somfy_rx.cpt_synchro_hw <= 7) somfy_rx.bit_length = 56;
            else if (somfy_rx.cpt_synchro_hw == 14) somfy_rx.bit_length = 56;
            else if (somfy_rx.cpt_synchro_hw == 13) somfy_rx.bit_length = 80;
            else if (somfy_rx.cpt_synchro_hw == 12) somfy_rx.bit_length = 80;
            else if (somfy_rx.cpt_synchro_hw > 17) somfy_rx.bit_length = 80;
            else somfy_rx.bit_length = 56;
            
            somfy_rx.status = receiving_data;
        }
        else {
            somfy_rx.cpt_synchro_hw = 0;
            
            if(duration > tempo_wakeup_min && duration < tempo_wakeup_max) {
                memset(&somfy_rx.payload, 0x00, sizeof(somfy_rx.payload));
                somfy_rx.previous_bit = 0x00;
                somfy_rx.waiting_half_symbol = false;
                somfy_rx.cpt_bits = 0;
                somfy_rx.bit_length = 56;
            }
            else if((somfy_rx.pulseCount > 20 && somfy_rx.cpt_synchro_hw == 0) || duration > 250000) {
                somfy_rx.pulseCount = 0;
            }
        }
        break;
        
    case receiving_data:
        if(somfy_rx.pulseCount < MAX_TIMINGS) 
            somfy_rx.pulses[somfy_rx.pulseCount++] = duration;
        
        if (duration > tempo_symbol_min && duration < tempo_symbol_max && !somfy_rx.waiting_half_symbol) {
            somfy_rx.previous_bit = 1 - somfy_rx.previous_bit;
            somfy_rx.payload[somfy_rx.cpt_bits / 8] += somfy_rx.previous_bit << (7 - somfy_rx.cpt_bits % 8);
            ++somfy_rx.cpt_bits;
        }
        else if (duration > tempo_half_symbol_min && duration < tempo_half_symbol_max) {
            if (somfy_rx.waiting_half_symbol) {
                somfy_rx.waiting_half_symbol = false;
                somfy_rx.payload[somfy_rx.cpt_bits / 8] += somfy_rx.previous_bit << (7 - somfy_rx.cpt_bits % 8);
                ++somfy_rx.cpt_bits;
            }
            else {
                somfy_rx.waiting_half_symbol = true;
            }
        }
        else {
            memset(&somfy_rx.payload, 0x00, sizeof(somfy_rx.payload));
            somfy_rx.pulseCount = 1;
            somfy_rx.cpt_synchro_hw = 0;
            somfy_rx.previous_bit = 0x00;
            somfy_rx.waiting_half_symbol = false;
            somfy_rx.cpt_bits = 0;
            somfy_rx.bit_length = 56;
            somfy_rx.status = waiting_synchro;
            somfy_rx.pulses[0] = duration;
        }
        break;
        
    default:
        break;
    }
    
    if (somfy_rx.status == receiving_data && somfy_rx.cpt_bits >= somfy_rx.bit_length) {
        if(rx_queue.length >= MAX_RX_BUFFER) {
            uint8_t ndx = rx_queue.index[MAX_RX_BUFFER - 1];
            if(ndx < MAX_RX_BUFFER) rx_queue.items[ndx].pulseCount = 0;
            rx_queue.index[MAX_RX_BUFFER - 1] = 255;
            rx_queue.length--;
        }
        
        uint8_t first = 0;
        for(uint8_t i = 0; i < MAX_RX_BUFFER; i++) {
            if(rx_queue.items[i].pulseCount == 0) {
                first = i;
                memcpy(&rx_queue.items[i], &somfy_rx, sizeof(somfy_rx_t));
                rx_queue.items[i].timestamp = millis();
                rx_queue.items[i].rssi = ELECHOUSE_cc1101.getRssi();
                break;
            }
        }
        
        for(uint8_t i = MAX_RX_BUFFER - 1; i > 0; i--) {
            rx_queue.index[i] = rx_queue.index[i - 1];
        }
        
        rx_queue.length++;
        rx_queue.index[0] = first;
        
        memset(&somfy_rx.payload, 0x00, sizeof(somfy_rx.payload));
        somfy_rx.cpt_synchro_hw = 0;
        somfy_rx.previous_bit = 0x00;
        somfy_rx.waiting_half_symbol = false;
        somfy_rx.cpt_bits = 0;
        somfy_rx.pulseCount = 0;
        somfy_rx.status = waiting_synchro;
    }
}

// ============================================
// TRANSCEIVER CLASS IMPLEMENTATION
// ============================================

bool Transceiver::save() {
    this->config.save();
    this->config.apply();
    return true;
}

bool Transceiver::begin() {
    this->config.load();
    rx_queue.init();
    
    // WICHTIG: enabled muss true sein, sonst wird der Empfang nicht aktiviert
    if(!this->config.enabled) {
        Serial.println("[SKYEO] Radio war deaktiviert, aktiviere es...");
        this->config.enabled = true;
    }
    
    this->config.apply();
    return true;
}

void Transceiver::loop() {
    somfy_rx_t rx;
    if(this->receive(&rx)) {
        // Frame received - process it
        Somfy.processFrame(this->frame, false);
    }
    else {
        Somfy.processWaitingFrame();
    }
}

bool Transceiver::end() {
    this->disableReceive();
    return true;
}

bool Transceiver::receive(somfy_rx_t *rx) {
    if(rx_queue.length > 0) {
        rx_queue.pop(rx);
        this->frame.decodeFrame(rx);
        this->emitFrame(&this->frame, rx);
        return this->frame.valid;
    }
    return false;
}

void Transceiver::clearReceived(void) {
    if(this->config.enabled)
        attachInterrupt(interruptPin, handleReceive, CHANGE);
}

void Transceiver::enableReceive(void) {
    uint32_t timing = millis();
    if(rxmode > 0) return;
    if(this->config.enabled) {
        rxmode = 1;
        pinMode(this->config.RXPin, INPUT);
        interruptPin = digitalPinToInterrupt(this->config.RXPin);
        ELECHOUSE_cc1101.SetRx();
        attachInterrupt(interruptPin, handleReceive, CHANGE);
        Serial.printf("Enabled receive on Pin #%d Timing: %ld\n", this->config.RXPin, millis() - timing);
    }
}

void Transceiver::disableReceive(void) {
    rxmode = 0;
    if(interruptPin > 0) detachInterrupt(interruptPin);
    interruptPin = 0;
}

somfy_frame_t& Transceiver::lastFrame() { 
    return this->frame; 
}

void Transceiver::sendFrame(uint8_t *frame, uint8_t sync, uint8_t bitLength) {
    if(!this->config.enabled) return;
    
    uint32_t pin = 1 << this->config.TXPin;
    
    if (sync == 2 || sync == 12) {
        // Wake-up pulse
        REG_WRITE(GPIO_OUT_W1TS_REG, pin);
        delayMicroseconds(10920);
        REG_WRITE(GPIO_OUT_W1TC_REG, pin);
        delayMicroseconds(7357);
    }
    
    // Hardware sync
    for (int i = 0; i < sync; i++) {
        REG_WRITE(GPIO_OUT_W1TS_REG, pin);
        delayMicroseconds(4 * SYMBOL);
        REG_WRITE(GPIO_OUT_W1TC_REG, pin);
        delayMicroseconds(4 * SYMBOL);
    }
    
    // Software sync
    REG_WRITE(GPIO_OUT_W1TS_REG, pin);
    delayMicroseconds(4850);
    REG_WRITE(GPIO_OUT_W1TC_REG, pin);
    delayMicroseconds(SYMBOL);
    
    // Payload
    uint8_t last_bit = 0;
    for (uint8_t i = 0; i < bitLength; i++) {
        if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
            REG_WRITE(GPIO_OUT_W1TC_REG, pin);
            delayMicroseconds(SYMBOL);
            REG_WRITE(GPIO_OUT_W1TS_REG, pin);
            delayMicroseconds(SYMBOL);
            last_bit = 1;
        } else {
            REG_WRITE(GPIO_OUT_W1TS_REG, pin);
            delayMicroseconds(SYMBOL);
            REG_WRITE(GPIO_OUT_W1TC_REG, pin);
            delayMicroseconds(SYMBOL);
            last_bit = 0;
        }
    }
    
    if(last_bit == 0) {
        REG_WRITE(GPIO_OUT_W1TS_REG, pin);
    }
    
    REG_WRITE(GPIO_OUT_W1TC_REG, pin);
    
    if(bitLength != 80) {
        delayMicroseconds(13717);
        delayMicroseconds(13717);
    }
}

void Transceiver::beginTransmit() {
    if(this->config.enabled) {
        this->disableReceive();
        pinMode(this->config.TXPin, OUTPUT);
        digitalWrite(this->config.TXPin, 0);
        ELECHOUSE_cc1101.SetTx();
    }
}

void Transceiver::endTransmit() {
    if(this->config.enabled) {
        ELECHOUSE_cc1101.setSidle();
        this->enableReceive();
    }
}

void Transceiver::emitFrame(somfy_frame_t *frame, somfy_rx_t *rx) {
    // Frame wurde empfangen - hier könnte man WebSocket Events senden
    // Für Skyeo speichern wir es in der ReceivedFrame Liste
}

bool Transceiver::usesPin(uint8_t pin) {
    if(this->config.enabled) {
        if(this->config.SCKPin == pin ||
           this->config.TXPin == pin ||
           this->config.RXPin == pin ||
           this->config.MOSIPin == pin ||
           this->config.MISOPin == pin ||
           this->config.CSNPin == pin)
            return true;
    }
    return false;
}

// ============================================
// TRANSCEIVER CONFIG IMPLEMENTATION
// ============================================

void transceiver_config_t::save() {
    Preferences pref;
    pref.begin("CC1101");
    pref.clear();
    pref.putUChar("type", this->type);
    pref.putUChar("TXPin", this->TXPin);
    pref.putUChar("RXPin", this->RXPin);
    pref.putUChar("SCKPin", this->SCKPin);
    pref.putUChar("MOSIPin", this->MOSIPin);
    pref.putUChar("MISOPin", this->MISOPin);
    pref.putUChar("CSNPin", this->CSNPin);
    pref.putFloat("frequency", this->frequency);
    pref.putFloat("deviation", this->deviation);
    pref.putFloat("rxBandwidth", this->rxBandwidth);
    pref.putBool("enabled", this->enabled);
    pref.putBool("radioInit", true);
    pref.putChar("txPower", this->txPower);
    pref.putChar("proto", static_cast<uint8_t>(this->proto));
    pref.end();
    
    Serial.print("Save Radio Settings ");
    Serial.printf("SCK:%u MISO:%u MOSI:%u CSN:%u RX:%u TX:%u\n", 
                  this->SCKPin, this->MISOPin, this->MOSIPin, this->CSNPin, this->RXPin, this->TXPin);
}

void transceiver_config_t::removeNVSKey(const char *key) {
    Preferences pref;
    pref.begin("CC1101");
    if(pref.isKey(key)) {
        Serial.printf("Removing NVS Key: CC1101.%s\n", key);
        pref.remove(key);
    }
    pref.end();
}

void transceiver_config_t::load() {
    Preferences pref;
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    
    switch(ci.model) {
    case esp_chip_model_t::CHIP_ESP32S3:
        this->TXPin = 15;
        this->RXPin = 14;
        this->MOSIPin = 11;
        this->MISOPin = 13;
        this->SCKPin = 12;
        this->CSNPin = 10;
        break;
    case esp_chip_model_t::CHIP_ESP32S2:
        this->TXPin = 15;
        this->RXPin = 14;
        this->MOSIPin = 35;
        this->MISOPin = 37;
        this->SCKPin = 36;
        this->CSNPin = 34;
        break;
    case esp_chip_model_t::CHIP_ESP32C3:
        this->TXPin = 13;
        this->RXPin = 12;
        this->MOSIPin = 16;
        this->MISOPin = 17;
        this->SCKPin = 15;
        this->CSNPin = 14;
        break;
    default:
        this->TXPin = 13;
        this->RXPin = 12;
        this->MOSIPin = 23;
        this->MISOPin = 19;
        this->SCKPin = 18;
        this->CSNPin = 5;
        break;
    }
    
    pref.begin("CC1101");
    this->type = pref.getUChar("type", 56);
    this->TXPin = pref.getUChar("TXPin", this->TXPin);
    this->RXPin = pref.getUChar("RXPin", this->RXPin);
    this->SCKPin = pref.getUChar("SCKPin", this->SCKPin);
    this->MOSIPin = pref.getUChar("MOSIPin", this->MOSIPin);
    this->MISOPin = pref.getUChar("MISOPin", this->MISOPin);
    this->CSNPin = pref.getUChar("CSNPin", this->CSNPin);
    this->frequency = pref.getFloat("frequency", this->frequency);
    this->deviation = pref.getFloat("deviation", this->deviation);
    this->enabled = pref.getBool("enabled", this->enabled);
    this->txPower = pref.getChar("txPower", this->txPower);
    this->rxBandwidth = pref.getFloat("rxBandwidth", this->rxBandwidth);
    this->proto = static_cast<radio_proto>(pref.getChar("proto", static_cast<uint8_t>(this->proto)));
    pref.end();
}

void transceiver_config_t::apply() {
    // Diese Methode wird nicht verwendet - Initialisierung erfolgt in SomfyTransceiver::begin()
}

// ============================================
// DEBUG LED IMPLEMENTATION
// ============================================

const char* DebugLED::getPatternName(LedPattern p) {
    switch(p) {
    case LED_OFF: return "OFF";
    case LED_ON: return "ON";
    case LED_SLOW_BLINK: return "BLINK_SLOW";
    case LED_FAST_BLINK: return "BLINK_FAST";
    case LED_DOUBLE_BLINK: return "BLINK_PATTERN_DATA";
    case LED_TRIPLE_BLINK: return "BLINK_PATTERN_PAIRING";
    case LED_ERROR: return "BLINK_PATTERN_ERROR";
    case LED_WIFI_CONNECTING: return "BLINK_PATTERN_WIFI";
    case LED_WIFI_CONNECTED: return "BLINK_PATTERN_WIFI";
    default: return "OFF";
    }
}

void DebugLED::update() {
    if (!enabled) { 
        digitalWrite(pin, LOW); 
        return; 
    }
    
    uint32_t now = millis();
    uint32_t elapsed = now - lastUpdate;
    
    switch(currentPattern) {
    case LED_OFF: 
        digitalWrite(pin, LOW); 
        break;
    case LED_ON: 
        analogWrite(pin, brightness); 
        break;
    case LED_SLOW_BLINK: 
        if (elapsed > 1000) { 
            digitalWrite(pin, !digitalRead(pin)); 
            lastUpdate = now; 
        }
        break;
    case LED_FAST_BLINK:
        if (elapsed > 200) { 
            digitalWrite(pin, !digitalRead(pin)); 
            lastUpdate = now; 
        }
        break;
    case LED_DOUBLE_BLINK:
        if (elapsed > 1000) {
            digitalWrite(pin, HIGH); delay(100); digitalWrite(pin, LOW); delay(100);
            digitalWrite(pin, HIGH); delay(100); digitalWrite(pin, LOW);
            lastUpdate = now;
        }
        break;
    case LED_TRIPLE_BLINK:
        if (elapsed > 1500) {
            for(int i=0; i<3; i++) { 
                digitalWrite(pin, HIGH); delay(100); digitalWrite(pin, LOW); delay(100); 
            }
            lastUpdate = now;
        }
        break;
    case LED_ERROR:
        analogWrite(pin, brightness);
        break;
    case LED_WIFI_CONNECTING:
        if (elapsed > 30) {
            static uint8_t v=0, d=1;
            v += d*3;
            if(v>=brightness) d=-1;
            if(v==0) d=1;
            analogWrite(pin, v);
            lastUpdate = now;
        }
        break;
    case LED_WIFI_CONNECTED:
        if (elapsed > 3000) {
            digitalWrite(pin, HIGH); delay(50); digitalWrite(pin, LOW);
            lastUpdate = now;
        }
        break;
    }
}

// ============================================
// SOMFYTRANSCEIVER IMPLEMENTATION
// ============================================

SomfyTransceiver::SomfyTransceiver(uint8_t sclk, uint8_t csn, uint8_t mosi, uint8_t miso, uint8_t tx, uint8_t rx)
  : led(DEBUG_LED_PIN), transceiver(),
    sclkPin(sclk), csnPin(csn), mosiPin(mosi), misoPin(miso), txPin(tx), rxPin(rx),
    shadeCount(0), pairingMode(false), pairingShadeId(255), pairingStep(PAIR_NONE),
    receiveMode(false), lastRSSI(0), framesReceived(0), framesSent(0), 
    initialized(false), lastError("") {
}

bool SomfyTransceiver::begin() {
    Serial.println("[SKYEO] Initialisiere Somfy Transceiver...");
    
    led.begin();
    led.setPattern(LED_WIFI_CONNECTING);
    
    // Shades initialisieren
    for(uint8_t i = 0; i < MAX_SHADES; i++) {
        shades[i].id = 255;
        shades[i].moving = false;
    }
    
    // Transceiver initialisieren
    transceiver.config.SCKPin = sclkPin;
    transceiver.config.CSNPin = csnPin;
    transceiver.config.MOSIPin = mosiPin;
    transceiver.config.MISOPin = misoPin;
    transceiver.config.TXPin = txPin;
    transceiver.config.RXPin = rxPin;
    transceiver.config.enabled = true;
    
    Serial.printf("[SKYEO] Pins: SCK=%d, MISO=%d, MOSI=%d, CSN=%d, TX=%d, RX=%d\n",
                  sclkPin, misoPin, mosiPin, csnPin, txPin, rxPin);
    
    if(!transceiver.begin()) {
        Serial.println("[SKYEO] ✗ Transceiver Initialisierung fehlgeschlagen!");
        led.setPattern(LED_ERROR);
        return false;
    }
    
    // Nach dem Begin sicherstellen, dass unsere Pins und enabled Status gesetzt sind
    transceiver.config.SCKPin = sclkPin;
    transceiver.config.CSNPin = csnPin;
    transceiver.config.MOSIPin = mosiPin;
    transceiver.config.MISOPin = misoPin;
    transceiver.config.TXPin = txPin;
    transceiver.config.RXPin = rxPin;
    transceiver.config.enabled = true;
    
    // Empfang explizit aktivieren
    transceiver.enableReceive();
    
    Serial.println("[SKYEO] ✓ CC1101 initialisiert");
    Serial.println("[SKYEO] ✓ Empfangsmodus aktiv");
    
    led.setPattern(LED_WIFI_CONNECTED);
    initialized = true;
    return true;
}

void SomfyTransceiver::end() {
    transceiver.end();
    initialized = false;
}

void SomfyTransceiver::update() {
    led.update();
    transceiver.loop();
    
    if(pairingMode) {
        pairingLoop();
    }
    
    for(uint8_t i = 0; i < MAX_SHADES; i++) {
        if(shades[i].id != 255 && shades[i].moving) {
            updateShadePosition(i);
        }
    }
}

void SomfyTransceiver::enableReceive() {
    transceiver.enableReceive();
    receiveMode = true;
}

void SomfyTransceiver::disableReceive() {
    transceiver.disableReceive();
    receiveMode = false;
}

bool SomfyTransceiver::isReceiving() {
    return receiveMode;
}

std::vector<ReceivedFrame> SomfyTransceiver::getReceivedFrames() {
    return receivedFrames;
}

void SomfyTransceiver::clearReceivedFrames() {
    receivedFrames.clear();
}

AntennaStatus SomfyTransceiver::getAntennaStatus() {
    AntennaStatus status;
    status.initialized = initialized;
    status.version = ELECHOUSE_cc1101.getCC1101() ? 0x14 : 0;
    status.rssi = initialized ? ELECHOUSE_cc1101.getRssi() : 0;
    status.receiveMode = receiveMode;
    status.framesReceived = framesReceived;
    status.framesSent = framesSent;
    
    if(initialized) {
        strcpy(status.lastError, "OK");
    }
    else {
        strcpy(status.lastError, lastError.c_str());
    }
    
    return status;
}

bool SomfyTransceiver::checkAntennaConnection() {
    return ELECHOUSE_cc1101.getCC1101();
}

// ============================================
// FRAME PROCESSING
// ============================================

void SomfyTransceiver::processFrame(somfy_frame_t &frame, bool internal) {
    if(!frame.valid) return;
    
    // Konvertiere zu ReceivedFrame
    ReceivedFrame rf;
    rf.timestamp = millis();
    rf.remoteAddress = frame.remoteAddress;
    rf.rollingCode = frame.rollingCode;
    rf.command = static_cast<uint8_t>(frame.cmd);
    rf.rssi = frame.rssi;
    rf.valid = true;
    strcpy(rf.direction, internal ? "TX" : "RX");
    
    receivedFrames.push_back(rf);
    if(receivedFrames.size() > 50) {
        receivedFrames.erase(receivedFrames.begin());
    }
    
    framesReceived++;
    
    Serial.printf("[SKYEO] Frame %s: Addr=0x%06X, RC=%d, Cmd=%s, RSSI=%d dBm\n",
                  internal ? "gesendet" : "empfangen",
                  frame.remoteAddress, frame.rollingCode, 
                  translateSomfyCommand(frame.cmd).c_str(), frame.rssi);
    
    if(!internal) {
        led.setPattern(LED_DOUBLE_BLINK);
    }
    
    // Pairing-Modus prüfen
    if(pairingMode && frame.cmd == somfy_commands::Prog && !internal) {
        handlePairingFrame(rf);
    }
    
    // Shade-Status aktualisieren
    for(uint8_t i = 0; i < MAX_SHADES; i++) {
        if(shades[i].id != 255 && shades[i].remoteAddress == frame.remoteAddress) {
            // Update shade state based on command
            switch(frame.cmd) {
            case somfy_commands::Up:
                shades[i].moving = true;
                shades[i].direction = -1;
                shades[i].target = 0;
                shades[i].moveStartTime = millis();
                break;
            case somfy_commands::Down:
                shades[i].moving = true;
                shades[i].direction = 1;
                shades[i].target = 100;
                shades[i].moveStartTime = millis();
                break;
            case somfy_commands::My:
                shades[i].moving = false;
                shades[i].direction = 0;
                break;
            default:
                break;
            }
        }
    }
}

void SomfyTransceiver::processWaitingFrame() {
    // Verarbeite wartende Frames
}

// ============================================
// SEND COMMANDS
// ============================================

somfy_commands SomfyTransceiver::toSomfyCommand(SomfyCommand cmd) {
    return static_cast<somfy_commands>(cmd);
}

SomfyCommand SomfyTransceiver::toLegacyCommand(somfy_commands cmd) {
    return static_cast<SomfyCommand>(cmd);
}

void SomfyTransceiver::buildFrame(uint8_t shadeId, SomfyCommand cmd, uint8_t* frame) {
    ShadeState* shade = &shades[shadeId];
    
    frame[0] = (shade->remoteAddress >> 16) & 0xFF;
    frame[1] = (shade->remoteAddress >> 8) & 0xFF;
    frame[2] = shade->remoteAddress & 0xFF;
    frame[3] = (shade->rollingCode >> 8) & 0xFF;
    frame[4] = shade->rollingCode & 0xFF;
    frame[5] = cmd & 0x0F;
    
    uint8_t checksum = frame[0] ^ frame[1] ^ frame[2] ^ frame[3] ^ frame[4] ^ frame[5];
    frame[5] |= (checksum & 0xF0);
}

void SomfyTransceiver::sendRawFrame(uint8_t* frame, uint8_t len) {
    bool wasReceiving = receiveMode;
    if(wasReceiving) {
        disableReceive();
    }
    
    // Wakeup-Pulse
    digitalWrite(txPin, HIGH);
    delayMicroseconds(9415);
    digitalWrite(txPin, LOW);
    delayMicroseconds(89565);
    
    // Hardware Sync
    for(uint8_t i = 0; i < 2; i++) {
        digitalWrite(txPin, HIGH);
        delayMicroseconds(2560);
        digitalWrite(txPin, LOW);
        delayMicroseconds(2560);
    }
    
    // Software Sync
    digitalWrite(txPin, HIGH);
    delayMicroseconds(4850);
    digitalWrite(txPin, LOW);
    delayMicroseconds(SYMBOL);
    
    // Daten senden
    for(uint8_t i = 0; i < len; i++) {
        for(int8_t j = 7; j >= 0; j--) {
            uint8_t bit = (frame[i] >> j) & 0x01;
            if(bit) {
                digitalWrite(txPin, LOW);
                delayMicroseconds(SYMBOL);
                digitalWrite(txPin, HIGH);
                delayMicroseconds(SYMBOL);
            }
            else {
                digitalWrite(txPin, HIGH);
                delayMicroseconds(SYMBOL);
                digitalWrite(txPin, LOW);
                delayMicroseconds(SYMBOL);
            }
        }
    }
    
    digitalWrite(txPin, LOW);
    framesSent++;
    led.setPattern(LED_DOUBLE_BLINK);
    
    if(wasReceiving) {
        delay(10);
        enableReceive();
    }
}

bool SomfyTransceiver::sendCommand(uint8_t shadeId, SomfyCommand cmd) {
    return sendCommand(shadeId, cmd, 2);
}

bool SomfyTransceiver::sendCommand(uint8_t shadeId, SomfyCommand cmd, uint8_t repeats) {
    if(shadeId >= MAX_SHADES || shades[shadeId].id == 255) {
        return false;
    }
    
    ShadeState* shade = &shades[shadeId];
    
    // Verwende die Original Frame-Encodierung
    somfy_frame_t frame;
    frame.remoteAddress = shade->remoteAddress;
    frame.rollingCode = shade->rollingCode;
    frame.cmd = toSomfyCommand(cmd);
    frame.bitLength = 56;
    frame.encKey = 0xA0 | static_cast<uint8_t>(frame.rollingCode & 0x000F);
    frame.proto = radio_proto::RTS;
    frame.valid = true;
    frame.repeats = 0;
    
    uint8_t encodedFrame[10];
    frame.encodeFrame(encodedFrame);
    
    Serial.printf("[SKYEO] Sende Befehl %s an Shade %d (0x%06X), RC=%d\n", 
                  translateSomfyCommand(frame.cmd).c_str(),
                  shadeId, shade->remoteAddress, shade->rollingCode);
    
    // Sende mit Original-Transceiver
    transceiver.beginTransmit();
    transceiver.sendFrame(encodedFrame, 2, 56);  // Erster Frame mit sync=2
    
    for(uint8_t r = 0; r < repeats; r++) {
        transceiver.sendFrame(encodedFrame, 7, 56);  // Wiederholungen mit sync=7
    }
    
    transceiver.endTransmit();
    
    // Rolling Code erhöhen
    shade->rollingCode++;
    saveShadeState(shadeId);
    
    // Shade Status aktualisieren
    if(cmd == SOMFY_UP) {
        shade->moving = true;
        shade->direction = -1;
        shade->target = 0;
        shade->moveStartTime = millis();
    }
    else if(cmd == SOMFY_DOWN) {
        shade->moving = true;
        shade->direction = 1;
        shade->target = 100;
        shade->moveStartTime = millis();
    }
    else if(cmd == SOMFY_MY) {
        shade->moving = false;
        shade->direction = 0;
    }
    
    // Frame verarbeiten (intern)
    processFrame(frame, true);
    
    return true;
}

bool SomfyTransceiver::sendPosition(uint8_t shadeId, uint8_t position) {
    if(position == 0) {
        return sendCommand(shadeId, SOMFY_UP);
    }
    else if(position == 100) {
        return sendCommand(shadeId, SOMFY_DOWN);
    }
    else {
        uint8_t currentPos = shades[shadeId].position;
        if(position > currentPos) {
            return sendCommand(shadeId, SOMFY_DOWN);
        }
        else {
            return sendCommand(shadeId, SOMFY_UP);
        }
    }
}

void SomfyTransceiver::sendFrame(uint8_t *frame, uint8_t sync, uint8_t bitLength) {
    transceiver.sendFrame(frame, sync, bitLength);
}

// ============================================
// SHADE MANAGEMENT
// ============================================

bool SomfyTransceiver::addShade(uint8_t id, const char* name, uint32_t remoteAddr, uint16_t upTime, uint16_t downTime) {
    if(id >= MAX_SHADES) return false;
    
    shades[id].id = id;
    strncpy(shades[id].name, name, sizeof(shades[id].name) - 1);
    shades[id].name[sizeof(shades[id].name) - 1] = '\0';
    shades[id].remoteAddress = remoteAddr;
    shades[id].upTime = upTime;
    shades[id].downTime = downTime;
    shades[id].position = 50;
    shades[id].target = 50;
    shades[id].rollingCode = 1;
    shades[id].moving = false;
    shades[id].direction = 0;
    
    shadeCount++;
    
    ShadeConfig cfg;
    cfg.id = id;
    strncpy(cfg.name, name, sizeof(cfg.name) - 1);
    cfg.remoteAddress = remoteAddr;
    cfg.upTime = upTime;
    cfg.downTime = downTime;
    cfg.rollingCode = 1;
    cfg.type = SHADE_ROLLER;
    cfg.enabled = true;
    
    Config::saveShade(cfg);
    
    Serial.printf("[SKYEO] Shade %d hinzugefügt: %s (0x%06X)\n", id, name, remoteAddr);
    return true;
}

bool SomfyTransceiver::removeShade(uint8_t id) {
    if(id >= MAX_SHADES || shades[id].id == 255) return false;
    
    shades[id].id = 255;
    shadeCount--;
    
    Config::deleteShade(id);
    
    Serial.printf("[SKYEO] Shade %d entfernt\n", id);
    return true;
}

ShadeState* SomfyTransceiver::getShade(uint8_t id) {
    if(id >= MAX_SHADES || shades[id].id == 255) return nullptr;
    return &shades[id];
}

uint8_t SomfyTransceiver::getShadeCount() {
    return shadeCount;
}

void SomfyTransceiver::updateShadePosition(uint8_t shadeId) {
    ShadeState* shade = &shades[shadeId];
    if(!shade->moving) return;
    
    uint32_t elapsed = millis() - shade->moveStartTime;
    uint16_t travelTime = (shade->direction > 0) ? shade->downTime : shade->upTime;
    
    if(elapsed >= travelTime) {
        shade->position = shade->target;
        shade->moving = false;
        shade->direction = 0;
    }
    else {
        uint8_t travelPercent = (elapsed * 100) / travelTime;
        if(shade->direction > 0) {
            shade->position = min((uint8_t)(shade->position + travelPercent), (uint8_t)100);
        }
        else {
            shade->position = (shade->position > travelPercent) ? (shade->position - travelPercent) : 0;
        }
    }
}

void SomfyTransceiver::stop(uint8_t shadeId) {
    if(shadeId >= MAX_SHADES || shades[shadeId].id == 255) return;
    
    shades[shadeId].moving = false;
    shades[shadeId].direction = 0;
    sendCommand(shadeId, SOMFY_MY);
}

void SomfyTransceiver::setMyPosition(uint8_t shadeId, uint8_t position) {
    if(shadeId >= MAX_SHADES || shades[shadeId].id == 255) return;
    shades[shadeId].position = position;
}

void SomfyTransceiver::saveShadeState(uint8_t shadeId) {
    ShadeConfig cfg;
    if(Config::loadShade(shadeId, cfg)) {
        cfg.rollingCode = shades[shadeId].rollingCode;
        Config::saveShade(cfg);
    }
}

// ============================================
// PAIRING
// ============================================

bool SomfyTransceiver::startPairing(uint8_t shadeId) {
    if(shadeId >= MAX_SHADES || shades[shadeId].id == 255) return false;
    
    pairingMode = true;
    pairingShadeId = shadeId;
    pairingStep = PAIR_WAIT_PROG;
    pairingStartTime = millis();
    
    Serial.printf("[SKYEO] Pairing gestartet für Shade %d\n", shadeId);
    led.setPattern(LED_TRIPLE_BLINK);
    
    return true;
}

void SomfyTransceiver::stopPairing() {
    pairingMode = false;
    pairingShadeId = 255;
    pairingStep = PAIR_NONE;
    
    Serial.println("[SKYEO] Pairing gestoppt");
    led.setPattern(LED_WIFI_CONNECTED);
}

void SomfyTransceiver::pairingLoop() {
    if(!pairingMode) return;
    
    if(millis() - pairingStartTime > 60000) {
        pairingStep = PAIR_FAILED;
        stopPairing();
    }
}

void SomfyTransceiver::handlePairingFrame(ReceivedFrame& frame) {
    if(pairingStep == PAIR_WAIT_PROG) {
        ShadeState* shade = &shades[pairingShadeId];
        shade->remoteAddress = frame.remoteAddress;
        shade->rollingCode = frame.rollingCode + 1;
        
        ShadeConfig cfg;
        if(Config::loadShade(pairingShadeId, cfg)) {
            cfg.remoteAddress = frame.remoteAddress;
            cfg.rollingCode = shade->rollingCode;
            Config::saveShade(cfg);
        }
        
        Serial.printf("[SKYEO] Adresse erkannt: 0x%06X\n", frame.remoteAddress);
        
        delay(100);
        sendCommand(pairingShadeId, SOMFY_PROG);
        
        pairingStep = PAIR_SUCCESS;
        
        delay(2000);
        stopPairing();
    }
}

PairingStep SomfyTransceiver::getPairingStep() {
    return pairingStep;
}

String SomfyTransceiver::getPairingInstructions() {
    switch(pairingStep) {
    case PAIR_NONE: return "Bereit";
    case PAIR_WAIT_PROG: return "Schritt 1: Halte PROG-Taste an der Fernbedienung 3 Sekunden";
    case PAIR_SEND_PROG: return "Schritt 2: Sende PROG-Signal...";
    case PAIR_WAIT_CONFIRM: return "Schritt 3: Drücke PROG-Taste kurz zur Bestätigung";
    case PAIR_SUCCESS: return "Pairing erfolgreich!";
    case PAIR_FAILED: return "Pairing fehlgeschlagen - Timeout";
    default: return "Unbekannter Status";
    }
}

bool SomfyTransceiver::isPairingMode() {
    return pairingMode;
}

// ============================================
// CONFIGURATION
// ============================================

void SomfyTransceiver::setPins(uint8_t sclk, uint8_t csn, uint8_t mosi, uint8_t miso, uint8_t tx, uint8_t rx) {
    sclkPin = sclk;
    csnPin = csn;
    mosiPin = mosi;
    misoPin = miso;
    txPin = tx;
    rxPin = rx;
    
    transceiver.config.SCKPin = sclk;
    transceiver.config.CSNPin = csn;
    transceiver.config.MOSIPin = mosi;
    transceiver.config.MISOPin = miso;
    transceiver.config.TXPin = tx;
    transceiver.config.RXPin = rx;
}

void SomfyTransceiver::getPins(uint8_t &sclk, uint8_t &csn, uint8_t &mosi, uint8_t &miso, uint8_t &tx, uint8_t &rx) {
    sclk = sclkPin;
    csn = csnPin;
    mosi = mosiPin;
    miso = misoPin;
    tx = txPin;
    rx = rxPin;
}

void SomfyTransceiver::savePinConfig() {
    Preferences prefs;
    prefs.begin("skyeo", false);
    prefs.putUChar("pin_sclk", sclkPin);
    prefs.putUChar("pin_csn", csnPin);
    prefs.putUChar("pin_mosi", mosiPin);
    prefs.putUChar("pin_miso", misoPin);
    prefs.putUChar("pin_tx", txPin);
    prefs.putUChar("pin_rx", rxPin);
    prefs.end();
}

bool SomfyTransceiver::loadPinConfig() {
    Preferences prefs;
    prefs.begin("skyeo", true);
    
    if(!prefs.isKey("pin_sclk")) {
        prefs.end();
        return false;
    }
    
    sclkPin = prefs.getUChar("pin_sclk", 18);
    csnPin = prefs.getUChar("pin_csn", 5);
    mosiPin = prefs.getUChar("pin_mosi", 23);
    misoPin = prefs.getUChar("pin_miso", 19);
    txPin = prefs.getUChar("pin_tx", 17);
    rxPin = prefs.getUChar("pin_rx", 16);
    
    prefs.end();
    
    transceiver.config.SCKPin = sclkPin;
    transceiver.config.CSNPin = csnPin;
    transceiver.config.MOSIPin = mosiPin;
    transceiver.config.MISOPin = misoPin;
    transceiver.config.TXPin = txPin;
    transceiver.config.RXPin = rxPin;
    
    return true;
}

// ============================================
// LED API
// ============================================

void SomfyTransceiver::setLEDPattern(const String& pattern) {
    if(pattern == "OFF") led.setPattern(LED_OFF);
    else if(pattern == "ON") led.setPattern(LED_ON);
    else if(pattern == "BLINK_SLOW") led.setPattern(LED_SLOW_BLINK);
    else if(pattern == "BLINK_FAST") led.setPattern(LED_FAST_BLINK);
    else if(pattern == "BLINK_PATTERN_WIFI") led.setPattern(LED_WIFI_CONNECTING);
    else if(pattern == "BLINK_PATTERN_DATA") led.setPattern(LED_DOUBLE_BLINK);
    else if(pattern == "BLINK_PATTERN_PAIRING") led.setPattern(LED_TRIPLE_BLINK);
    else if(pattern == "BLINK_PATTERN_ERROR") led.setPattern(LED_ERROR);
}

String SomfyTransceiver::getLEDCurrentPattern() {
    LedPattern p = led.getPattern();
    switch(p) {
    case LED_OFF: return "OFF";
    case LED_ON: return "ON";
    case LED_SLOW_BLINK: return "BLINK_SLOW";
    case LED_FAST_BLINK: return "BLINK_FAST";
    case LED_WIFI_CONNECTING: return "BLINK_PATTERN_WIFI";
    case LED_WIFI_CONNECTED: return "BLINK_PATTERN_WIFI";
    case LED_DOUBLE_BLINK: return "BLINK_PATTERN_DATA";
    case LED_TRIPLE_BLINK: return "BLINK_PATTERN_PAIRING";
    case LED_ERROR: return "BLINK_PATTERN_ERROR";
    default: return "OFF";
    }
}

// ============================================
// MISSING METHODS FOR API COMPATIBILITY
// ============================================

uint8_t SomfyTransceiver::getPosition(uint8_t id) {
    if(id >= MAX_SHADES || shades[id].id == 255) return 0;
    return shades[id].position;
}

bool SomfyTransceiver::isMoving(uint8_t id) {
    if(id >= MAX_SHADES || shades[id].id == 255) return false;
    return shades[id].moving;
}
