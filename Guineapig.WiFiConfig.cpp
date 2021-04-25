#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include "Guineapig.WiFiConfig.h"

void GuineapigWiFiConfig::printLog(String msg)
{
    Serial.print(msg);
    if (this->logCallback != 0)
        this->logCallback(msg);
}

void GuineapigWiFiConfig::initConfigWeb()
{
#ifdef ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t chipID = macAddressTrunc >> 40;
#else
    auto chipID = ESP.getChipId();
#endif
    //Set WiFi AP+STA mode
    auto apSsid = String("ESP-") + String(chipID, HEX);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid.c_str());
    IPAddress apIp = WiFi.softAPIP();
    AsyncWebServer server(80);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", guineapig_wifi_config_html);
    });
    String ssid(""), passwd(""), phase("WAIT"), message("請設定 SSID 與密碼");
    bool reboot = false;
    server.on("/", HTTP_POST, [&ssid, &passwd, &phase, &message, &reboot](AsyncWebServerRequest *request) {
        if ((*request->getParam("action", true)).value() == "重新啟動") {
            reboot = true;
        }
        else {
            ssid = (*request->getParam("ssid", true)).value();
            passwd = (*request->getParam("passwd", true)).value();
            message = "正在測試 [" + ssid + "] 無線網路...";
            phase = "CONNECTING";
        }
        request->send(200, "text/plain", "OK");
    });
    server.on("/status", HTTP_GET, [&phase, &message](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{ \"p\":\"" + phase + "\", \"m\":\"" + message + "\" }");
    });

    this->printlnLog("AP Mode <" + apSsid + ">");
    String ipStr = apIp.toString();
    this->printlnLog("IP=" + ipStr);
    this->printlnLog();
    server.begin();
    this->printlnLog("WiFi config web on");
    this->printlnLog("http://" + ipStr);

    String wifiIp;
    while (true)
    {
        if (phase == "CONNECTING")
        {
            WiFi.begin(ssid.c_str(), passwd.c_str());
            this->printlnLog("connecting [" + ssid + "] ");
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
                phase = "CONNECTED";
                message = "[" + ssid + "]連線成功<br />IP " + wifiIp + "<br />重新啟動後生效";
            }
            else
            {
                this->printlnLog(ssid + " not connected");
                phase = "FAILED";
                message = "[" + ssid + "]連線失敗<br />請重新輸入";
            }
        }
        if (reboot)
        {
            phase = "REBOOT";
            message = "ESP 正在重新啟動...<br />稍後請切換網路改用<br /><a href=http://" + wifiIp + ">http://" + wifiIp + "</a> 操作";
            delay(5000);
            ESP.restart();
        }
        digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ^ 1);
        delay(200);
    }
}
const char *ESP32_WIFI_CONF = "ESP32-WIFI-CONF";

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
    if (ssid == ESP32_WIFI_CONF || (ssid != "" && pwd != ""))
    {
        WiFi.mode(WIFI_STA);
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
        this->initConfigWeb();
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
    delay(2000);
    this->printlnLog("Guineapig WiFiConfig");
    pinMode(LED_BUILTIN, OUTPUT);
    return tryConnect();
}

void GuineapigWiFiConfig::clearWiFiConfig()
{
#ifdef ESP32
    WiFi.disconnect(true, true);
#else
    WiFi.disconnect(true);
#endif
    delay(1000);
    ESP.restart();
}

GuineapigWiFiConfig WiFiConfig;