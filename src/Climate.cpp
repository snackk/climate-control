#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Filesys.h>
#include <Midea.h>
#include <Wifi.h>

const char* VERSION = "1.0.0";

// AsyncWebServer on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

// ESP restart
boolean restart = false;

// MideaAC
MideaAC mideaAc;

void initAsyncWebServer();
String processor(const String& var);
void onAcStatus(ac_status_t* status);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void webLog(String message); 

void setup() {
    // Initialize Serial port - 9600 for Midea
    Serial.begin(9600);
    Serial1.begin(115200);
    webLog("\n\nMidea AC Controller Starting...");

    // Initialize File System
    Filesys.initFS();
    
    // Initialize WiFi
    Wifi.initWiFi(&server);

    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Initialize OTA
    ElegantOTA.begin(&server);
    
    // Initialize Web Server 
    initAsyncWebServer();
    
    mideaAc.begin(&Serial);
    webLog("MideaAC.begin() called");
    
    mideaAc.onStatus(onAcStatus);
    webLog("Status callback registered");
    
    webLog("AC Controller Ready!");
}

void loop() {
    Wifi.handleWiFiReconnection();
    ElegantOTA.loop();
    ws.cleanupClients();  // <-- Clean up old WebSocket connections
    
    // Periodic AC status check
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        webLog("--- Checking AC communication ---");
        
        // Check if data is available
        if (Serial.available()) {
            webLog("Data available from AC!");
        } else {
            webLog("No data from AC");
        }
    }
    
    mideaAc.loop();

    if (restart) {
        delay(5000);
        ESP.restart();
    } 
}

// WebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial1.printf("WebSocket client #%u connected\n", client->id());
        client->text("Connected to AC Controller");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial1.printf("WebSocket client #%u disconnected\n", client->id());
    }
}

// Send log message to both Serial and WebSocket
void webLog(String message) {
    Serial1.println(message);
    ws.textAll(message);  // Send to all connected WebSocket clients
}

String processor(const String& var) {
    if(var == "AC_STATE") {
        return "HARDCODED";
    }
    if(var == "VERSION") {
        return VERSION;
    }
    return String();
}

void initAsyncWebServer() {
    // Dynamic root handler
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.getMode() == WIFI_AP || WiFi.status() != WL_CONNECTED) {
            request->send(LittleFS, "/wifi_setup.html", "text/html");
        } else {
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        }
    });

    // Console page
    server.on("/console", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral()rawliteral";
        request->send(LittleFS, "/console.html", "text/html");
    });

    // Control routes
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        webLog("Web command: Power ON");
        mideaAc.setPower(true);
        request->send(200, "text/plain", "OK");
    });

    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        webLog("Web command: Power OFF");
        mideaAc.setPower(false);
        request->send(200, "text/plain", "OK");
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        ac_status_t* status = mideaAc.getStatus();
        
        String json = "{";
        json += "\"power\":" + String(status->power ? "true" : "false") + ",";
        json += "\"mode\":" + String(status->mode) + ",";
        json += "\"temp\":" + String(status->targetTemp) + ",";
        json += "\"indoor_temp\":" + String(status->indoorTemp) + ",";
        json += "\"outdoor_temp\":" + String(status->outdoorTemp) + ",";
        json += "\"fan\":" + String(status->fanSpeed) + ",";
        json += "\"swing\":" + String(status->swingMode) + ",";
        json += "\"turbo\":" + String(status->turboMode ? "true" : "false") + ",";
        json += "\"eco\":" + String(status->ecoMode ? "true" : "false");
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

void onAcStatus(ac_status_t* status) {
    webLog("=== AC Status Callback Triggered ===");
    webLog("Power: " + String(status->power ? "ON" : "OFF"));
    webLog("Mode: " + String(status->mode));
    webLog("Target Temp: " + String(status->targetTemp) + "°C");
    webLog("Indoor Temp: " + String(status->indoorTemp) + "°C");
    webLog("Fan Speed: " + String(status->fanSpeed));
}
