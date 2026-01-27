#ifndef MIDEA_H
#define MIDEA_H

#include <Arduino.h>
#include <functional>

typedef struct {
    bool power;
    uint8_t mode;
    float targetTemp;
    float indoorTemp;
    float outdoorTemp;
    uint8_t fanSpeed;
    uint8_t swingMode;
    bool turboMode;
    bool ecoMode;
} ac_status_t;

typedef std::function<void(ac_status_t* status)> acSerialEvent;

class MideaAC {
public:
    MideaAC();
    void begin(Stream* serial);
    void loop();
    
    // Control methods
    void setPower(bool power);
    void setMode(uint8_t mode);
    void setTemp(float temp);
    void setFanSpeed(uint8_t speed);
    void setSwingMode(uint8_t mode);
    void setTurbo(bool turbo);
    void setEco(bool eco);
    
    // Callback
    void onStatus(acSerialEvent callback);
    
    // Get current status
    ac_status_t* getStatus();

private:
    Stream* _serial;
    ac_status_t _status;
    acSerialEvent _callback;
    
    uint8_t _buffer[256];
    uint8_t _bufferPos;
    unsigned long _lastUpdate;
    
    void processPacket();
    void sendCommand();
    uint8_t calculateChecksum(uint8_t* data, uint8_t len);
    void parseStatus(uint8_t* data);
};

// Mode constants
#define MODE_AUTO       0
#define MODE_COOL       1
#define MODE_DRY        2
#define MODE_HEAT       3
#define MODE_FAN        4

// Fan speed constants
#define FAN_AUTO        0
#define FAN_LOW         1
#define FAN_MEDIUM      2
#define FAN_HIGH        3

// Swing mode constants
#define SWING_OFF       0
#define SWING_VERTICAL  1
#define SWING_HORIZONTAL 2
#define SWING_BOTH      3

#endif
