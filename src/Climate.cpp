#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Filesys.h>
#include <Wifi.h>
#include <Appliance/AirConditioner/AirConditioner.h>
#include <SoftwareSerial.h>
#include <ESP8266mDNS.h>

#define MIDEA_TX_PIN 12  // D6
#define MIDEA_RX_PIN 14  // D5

using namespace dudanov::midea::ac;

const char* VERSION = "1.0.4";
const char* devNamePath = "/dev_name.txt";

// AsyncWebServer on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

// ESP restart - STATE MACHINE
enum RestartState {
  RESTART_IDLE,
  RESTART_DELAYING,
  RESTARTING
};
RestartState restartState = RESTART_IDLE;
unsigned long restartStartTime = 0;
const unsigned long RESTART_DELAY_MS = 5000;

// WiFi Setup - STATE MACHINE  
enum WifiState {
  WIFI_SETUP_IDLE,
  WIFI_SETUP_DELAYING,
  WIFI_SETUP_RESTARTING
};
WifiState wifiState = WIFI_SETUP_IDLE;
unsigned long wifiStartTime = 0;
const unsigned long WIFI_DELAY_MS = 2000; 

boolean restart = false;

// MideaAC
AirConditioner ac;
SoftwareSerial mideaSerial(MIDEA_RX_PIN, MIDEA_TX_PIN);

void initAsyncWebServer();
void onAcStateChange();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void webLog(String message); 
void initmDNS();
void updateRestartState();
void updateWifiState();

void setup() {
    // Initialize File System
    Filesys.initFS();
    
    // Initialize WiFi
    Wifi.initWiFi(&server);

    // MideaUART
    mideaSerial.begin(9600);
    ac.setStream(&mideaSerial);
    ac.addOnStateCallback(onAcStateChange);
    ac.setup();

    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Initialize OTA
    ElegantOTA.begin(&server);
    
    // Initialize Web Server 
    initAsyncWebServer();
    
    // Initialize mDNS
    initmDNS();

    webLog("MideaUART initialized");
    webLog("AC Controller Ready!");
}

void loop() {
    // Always handle these first - non-blocking
    Wifi.handleWiFiReconnection();
    ElegantOTA.loop();
    MDNS.update();
    ws.cleanupClients();  
    
    // non-blocking state machine
    updateWifiState();
    updateRestartState();
    
    ac.loop();
}

void updateWifiState() {
    unsigned long now = millis();
    
    switch(wifiState) {
        case WIFI_SETUP_IDLE:
            // Waiting for WiFi setup trigger (handled in web handler)
            break;
            
        case WIFI_SETUP_DELAYING:
            if (now - wifiStartTime >= WIFI_DELAY_MS) {
                wifiState = WIFI_SETUP_RESTARTING;
            }
            break;
            
        case WIFI_SETUP_RESTARTING:
            ESP.restart();  // Never reached, but keeps state clean
            wifiState = WIFI_SETUP_IDLE;
            break;
    }
}

void updateRestartState() {
    unsigned long now = millis();
    
    switch(restartState) {
        case RESTART_IDLE:
            if (restart) {
                restartState = RESTART_DELAYING;
                restartStartTime = now;
            }
            break;
            
        case RESTART_DELAYING:
            if (now - restartStartTime >= RESTART_DELAY_MS) {
                restartState = RESTARTING;
            }
            break;
            
        case RESTARTING:
            ESP.restart();  // execute immediately
            break;
    }
}

void initmDNS() {
    String devName = Filesys.readFirstLine(devNamePath);
    if (devName.length() == 0) {
        devName = "midea-ac";
        Filesys.writeFile(devNamePath, devName.c_str());
    }
    
    if (MDNS.begin(devName.c_str())) {
        MDNS.addService("http", "tcp", 80); 
        webLog("mDNS started: http://" + devName + ".local/");
    } else {
        webLog("Error starting mDNS");
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        client->text("Connected to AC Controller");
    }
}

void webLog(String message) {
    if (ws.count() > 0) {
        ws.textAll(message);
    }
}

void initAsyncWebServer() {
    // Dynamic root handler
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.getMode() == WIFI_AP || WiFi.status() != WL_CONNECTED) {
            request->send(LittleFS, "/wifi_setup.html", "text/html");
        } else {
            request->send(LittleFS, "/index.html", "text/html");
        }
    });

    // Console page
    server.on("/console", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/console.html", "text/html");
    });

    // Firmware version
    server.on("/api/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"version\":\"" + String(VERSION) + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool powerState = ac.getPowerState();
        String json = "{";
        json += "\"power\":" + String(powerState ? "true" : "false") + ",";
        json += "\"state\":\"" + String(powerState ? "ON" : "OFF") + "\",";
        json += "\"mode\":" + String((int)ac.getMode()) + ",";
        json += "\"temp\":" + String(ac.getTargetTemp()) + ",";
        json += "\"indoor_temp\":" + String(ac.getIndoorTemp()) + ",";
        json += "\"outdoor_temp\":" + String(ac.getOutdoorTemp()) + ",";
        json += "\"fan\":" + String((int)ac.getFanMode()) + ",";
        json += "\"swing\":" + String((int)ac.getSwingMode()) + ",";
        json += "\"turbo\":" + String(ac.getPreset() == Preset::PRESET_TURBO ? "true" : "false") + ",";
        json += "\"eco\":" + String(ac.getPreset() == Preset::PRESET_FREEZE_PROTECTION ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("^\\/api\\/state\\/(ON|OFF)$", HTTP_PUT, [](AsyncWebServerRequest *request) {
        String state = request->pathArg(0);
        bool powerOn = (state == "ON");
        
        webLog("API command: Power " + state);
        ac.setPowerState(powerOn);
        
        String json = "{";
        json += "\"success\":true,";
        json += "\"state\":\"" + state + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        String newSSID = "", newPass = "", devName = "";
        
        int params = request->params();
        for(int i = 0; i < params; i++){
            const AsyncWebParameter* p = request->getParam(i);
            if(p->isPost()){
                if (p->name() == "ssid") {
                    newSSID = p->value();
                    Filesys.writeFile("/ssid.txt", newSSID.c_str());
                }
                if (p->name() == "pass") {
                    newPass = p->value();
                    Filesys.writeFile("/pass.txt", newPass.c_str());
                }
                if (p->name() == "dev_name") {
                    devName = p->value();
                    Filesys.writeFile("/dev_name.txt", devName.c_str());
                }                
            }
        }
        
        request->send(LittleFS, "/wifi_setup_success.html", "text/html");
        
        wifiState = WIFI_SETUP_DELAYING;
        wifiStartTime = millis();
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"wifi_mode\":\"" + String(WiFi.getMode() == WIFI_AP ? "AP" : "STA") + "\",";
        json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    server.serveStatic("/", LittleFS, "/");
    server.begin();
    webLog("Web server started");
}

void onAcStateChange() {
    webLog("=== AC Status Callback Triggered ===");
    webLog("Power: " + String(ac.getPowerState() ? "ON" : "OFF"));
    webLog("Mode: " + String((int)ac.getMode()));
    webLog("Target Temp: " + String(ac.getTargetTemp()) + "°C");
    webLog("Indoor Temp: " + String(ac.getIndoorTemp()) + "°C");
    webLog("Outdoor Temp: " + String(ac.getOutdoorTemp()) + "°C");
    webLog("Fan Mode: " + String((int)ac.getFanMode()));
    webLog("Swing Mode: " + String((int)ac.getSwingMode()));
}
