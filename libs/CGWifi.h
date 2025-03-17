#ifndef CGWIFI_H
#define CGWIFI_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

class CGWifi {
public:
    CGWifi(const char* ssid, const char* password) 
        : _ssid(ssid), _password(password), _lastReconnectAttempt(0), _previousMillis(0), _wasDisconnected(false), _ledState(false) {}

    void connect() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(_ssid, _password);

        Serial.println("🔄 Connecting to Wi-Fi...");
        printLCDMessage("Connecting... Wait!");

        // Keep trying until connected
        while (WiFi.status() != WL_CONNECTED) { 
            delay(1000);
            Serial.print(".");
        }

        onConnected();
    }

    void checkReconnect() {
        if (WiFi.status() == WL_CONNECTED) {
            blinkLED();
            if (_wasDisconnected) {
                onConnected();
                _wasDisconnected = false;
            }
        } else {
            onDisconnected();
            attemptReconnect();
        }
    }

private:
    const char* _ssid;
    const char* _password;
    unsigned long _lastReconnectAttempt;
    unsigned long _previousMillis;
    bool _wasDisconnected;
    bool _ledState;

    void onConnected() {
        printLCDMessage("Wi-Fi Connected!");
        Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());
        digitalWrite(LED_BUILTIN, LOW);
        enableCoinSlot();
    }

    void onDisconnected() {
        digitalWrite(LED_BUILTIN, HIGH);
        disableCoinSlot();
    }

    void attemptReconnect() {
        unsigned long currentMillis = millis();
        if (currentMillis - _lastReconnectAttempt >= 10000) { // Retry every 10 seconds
            Serial.println("🚨 Wi-Fi lost! Trying to reconnect...");
            printLCDMessage("Reconnecting...");

            WiFi.disconnect();
            delay(100);
            WiFi.begin(_ssid, _password);

            _lastReconnectAttempt = currentMillis;
            _wasDisconnected = true;
        }
    }

    void blinkLED() {
        unsigned long currentMillis = millis();
        if (currentMillis - _previousMillis >= 500) {
            _previousMillis = currentMillis;
            _ledState = !_ledState;
            digitalWrite(LED_BUILTIN, _ledState ? LOW : HIGH);
        }
    }

    void enableCoinSlot() {
        Serial.println("🟢 Coin slot enabled.");
    }

    void disableCoinSlot() {
        Serial.println("🔴 Coin slot disabled.");
    }

    void printLCDMessage(const String& message) {
        Serial.println("📟 LCD: " + message);
    }
};

#endif
