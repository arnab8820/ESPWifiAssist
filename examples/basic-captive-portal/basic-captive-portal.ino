#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPWifiAssist.h>

/*
   ================================
   Configuration
   ================================
*/

// Access Point credentials (shown when WiFi not configured)
#define AP_SSID     "ESP-Setup"
#define AP_PASSWORD "12345678"

// Optional hostname once connected
#define HOST_NAME   "esp-wifi-assist"

/*
   ================================
   Global objects
   ================================
*/

ESPWifiAssist wifiAssist(AP_SSID, AP_PASSWORD);

/*
   ================================
   Callback handlers
   ================================
*/

void onWifiConnected(const String& ssid)
{
    Serial.println();
    Serial.println("✅ WiFi connected to AP");
    Serial.print("   SSID: ");
    Serial.println(ssid);
}

void onWifiGotIp(IPAddress ip)
{
    Serial.println("🌐 Got IP address");
    Serial.print("   IP: ");
    Serial.println(ip);

    Serial.println("   Open this in browser:");
    Serial.print("   http://");
    Serial.println(ip);
}

void onWifiFailed(uint8_t reason)
{
    Serial.println("❌ WiFi connection failed");
    Serial.print("   Reason code: ");
    Serial.println(reason);

    /*
       You can decode reasons if you want:
       https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/include/wl_definitions.h
    */
}

void onWifiModeChanged(WiFiMode_t mode)
{
    Serial.print("🔄 WiFi mode changed: ");

    switch (mode)
    {
        case WIFI_OFF: Serial.println("WIFI_OFF"); break;
        case WIFI_STA: Serial.println("WIFI_STA"); break;
        case WIFI_AP:  Serial.println("WIFI_AP");  break;
        case WIFI_AP_STA: Serial.println("WIFI_AP_STA"); break;
        default: Serial.println("UNKNOWN"); break;
    }
}

/*
   ================================
   Arduino lifecycle
   ================================
*/

void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println("================================");
    Serial.println(" ESPWifiAssist - Basic Example ");
    Serial.println("================================");

    // Register callbacks
    wifiAssist.onWifiConnected(onWifiConnected);
    wifiAssist.onWifiGotIp(onWifiGotIp);
    wifiAssist.onWifiConnectFailed(onWifiFailed);
    wifiAssist.onWifiModeChanged(onWifiModeChanged);

    // Optional hostname
    wifiAssist.setHostName(HOST_NAME);

    // Start WiFi logic
    wifiAssist.beginWifi();

    Serial.println("WiFi Assist started");
    Serial.println("If not connected, connect to AP:");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
}

void loop()
{
    // VERY IMPORTANT
    // This keeps the captive portal, DNS, and web server alive
    wifiAssist.monitorWifiConnection();

    // Your application logic can run here
}
