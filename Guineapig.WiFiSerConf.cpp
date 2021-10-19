#include "Guineapig.WiFiSerConf.h"

String GuineapigWiFiSerConf::ssid;
String GuineapigWiFiSerConf::passwd;
SerialCommand GuineapigWiFiSerConf::sCmd;
int GuineapigWiFiSerConf::phase;         
int GuineapigWiFiSerConf::flashBtnCount;

void GuineapigWiFiSerConf::init()
{
    ssid = WiFi.SSID();
    passwd = WiFi.psk();  
    phase = -1;
    flashBtnCount = 0;    
    //準備WIFI設定指令
    sCmd.addCommand("SET", GuineapigWiFiSerConf::config);
    sCmd.addCommand("CONNECT", GuineapigWiFiSerConf::connect);
    sCmd.addCommand("RESET", GuineapigWiFiSerConf::reset);
    sCmd.addCommand("EXIT", GuineapigWiFiSerConf::exitCmdMode);
    sCmd.setDefaultHandler(GuineapigWiFiSerConf::flexInput);
    sCmd.echo = true;
    pinMode(0, INPUT);
}

void GuineapigWiFiSerConf::flexInput(const char *command)
{
    String maskedPwd;
    switch (phase)
    {
    case 1:
        ssid = String(command);
        Serial.println("SSID = " + ssid);
        break;
    case 2:
        passwd = String(command);
        sCmd.echo = true;
        maskedPwd = String(command);
        if (maskedPwd.length() > 2)
        {
            for (uint i = 1; i < maskedPwd.length() - 1; i++)
                maskedPwd.setCharAt(i, '*');
        }
        Serial.println("Password = " + maskedPwd);
        break;
    default:
        Serial.println("command unknown");
        break;
    }
    phase = 0;
}

void GuineapigWiFiSerConf::config()
{
    auto arg = String(sCmd.next());
    if (arg == "SSID")
    {
        phase = 1;
        Serial.println("Please input SSID:");
    }
    else if (arg == "PASS")
    {
        phase = 2;
        sCmd.echo = false;
        Serial.println("Please input password:");
    }
}

void GuineapigWiFiSerConf::connect()
{
    if (ssid.length() == 0 && passwd.length() == 0) {
        Serial.println("WiFi not configed!");
        return;
    }
    Serial.print("Connecting " + ssid + "...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passwd);
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout-- > 0)
    {
        Serial.print(".");
        delay(1000);
    }
    if (timeout > 0)
    {
        Serial.println("\nConnected as " + WiFi.localIP().toString());
    }
    else
    {
        Serial.println("\nFailed to connect WiFi!");
    }
}

void GuineapigWiFiSerConf::reset() {
    WiFi.disconnect();
    Serial.println("Clear WiFi settings and disconnect");
}

void GuineapigWiFiSerConf::exitCmdMode()
{
    Serial.println("Exit WiFi config mode!");
    phase = -1;
}

void GuineapigWiFiSerConf::loop()
{
    if (phase != -1)
        sCmd.readSerial();
    else if (digitalRead(0) == LOW)
    {
        flashBtnCount++;
        if (flashBtnCount > 10)
        {
            phase = 0;
            Serial.println("** WiFi Config Mode **");
            int len = strlen_P(GuineapigWiFiSerConf_Logo);
            for (int i = 0; i < len; i++) {
                char c = pgm_read_byte_near(GuineapigWiFiSerConf_Logo + i);
                Serial.print(c);
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Conneted to " + ssid + " now, IP = " + WiFi.localIP().toString());
            }
            else {
                Serial.println("Wireless network not connected");
            }
            Serial.println("'SET SSID', 'SET PASS' to set AP identity,");
            Serial.println("'CONNECT' to connect AP");
            Serial.println("'RESET' to clear WiFi settings");
            Serial.println("'EXIT' to quit config mode.");
        }
        delay(50);
    }
    else
    {
        flashBtnCount = 0;
    }
}

