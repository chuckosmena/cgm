#include "libs/CGConfig.h"
#include "libs/CGLCD.h"
#include "libs/CGPrinter.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>  // Use WiFiClientSecure for HTTPS

// Set up HTTP client
HTTPClient http;
WiFiClientSecure client;

// CGPrinter Printer(PIN_PRINTER_RX, PIN_PRINTER_TX, PRINTER_BAUD_RATE, PRINTER_HEAT_DOTS, PRINTER_HEAT_TIME, PRINTER_HEAT_INTERVAL);
CGPrinter Printer;
CGLCD Lcd;

// Interrupt Service Routine (ISR) for Coin Insertion
void IRAM_ATTR coinInserted() {
    unsigned long now = millis();

    // Ignore noise: If two pulses come too fast (<COIN_MIN_PULSE_INTERVAL_MS), discard it
    if (now - gLastPulseTime < COIN_MIN_PULSE_INTERVAL_MS) {
        return;
    }

    // Double-check the pin state before counting the coin
    if (digitalRead(PIN_COIN_PULSE) == HIGH) {
        return;  // Ignore false pulses
    }

    // If more than COIN_PULSE_TIMEOUT_MS ms passed, assume new coin
    if (now - gLastPulseTime > COIN_PULSE_TIMEOUT_MS) {
        gPulseCounter = 0;
    }

    gLastPulseTime = now;
    gPulseCounter++;

    if (gPulseCounter >= COIN_VALID_PULSES) {
        // Track coins inserted during API request
        if (gApiInProgress) {
            gNewCoinsDuringApi++;  // Buffer coins during API call
        } else {
            gTotalFivePesoCoins++;  // Register 1 valid coin
        }
        gPulseCounter = 0;  // Reset pulse counter for the next coin
        Serial.println("✅ 5 Peso Coin Inserted! Total: " + String(gTotalFivePesoCoins + gNewCoinsDuringApi));
        Lcd.coinInsertedText();

        // Start/reset the reset timer
        gResetTimerActive = true;
        gResetStartTime = millis();
    }
}

// Setup Function
void setup() {
    Serial.begin(115200);

    // Initialize MyLCD
    Lcd.begin();
    Lcd.reset();

    // Initialize the printer
    Printer.init();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);  // Start with LED OFF

    pinMode(PIN_COIN_RELAY, OUTPUT);
    digitalWrite(PIN_COIN_RELAY, LOW);  // Start with the coin slot OFF

    wifiConnection();

    // Setup Coin Interrupt
    pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_COIN_PULSE), coinInserted, FALLING);
}

void loop() {
    checkWiFiReconnect();  // Check Wi-Fi and reconnect if necessary

    // Only process reset if API is NOT in progress
    if (!gApiInProgress && gResetTimerActive && (millis() - gResetStartTime >= COIN_RESET_DELAY_MS)) {
        gResetTimerActive = false;  // Stop timer BEFORE making API request

        if (gTotalFivePesoCoins > 0 || gFailedCoins > 0) {
            int totalToRequest = gTotalFivePesoCoins + gFailedCoins;
            Serial.println("⏳ " + String(COIN_RESET_DELAY_MS / 1000) + " Seconds Passed! Final Total: " + String(totalToRequest));

            // Mark API request as in progress
            gApiInProgress = true;
            apiRequest(totalToRequest);
        }
    }

    delay(100);
}

// Function to enable the coin slot
void enableCoinSlot() {
    digitalWrite(PIN_COIN_RELAY, HIGH);  // Turns the relay ON (coin slot enabled)
}

// Function to disable the coin slot
void disableCoinSlot() {
    digitalWrite(PIN_COIN_RELAY, LOW);  // Turns the relay OFF (coin slot disabled)
}

// Wi-Fi Connection
void wifiConnection() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(kWiFiSsid, kWiFiPass);

    Serial.println("🔄 Connecting to Wi-Fi...");
    Lcd.printCenter("Connecting... Wait!", 1);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
        delay(1000);
        Serial.print(".");
        retryCount++;
        // Serial.print(" Status: ");
        // Serial.println(WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED) {
        Lcd.insertCoinText();
        Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());
        digitalWrite(LED_BUILTIN, LOW);
        enableCoinSlot();
    } else {
        Lcd.printCenter("Wi-Fi Failed!", 1);
        Serial.println("\n❌ Wi-Fi Failed! Last status: " + String(WiFi.status()));
        disableCoinSlot();
    }
}

// Function to Monitor Wi-Fi & Force Reconnect
void checkWiFiReconnect() {
    static unsigned long lastReconnectAttempt = 0;
    static bool wasDisconnected = false;
    static unsigned long previousMillis = 0;
    static bool ledState = false;

    // Serial.print("WiFi Status: ");
    // Serial.println(WiFi.status());

    if (WiFi.status() == WL_CONNECTED) {
        unsigned long currentMillis = millis();

        // Blink LED every 500ms when connected
        if (currentMillis - previousMillis >= 500) {
            previousMillis = currentMillis;
            ledState = !ledState;                              // Toggle LED state
            digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);  // Active LOW logic
        }

        if (wasDisconnected) {
            enableCoinSlot();
            Serial.println("✅ Reconnected! IP: " + WiFi.localIP().toString());
            Lcd.insertCoinText();
            wasDisconnected = false;
        }
    } else {
        // Wi-Fi Disconnected: Keep LED OFF
        digitalWrite(LED_BUILTIN, HIGH);
        disableCoinSlot();

        unsigned long currentMillis = millis();

        // Try to reconnect every 10 seconds
        if (currentMillis - lastReconnectAttempt >= 10000) {
            Serial.println("🚨 Wi-Fi lost! Trying to reconnect...");
            Lcd.printCenter("Reconnecting...", 1);

            WiFi.mode(WIFI_STA);               // Ensure STA mode
            WiFi.disconnect();                 // Reset connection
            delay(100);                        // Short delay to stabilize
            WiFi.begin(kWiFiSsid, kWiFiPass);  // Reconnect

            lastReconnectAttempt = currentMillis;
            wasDisconnected = true;
        }
    }
}

// API Request Function
void apiRequest(int quantity) {
    if (WiFi.status() == WL_CONNECTED) {
        Lcd.printCenter("Buying Voucher", 1);
        Serial.println("Requesting to " + String(kServerApiUrl) + String(quantity));
        client.setInsecure();
        http.begin(client, kServerApiUrl + String(quantity));
        http.addHeader("GEEKX-API-KEY", kServerApiKey);
        http.addHeader("Accept", "application/json");
        http.setTimeout(8000);

        int httpCode = http.GET();
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);

        if (httpCode > 0) {
            Serial.println("Request successful, parsing JSON...");
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, http.getString());

            if (!error) {
                JsonObject data = doc.as<JsonObject>();
                if (data.containsKey("vouchers")) {
                    for (JsonVariant voucherCode : data["vouchers"].as<JsonArray>()) {
                        printVoucher(voucherCode.as<String>(), data["product"]["name"].as<String>(), data["paid_at"].as<String>());
                    }
                }
                gFailedCoins = 0;  // Clear failed coins since API worked
            } else {
                Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
                gFailedCoins += quantity;  // Add failed coins back
            }
        } else {
            Serial.println("❌ Request failed, HTTP code: " + String(httpCode));
            gFailedCoins += quantity;  // Add failed coins back
        }

        http.end();
    } else {
        Serial.println("❌ Wi-Fi not connected");
        gFailedCoins += quantity;  // Add failed coins back
    }

    // ✅ Merge new coins that were inserted during API request
    gTotalFivePesoCoins = gNewCoinsDuringApi;
    gNewCoinsDuringApi = 0;
    gApiInProgress = false;  // ✅ Allow new coins to be counted
}

// Function to Print a Voucher
void printVoucher(String code, String productName, String paidAt) {
    String howToConnect =
        "How to connect? Follow these:\n"
        "1. Connect to GeekX_5G\n"
        "2. Popup shows, enter the code\n"
        "3. Open browser and type:\n"
        "geekxfiber.technology/auth.htm\n"
        "4. SURF NOW\n\n"
        "Date Time:   " +
        String(paidAt) +
        "\n"
        "Subtotal:    Php 4.40\n"
        "Vat:         Php 0.60\n"
        "Grand Total: Php 5.00\n";

    Serial.println("🖨 Printing Voucher: " + code);
    Lcd.printCenter("Printing Voucher", 1);
    Lcd.printCenter(code.c_str(), 2);

    // Initialize printer and ensure settings are applied
    Printer.wake();  // Ensure the printer is awake
    Printer.feed(1); // Feed one line to clear any residual data

    // Printer.setFontStyle(size, alignment, isBold, type)
    // Printer.println(text, size, alignment, isBold, type)

    Printer.printlnr("5GXMART TECHNOLOGIES CORP.", 'S', 'C', 1, 0);
    Printer.printlnr("Vat Reg: 646-091-737-00000", 'S', 'C', 0, 2);
    Printer.println("________________________________");
    Printer.printlnr(productName.c_str(), 'M', 'C', 1, 2);
    Printer.printlnr("Voucher Code", 'M', 'C', 0, 3);
    Printer.printlnr(code.c_str(), 'L', 'C', 1, 0);
    Printer.println("________________________________");

    Printer.printlnr(howToConnect.c_str(), 'S', 'L', 0, 2);

    Printer.printlnr("Powered by CoolGeeks", 'M', 'C', 1, 0);
    Printer.println("================================\n\n");
    Printer.feed(2);
    Printer.sleep();

    // ✅ Estimate print time (3ms per character, adjust as needed)
    String fullText = code + productName + paidAt;
    int estimatedPrintTime = strlen(fullText.c_str()) * 3 + 3000;
    delay(estimatedPrintTime);

    Lcd.insertCoinText();
    Lcd.clearRow(2);
}