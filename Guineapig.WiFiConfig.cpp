#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif
#include "Guineapig.WiFiConfig.h"
#include <EEPROM.h>
#define EEPROM_SIZE 64

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
        request->send(200, "text/plain", "Rebooting");
    });

    this->printLog("AP Mode <");
    this->printLog(apSsid);
    this->printlnLog(">");
    String ipStr = this->Ip2String(apIp);
    this->printlnLog("IP=" + ipStr);
    this->printlnLog();
    server.begin();
    this->printlnLog("WiFi config web on");
    this->printlnLog("http://" + ipStr);

    while (true)
    {
        if (ssid != "" && passwd != "")
        {
            EEPROM.begin(EEPROM_SIZE);
            String ssidPwd = String("\t") + ssid + "\t" + passwd;
            for (int i = 0; i < ssidPwd.length() && i < EEPROM_SIZE; i++)
            {
                EEPROM.write(i, ssidPwd[i]);
            }
            EEPROM.write(ssidPwd.length(), 0);
            EEPROM.commit();
            this->printlnLog("ESP is rebooting...");
            ESP.restart();
        }
        digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ^ 1);
        delay(500);
    }
}

bool GuineapigWiFiConfig::tryConnect()
{
    String ssid = WiFi.SSID();
    String pwd = WiFi.psk();
    //check EEPROM for new ssid and password
    EEPROM.begin(EEPROM_SIZE);
    if (EEPROM.read(0) == '\t')
    {
        this->printlnLog("Reading WiFi config");
        String ssidPwd;
        for (int i = 1; i < EEPROM_SIZE; i++)
        {
            char c = EEPROM.read(i);
            if (c == 0) break;
            ssidPwd += c;
        }
        int pos = ssidPwd.indexOf('\t');
        if (pos <= 0)
        {
            this->printlnLog("invalid WiFi config");
        }
        else
        {
            ssid = ssidPwd.substring(0, pos);
            this->printlnLog("SSID=" + ssid);
            pwd = ssidPwd.substring(pos + 1, ssidPwd.length());
        }
    }
    this->printlnLog();
    int timeoutCount = 100; //connection timeout = 10s
    bool connected = false;
    //flash the LED
    pinMode(LED_BUILTIN, OUTPUT);
    bool onOff = true;
    if (ssid != "" && pwd != "")
    {
        this->printLog(String("Conneting ") + ssid + " ");
        WiFi.begin(ssid.c_str(), pwd.c_str());
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
        this->printlnLog(this->Ip2String(WiFi.localIP()));
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

void GuineapigWiFiConfig::resetWiFiConfig()
{
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; i++)
        EEPROM.write(i, 0);
    EEPROM.commit();
    WiFi.disconnect();
    ESP.restart();
}

GuineapigWiFiConfig WiFiConfig;