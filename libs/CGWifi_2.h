#include "CGWifi.h"

CGWifi::CGWifi(const char* ssid, const char* password)
    : _ssid(ssid), _password(password), _lastReconnectAttempt(0), _previousMillis(0), _wasDisconnected(false), _ledState(false) {}

// Wi-Fi Connection
void CGWifi::connect() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(_ssid, _password);

    Serial.println("🔄 Connecting to Wi-Fi...");
    printLCDMessage("Connecting... Wait!");

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) { 
        delay(1000);
        Serial.print(".");
        retryCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        printLCDMessage("Wi-Fi Connected!");
        Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());
        digitalWrite(LED_BUILTIN, LOW);
        enableCoinSlot();
    } else {
        printLCDMessage("Wi-Fi Failed!");
        Serial.println("\n❌ Wi-Fi Failed! Last status: " + String(WiFi.status()));
        disableCoinSlot();
    }
}

// Monitor Wi-Fi & Reconnect
void CGWifi::checkReconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        unsigned long currentMillis = millis();

        // Blink LED every 500ms when connected
        if (currentMillis - _previousMillis >= 500) {
            _previousMillis = currentMillis;
            _ledState = !_ledState;
            digitalWrite(LED_BUILTIN, _ledState ? LOW : HIGH);
        }

        if (_wasDisconnected) {
            enableCoinSlot();
            Serial.println("✅ Reconnected! IP: " + WiFi.localIP().toString());
            printLCDMessage("Insert Coin");
            _wasDisconnected = false;
        }
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
        disableCoinSlot();

        unsigned long currentMillis = millis();

        if (currentMillis - _lastReconnectAttempt >= 10000) {
            Serial.println("🚨 Wi-Fi lost! Trying to reconnect...");
            printLCDMessage("Reconnecting...");

            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(100);
            WiFi.begin(_ssid, _password);

            _lastReconnectAttempt = currentMillis;
            _wasDisconnected = true;
        }
    }
}

// Stub functions for coin slot control and LCD
void CGWifi::enableCoinSlot() {
    Serial.println("🟢 Coin slot enabled.");
}

void CGWifi::disableCoinSlot() {
    Serial.println("🔴 Coin slot disabled.");
}

void CGWifi::printLCDMessage(const String& message) {
    Serial.println("📟 LCD: " + message);
}
