#include "Arduino.h"
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>

#define BUILTIN_LED 2
#define FLASH_BUTTON 0

Ticker ticker;
boolean isFlashButtonWasPressed = false;

void ICACHE_RAM_ATTR pushFlashButtonHandler() {
    isFlashButtonWasPressed = true;
}

void tick() {
    //toggle state
    int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
    ticker.attach(0.2, tick);
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(FLASH_BUTTON, INPUT_PULLUP);
    attachInterrupt ( digitalPinToInterrupt (FLASH_BUTTON), pushFlashButtonHandler, FALLING);

    //set led pin as output
    pinMode(BUILTIN_LED, OUTPUT);
    // start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setTimeout(10);
//    wifiManager.setConnectTimeout(10);
    wifiManager.setBreakAfterConfig(true);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    station_config conf{};
    wifi_station_get_config(&conf);
    while (!wifiManager.autoConnect("AutoConnectAP")) {
//        wifi_station_set_config_current(&conf);
        if (isFlashButtonWasPressed) {
            WiFi.disconnect();
            delay(100);
        }
        ESP.restart();
        delay(1000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    wifi_station_get_config(&conf);
    strcpy(reinterpret_cast<char *>(conf.password), wifiManager.getPassword().c_str());
    strcpy(reinterpret_cast<char *>(conf.ssid), wifiManager.getSSID().c_str());
    wifi_station_set_config(&conf);

    ticker.detach();
    //keep LED on
    digitalWrite(BUILTIN_LED, LOW);
}

void loop() {
    // put your main code here, to run repeatedly:
    if (isFlashButtonWasPressed) {
        WiFi.disconnect();
        delay(100);
        ESP.restart();
    }
    
    wl_status_t status = WiFi.status();
    if (status != WL_CONNECTED) {
        Serial.printf("wifi connection lost, status %d", status);
        ESP.restart();
    }
}
