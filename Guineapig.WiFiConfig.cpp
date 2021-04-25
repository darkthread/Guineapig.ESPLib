#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif
#include "Guineapig.WiFiConfig.h"

void GuineapigWiFiConfig::printLog(String msg)
{
    Serial.print(msg);
    if (this->logCallback != 0)
        this->logCallback(msg);
}

void GuineapigWiFiConfig::initSetupWeb()
{
#ifdef ESP8266
    auto chipID = ESP.getChipId();
#endif

#ifdef ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t chipID = macAddressTrunc >> 40;
#endif

    //Set WiFi AP mode
    auto apSsid = String("ESP-") + String(chipID, HEX);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid.c_str());
    IPAddress apIp = WiFi.softAPIP();
    AsyncWebServer server(80);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", guineapig_wifi_config_html);
    });
    String ssid(""), passwd("");
    server.on("/", HTTP_POST, [&ssid, &passwd](AsyncWebServerRequest *request) {
        ssid = (*request->getParam("ssid", true)).value();
        passwd = (*request->getParam("passwd", true)).value();
        request->redirect("/pending");
    });
    String wifiIp("");
    String viewPortHeader = F("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>");
    server.on("/pending", HTTP_GET, [&wifiIp, &viewPortHeader](AsyncWebServerRequest *request) {
        if (wifiIp == "timeout")
        {
            wifiIp = "";
            request->redirect("/?failed");
            return;
        }
        String html = viewPortHeader;
        if (wifiIp == "")
            html += "Waiting... <script>setTimeout(function() { location.reload(); }, 2000);</script></body></html>";
        else
        {
            html += F("Please connect http://");
            html += wifiIp;
            html += " after reboot. <br /><a href=/reboot>Reboot</a></body></html>";
        }
        request->send(200, "text/html", html);
    });
    bool reboot = false;
    server.on("/reboot", HTTP_GET, [&reboot](AsyncWebServerRequest *request) {
        reboot = true;
        request->send(200, "text/html", F("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>Rebooting...</body></html>"));
    });

    this->printLog("AP Mode <");
    this->printLog(apSsid);
    this->printlnLog(">");
    String ipStr = apIp.toString();
    this->printlnLog("IP=" + ipStr);
    this->printlnLog();
    server.begin();
    this->printlnLog("WiFi config web on");
    this->printlnLog("http://" + ipStr);

    while (true)
    {
        if (ssid != "" && passwd != "")
        {
            WiFi.begin(ssid.c_str(), passwd.c_str());
            this->printlnLog("connecting " + ssid);
            int timeout = 30;
            while (WiFi.status() != WL_CONNECTED && timeout-- > 0)
            {
                this->printLog(".");
                delay(1000);
            }
            this->printlnLog();
            if (timeout > 0)
            {
                this->printlnLog("connected.");
                wifiIp = WiFi.localIP().toString();
                this->printlnLog("IP=" + wifiIp);
                ssid = "";
            }
            else
            {
                this->printlnLog(ssid + " not connected");
                wifiIp = "timeout";
                ssid = "";
            }
        }
        if (reboot)
        {
            ESP.restart();
        }
        digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ^ 1);
        delay(200);
    }
}
const char* ESP32_WIFI_CONF = "ESP32-WIFI-CONF";

bool GuineapigWiFiConfig::tryConnect()
{
    String ssid = WiFi.SSID();
    String pwd = WiFi.psk();
#ifdef ESP32
    ssid = ESP32_WIFI_CONF;
#endif    

    this->printlnLog();
    int timeoutCount = 100; //connection timeout = 10s
    bool connected = false;
    //flash the LED
    pinMode(LED_BUILTIN, OUTPUT);
    bool onOff = true;
    if (ssid == ESP32_WIFI_CONF || ssid != "" && pwd != "")
    {
#ifdef ESP32 
        this->printlnLog(String("Connecting stored SSID "));
        WiFi.begin();
#else        
        this->printLog(String("Conneting ") + ssid + " ");
        WiFi.begin(ssid.c_str(), pwd.c_str());
#endif
        while (WiFi.status() != WL_CONNECTED && timeoutCount > 0)
        {
            this->printLog(".");
            onOff = !onOff;
            digitalWrite(LED_BUILTIN, onOff ? LOW : HIGH);
            delay(100);
            timeoutCount--;
        }
        this->printlnLog();
        connected = timeoutCount > 0;
    }
    if (!connected)
    {
        this->initSetupWeb();
        return false;
    }
    else
    {
        this->printlnLog();
        this->printlnLog("AP connected.");
        this->printlnLog();
        this->printlnLog("SSID=>" + ssid);
        this->printLog("IP=>");
        this->printlnLog(WiFi.localIP().toString());
        this->printlnLog();
        return true;
    }
}

bool GuineapigWiFiConfig::connectWiFi()
{
    Serial.begin(115200);
    delay(2000);
    this->printlnLog("Guineapig WiFiConfig");
    pinMode(LED_BUILTIN, OUTPUT);
    return tryConnect();
}

void GuineapigWiFiConfig::clearWiFiConfig() {
#ifdef ESP32    
    WiFi.disconnect(true, true);
#else
    WiFi.disconnect(true);
#endif
    delay(1000);
    ESP.restart();
}


GuineapigWiFiConfig WiFiConfig;