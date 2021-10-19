#ifndef WiFiSerConf_h
#define WiFiSerConf_h

#include <Arduino.h>
#include <SerialCommand.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

class GuineapigWiFiSerConf {
  public:
    static void loop();
    static void init();
    static void connect();
    static void reset();
  private:
    GuineapigWiFiSerConf() = default;
    static String ssid;
    static String passwd;
    static SerialCommand sCmd;
    static int phase;
    static int flashBtnCount;
    static void flexInput(const char *command);
    static void config();
    static void exitCmdMode();
};

const char GuineapigWiFiSerConf_Info[] PROGMEM = R"===(
**** WiFi Config Mode ****
 > 'SET SSID', 'SET PASS' to set AP identity
 > 'CONNECT' to connect AP
 > 'RESET' to clear WiFi settings
 > 'EXIT' to quit config mode

)===";

#endif