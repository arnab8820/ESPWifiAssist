#include "ESPWifiAssist.h"
#include "index_html.h"

ESPWifiAssist::ESPWifiAssist(String _apSsid, String _apPassword)
{
    // Constructor implementation
    WiFi.mode(WIFI_OFF);
    initFs();
    apSsid = _apSsid;
    apPassword = _apPassword;
    webServer = new ESP8266WebServer(80); // Assign the server to the member variable
    initWebServer();
    registerEventHandlers();
}

ESPWifiAssist::ESPWifiAssist(String _apSsid, String _apPassword, ESP8266WebServer& _server) : webServer(&_server)
{
    // Constructor implementation
    WiFi.mode(WIFI_OFF);
    initFs();
    apSsid = _apSsid;
    apPassword = _apPassword;
    initWebServer();
    registerEventHandlers();
}

void ESPWifiAssist::beginWifi(){
    if(isFileExists("/config/wifi-config.json")){
        // read the JSON file
        DynamicJsonDocument doc = readJsonFromFile("/config/wifi-config.json");

        // set saved ssid
        String value = doc["ssid"].as<String>();
        strncpy(ssid, value.c_str(), sizeof(ssid) - 1);
        ssid[sizeof(ssid) - 1] = '\0';  // Ensure null termination

        // set saved password
        value = doc["password"].as<String>();
        strncpy(password, value.c_str(), sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        if(ssid == "" || password == ""){
            startAp();            
            return;
        }

        connectToWiFi();
    } else {
        startAp();        
    }
}

void ESPWifiAssist::onWifiConnected(WifiConnectedCb cb)
{
    connectedCb = cb;
}

void ESPWifiAssist::onWifiConnectFailed(WifiFailedCb cb) {
    failedCb = cb;
}

void ESPWifiAssist::onWifiGotIp(WifiGotIpCb cb)
{
    gotIpCb = cb;
}

void ESPWifiAssist::onWifiModeChanged(WifiModeChangeCb cb)
{
    modeChangeCb = cb;
}

void ESPWifiAssist::connectToWiFi()
{
    if(ssid == "" || ssid == NULL || password == "" || password == NULL){
        return;
    }

    // start in station mode
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
}

void ESPWifiAssist::startAp()
{
    // Set WiFi mode to Access Point
    WiFi.mode(WIFI_AP); 
    
    // Start the Access Point with SSID and password
    WiFi.softAP(apSsid, apPassword); 

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

void ESPWifiAssist::monitorWifiConnection(){
    webServer->handleClient();
    dnsServer.processNextRequest();
}

void ESPWifiAssist::setHostName(String hostname)
{
    WiFi.setHostname(hostname.c_str());
}

void ESPWifiAssist::initWebServer(){
    webServer->on("/wifi", HTTP_GET, [this](){        
        webServer->send_P(200, "text/html", (const char*)index_html, index_html_len);
    });

    webServer->on("/scan", HTTP_GET, [this](){
        int n = WiFi.scanNetworks();
        String scanData;
        const size_t arrSize = JSON_ARRAY_SIZE(1024);
        StaticJsonDocument<arrSize> arr;
        JsonArray aplist = arr.to<JsonArray>();
        if (n == 0) { 
            
        } else {
            for (int i = 0; i < n; ++i) {
                const size_t CAPACITY = JSON_OBJECT_SIZE(5);
                StaticJsonDocument<CAPACITY> doc;
                JsonObject ap = doc.to<JsonObject>();
                ap["ssid"] = WiFi.SSID(i);
                if (WiFi.isConnected() && WiFi.SSID(i) == WiFi.SSID()){
                    ap["status"] = "Connected";
                } else {
                    ap["status"] = WiFi.encryptionType(i)==5?"Secured(WEP)":WiFi.encryptionType(i)==2?"Secured(WPA)":WiFi.encryptionType(i)==4?"Secured(WPA2)":WiFi.encryptionType(i)==7?"Open":"Secured(WPA2/WPA Auto)";
                }
                
                ap["signal"] = WiFi.RSSI(i);
                aplist.add(ap);
            }
            serializeJson(aplist, scanData);
            webServer->send(200, "application/json", scanData);
        }
    });

    webServer->on("/connect", HTTP_POST, [this](){
        String postData = webServer->arg("plain");
        DynamicJsonDocument doc(1000);
        DeserializationError error = deserializeJson(doc, postData);
        if (error) {
            Serial.println(F("ERROR: deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        } else {
            const char* ssid = doc["ssid"];
            const char* password = doc["password"];
            saveWifiCredentials(ssid, password);
            connectToWiFi();
        }
        webServer->send(200, "text/plain", "");
    });

    webServer->on("/generate_204", HTTP_GET, [this](){
        webServer->sendHeader("Location", "/wifi", true);
        webServer->send(302, "text/plain", "");
    });
    
    webServer->on("/redirect", HTTP_GET, [this](){
        webServer->sendHeader("Location", "/wifi", true);
        webServer->send(302, "text/plain", "");
    });
    
    webServer->on("/hotspot-detect.html", HTTP_GET, [this](){
        webServer->sendHeader("Location", "/wifi", true);
        webServer->send(302, "text/plain", "");
    });

    webServer->onNotFound([this](){
        webServer->sendHeader("Location", "/wifi", true);
        webServer->send(302, "text/plain", "");
    });
    
    webServer->begin();
}

void ESPWifiAssist::saveWifiCredentials(const char *inputSsid, const char *inputPassword)
{
    if (inputSsid == NULL)
    {
        return;
    }
    strncpy(ssid, inputSsid, sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
    strncpy(password, inputPassword, sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';
    DynamicJsonDocument doc(1024);
    doc["ssid"] = ssid;
    doc["password"] = password;
    writeJsonToFile("/config/wifi-config.json", doc);
}

void ESPWifiAssist::registerEventHandlers()
{
    // set wifi connect event handler
    onConnectedHandler = WiFi.onStationModeConnected([this](const WiFiEventStationModeConnected& event) {
        Serial.println("Connected to AP");
        WiFi.softAPdisconnect(true);
        if (connectedCb)
        {
            connectedCb(ssid);
        }
        
    });

    // set wifi disconnected event handler 
    onDisconnectedHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected& event) {
        Serial.println("Disconnected from AP");
        deleteFile("/config/wifi-config.json");
        startAp();
        if (failedCb)
        {
            failedCb(event.reason);
        }
    });

    // set wifi got ip event handler
    onGotIpHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
        Serial.println("Got IP address: " + WiFi.localIP().toString());
        if(gotIpCb)
        {
            gotIpCb(WiFi.localIP());
        }
    });

    // set wifi mode change event handler
    onModeChangeHandler = WiFi.onWiFiModeChange([this](const WiFiEventModeChange& event) {
        Serial.println("WiFi mode changed");
        if(modeChangeCb)
        {
            modeChangeCb(event.newMode);
        }
    });
}
