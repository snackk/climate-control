#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Filesys.h>
#include <Wifi.h>
#include <Appliance/AirConditioner/AirConditioner.h>

using namespace dudanov::midea::ac;

const char* VERSION = "1.0.3";

// AsyncWebServer on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

// ESP restart
boolean restart = false;

// MideaAC
AirConditioner ac;

void initAsyncWebServer();
void onAcStateChange();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void webLog(String message); 

void setup() {
    // Initialize Serial port - 9600 for Midea
    Serial.begin(9600);

    // Initialize File System
    Filesys.initFS();
    
    // Initialize WiFi
    Wifi.initWiFi(&server);

    // MideaUART
    ac.setStream(&Serial);
    ac.addOnStateCallback(onAcStateChange); 
    ac.setup();

    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Initialize OTA
    ElegantOTA.begin(&server);
    
    // Initialize Web Server 
    initAsyncWebServer();

    webLog("MideaUART initialized");
    webLog("AC Controller Ready!");
}

void loop() {
    Wifi.handleWiFiReconnection();
    ElegantOTA.loop();
    // Clean old WebSocket connections
    ws.cleanupClients();  
    
    ac.loop();

    if (restart) {
        delay(5000);
        ESP.restart();
    } 
}

// WebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        client->text("Connected to AC Controller");
    }
}

// Send log message to both Serial and WebSocket
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
        json += "\"state\":\"" + String(powerState ? "ON" : "OFF") + "\","; // NOVO
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

    // WiFi setup POST handler
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        String newSSID = "";
        String newPass = "";
        
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
            }
        }
        
        request->send(LittleFS, "/wifi_setup_success.html", "text/html");
        delay(2000);
        ESP.restart();
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
