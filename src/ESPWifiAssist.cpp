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

        if(ssid[0] == '\0' || password[0] == '\0'){
            startAp(false);
            return;
        }

        startAp(true);
        connectToWiFi();
    } else {
        startAp(false);
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

void ESPWifiAssist::connectToWiFi(const char* inputSsid, const char* inputPassword)
{
    const char* connectSsid = inputSsid ? inputSsid : ssid;
    const char* connectPassword = inputPassword ? inputPassword : password;

    if (connectSsid == NULL || connectSsid[0] == '\0' || connectPassword == NULL || connectPassword[0] == '\0') {
        return;
    }

    if (apStarted) {
        WiFi.mode(WIFI_AP_STA);
    } else {
        WiFi.mode(WIFI_STA);
    }

    WiFi.begin(connectSsid, connectPassword);
    connectionStartTime = millis();
    isRetryingConnection = true;
}

void ESPWifiAssist::startAp(bool allowSta)
{
    if (apStarted) {
        return;
    }

    WiFi.mode(allowSta ? WIFI_AP_STA : WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    apStarted = true;
}

void ESPWifiAssist::monitorWifiConnection(){
    webServer->handleClient();
    dnsServer.processNextRequest();

    if (isRetryingConnection && WiFi.status() != WL_CONNECTED) {
        if (millis() - connectionStartTime >= CONNECTION_RETRY_INTERVAL) {
            if (pendingConfig) {
                connectToWiFi(pendingSsid, pendingPassword);
            } else {
                connectToWiFi();
            }
        }
    }
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
            const char* requestSsid = doc["ssid"];
            const char* requestPassword = doc["password"];
            if (requestSsid == NULL || requestSsid[0] == '\0') {
                return;
            }

            strncpy(pendingSsid, requestSsid, sizeof(pendingSsid) - 1);
            pendingSsid[sizeof(pendingSsid) - 1] = '\0';
            strncpy(pendingPassword, requestPassword ? requestPassword : "", sizeof(pendingPassword) - 1);
            pendingPassword[sizeof(pendingPassword) - 1] = '\0';
            pendingConfig = true;

            bool savedConfigExists = ssid[0] != '\0' && password[0] != '\0';
            bool ssidChanged = savedConfigExists ? String(pendingSsid) != String(ssid) : true;
            bool passwordChanged = savedConfigExists ? String(pendingPassword) != String(password) : true;
            pendingSaveOnConnect = !savedConfigExists || ssidChanged || passwordChanged;

            connectToWiFi(pendingSsid, pendingPassword);
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
        hasConnectedSuccessfully = true;
        isRetryingConnection = false;

        if (pendingConfig && pendingSaveOnConnect) {
            saveWifiCredentials(pendingSsid, pendingPassword);
        } else if (pendingConfig && ssid[0] == '\0') {
            saveWifiCredentials(pendingSsid, pendingPassword);
        }

        pendingConfig = false;
        pendingSaveOnConnect = false;

        if (apStarted) {
            WiFi.softAPdisconnect(true);
            dnsServer.stop();
            apStarted = false;
        }

        if (connectedCb)
        {
            connectedCb(ssid);
        }
    });

    // set wifi disconnected event handler 
    onDisconnectedHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected& event) {
        Serial.println("Disconnected from AP");

        bool hasSavedConfig = ssid[0] != '\0' && password[0] != '\0';
        if (!hasConnectedSuccessfully) {
            if (pendingConfig && hasSavedConfig) {
                pendingConfig = false;
                pendingSaveOnConnect = false;
            }

            if (!apStarted) {
                startAp(true);
            }
        } else {
            WiFi.mode(WIFI_STA);
        }

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
