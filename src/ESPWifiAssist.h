#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <functional>
#include "file-manager.h"

class ESPWifiAssist
{
private:
    char ssid[64] = "";
    char password[64] = "";

    String apSsid = "ESP-wifi-assist";
    String apPassword = "";

    WiFiEventHandler onConnectedHandler;
    WiFiEventHandler onDisconnectedHandler;
    WiFiEventHandler onGotIpHandler;
    WiFiEventHandler onModeChangeHandler;

    using WifiConnectedCb = std::function<void(String ssid)>;
    using WifiFailedCb    = std::function<void(uint8_t reason)>;
    using WifiGotIpCb     = std::function<void(const IPAddress& ip)>;
    using WifiModeChangeCb = std::function<void(WiFiMode mode)>;

    // bool wifiConnecting = false;
    // long connectionStartTime = 0;

    const byte DNS_PORT = 53; // DNS port number
    DNSServer dnsServer;

    ESP8266WebServer* webServer;
    
    void connectToWiFi();
    void startAp();
    void initWebServer();
    void saveWifiCredentials(const char *inputSsid, const char *inputPassword);
    void registerEventHandlers();

    WifiConnectedCb connectedCb;
    WifiFailedCb failedCb;
    WifiGotIpCb gotIpCb;
    WifiModeChangeCb modeChangeCb;

public:
    ESPWifiAssist(String _apSsid, String _apPassword);
    ESPWifiAssist(String _apSsid, String _apPassword, ESP8266WebServer& _server);
    void beginWifi();
    void onWifiConnected(WifiConnectedCb cb);
    void onWifiConnectFailed(WifiFailedCb cb);
    void onWifiGotIp(WifiGotIpCb cb);
    void onWifiModeChanged(WifiModeChangeCb cb);
    void monitorWifiConnection();
    void setHostName(String hostname);
};
