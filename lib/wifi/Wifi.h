#ifndef WIFI_H
#define WIFI_H

#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <Filesys.h>

class WifiClass {
    private:
        // Constants
        static const unsigned long RECONNECT_DELAY = 10000;     // 10 seconds
        static const unsigned long WIFI_TIMEOUT = 1200000;       // 120 seconds
        static const int MAX_WIFI_ATTEMPTS = 5;
        static const int WEAK_SIGNAL_THRESHOLD = -80;           // dBm

        AsyncWebServer* server;
        std::function<void(bool)> alexaCallback;
        String ssid;
        String pass;
        const char* ssidPath = "/ssid.txt";
        const char* passPath = "/pass.txt";
        
        // Connection state
        unsigned long wifiConnectStartTime = 0;
        unsigned long lastDisconnectTime = 0;
        int connectionAttempts = 0;
        bool shouldReconnect = false;
        
        // Event handlers
        WiFiEventHandler wifiConnectHandler;
        WiFiEventHandler wifiDisconnectHandler;
        
        // Private methods
        void connectToWiFi();
        void onWifiConnect(const WiFiEventStationModeGotIP& event);
        void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
        void switchToAPMode();
        void switchToStaMode();
        String getBestBSSID();
        void convertBSSIDStringToBytes(const String& bssidStr, uint8_t* bssidBytes);
        
    public:
        void initWiFi(AsyncWebServer* server);
        void handleWiFiReconnection();
        wl_status_t getStatus() { return WiFi.status(); }
        String getLocalIP() { return WiFi.localIP().toString(); }
        int getRSSI() { return WiFi.RSSI(); }
};

extern WifiClass Wifi;

#endif
