#include <Midea.h>

MideaAC::MideaAC() {
    _serial = nullptr;
    _bufferPos = 0;
    _lastUpdate = 0;
    _callback = nullptr;
    
    memset(&_status, 0, sizeof(ac_status_t));
    _status.targetTemp = 24.0;
    _status.fanSpeed = FAN_AUTO;
}

void MideaAC::begin(Stream* serial) {
    _serial = serial;
}

void MideaAC::loop() {
    if (!_serial) return;
    
    // Read incoming data
    while (_serial->available()) {
        uint8_t byte = _serial->read();
        
        if (_bufferPos == 0 && byte != 0xAA) {
            continue; // Wait for start byte
        }
        
        _buffer[_bufferPos++] = byte;
        
        // Check if we have complete packet
        if (_bufferPos >= 3) {
            uint8_t expectedLen = _buffer[1];
            if (_bufferPos >= expectedLen) {
                processPacket();
                _bufferPos = 0;
            }
        }
        
        if (_bufferPos >= 256) {
            _bufferPos = 0; // Reset on overflow
        }
    }
    
    // Send status request every 5 seconds
    if (millis() - _lastUpdate > 5000) {
        sendCommand();
        _lastUpdate = millis();
    }
}

void MideaAC::processPacket() {
    // Verify checksum
    uint8_t len = _buffer[1];
    uint8_t checksum = calculateChecksum(_buffer, len - 1);
    
    if (checksum != _buffer[len - 1]) {
        Serial1.println("Checksum error");
        return;
    }
    
    // Parse packet type
    uint8_t type = _buffer[2];
    
    if (type == 0xC0 || type == 0xC1) {
        // Status response
        parseStatus(_buffer);
        
        if (_callback) {
            _callback(&_status);
        }
    }
}

void MideaAC::parseStatus(uint8_t* data) {
    _status.power = (data[11] & 0x01) != 0;
    _status.mode = (data[12] >> 5) & 0x07;
    _status.targetTemp = (data[12] & 0x0F) + 16.0;
    _status.fanSpeed = (data[13] >> 5) & 0x07;
    _status.swingMode = (data[17] & 0x0F);
    _status.turboMode = (data[18] & 0x20) != 0;
    _status.ecoMode = (data[19] & 0x10) != 0;
    
    // Indoor temperature (if available)
    if (data[11] & 0x04) {
        _status.indoorTemp = ((data[11] >> 3) & 0x0F) + (data[26] - 50) / 2.0;
    }
}

void MideaAC::sendCommand() {
    if (!_serial) return;
    
    uint8_t cmd[] = {
        0xAA, 0x21, 0xAC, 0x8D, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x03, 0x41, 0x81, 0x00, 0xFF, 0x03, 0xFF,
        0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x00
    };
    
    // Set power
    if (_status.power) {
        cmd[11] |= 0x01;
    }
    
    // Set mode
    cmd[12] = (_status.mode << 5) | ((uint8_t)(_status.targetTemp - 16) & 0x0F);
    
    // Set fan speed
    cmd[13] = _status.fanSpeed << 5;
    
    // Set swing mode
    cmd[17] = _status.swingMode;
    
    // Set turbo
    if (_status.turboMode) {
        cmd[18] |= 0x20;
    }
    
    // Set eco
    if (_status.ecoMode) {
        cmd[19] |= 0x10;
    }
    
    // Calculate checksum
    cmd[32] = calculateChecksum(cmd, 32);
    
    _serial->write(cmd, 34);
}

uint8_t MideaAC::calculateChecksum(uint8_t* data, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 1; i < len; i++) {
        checksum += data[i];
    }
    return (~checksum) + 1;
}

void MideaAC::setPower(bool power) {
    _status.power = power;
    sendCommand();
}

void MideaAC::setMode(uint8_t mode) {
    _status.mode = mode;
    sendCommand();
}

void MideaAC::setTemp(float temp) {
    if (temp >= 17 && temp <= 30) {
        _status.targetTemp = temp;
        sendCommand();
    }
}

void MideaAC::setFanSpeed(uint8_t speed) {
    _status.fanSpeed = speed;
    sendCommand();
}

void MideaAC::setSwingMode(uint8_t mode) {
    _status.swingMode = mode;
    sendCommand();
}

void MideaAC::setTurbo(bool turbo) {
    _status.turboMode = turbo;
    sendCommand();
}

void MideaAC::setEco(bool eco) {
    _status.ecoMode = eco;
    sendCommand();
}

void MideaAC::onStatus(acSerialEvent callback) {
    _callback = callback;
}

ac_status_t* MideaAC::getStatus() {
    return &_status;
}
