#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>  // Use WiFiClientSecure for HTTPS
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include "config.h"

// Set up HTTP client
HTTPClient http;
WiFiClientSecure client;

// Initialize SoftwareSerial and Printer
SoftwareSerial mySerial(RX_PIN, TX_PIN);
Adafruit_Thermal printer(&mySerial);

// Interrupt Service Routine (ISR) for Coin Insertion
void IRAM_ATTR coinInserted() {
    unsigned long now = millis();

    if (now - lastPulseTime < MIN_PULSE_INTERVAL) return;  // Ignore rapid pulses
    if (digitalRead(COIN_PIN) == HIGH) return;  // Ignore false pulses

    if (now - lastPulseTime > PULSE_TIMEOUT) {
        pulseCounter = 0; // Reset pulse counter
    }

    lastPulseTime = now;
    pulseCounter++;

    if (pulseCounter >= VALID_PULSES) {
        if (apiInProgress) {
            newCoinsDuringAPI++; // Buffer coins during API call
        } else {
            totalFivePesoCoins++; // Register 1 valid coin
        }
        pulseCounter = 0; // Reset counter
        Serial.println("✅ 5 Peso Coin Inserted! Total: " + String(totalFivePesoCoins + newCoinsDuringAPI));

        resetTimerActive = true;
        resetStartTime = millis();
    }
}

// Wi-Fi Connection
void wifiConnection() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("Connecting to Wi-Fi...");

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
        delay(1000);
        Serial.print(".");
        retryCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected to Wi-Fi! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n❌ Wi-Fi Connection Failed! Retrying...");
        delay(5000);
        wifiConnection(); // Retry connection
    }
}

// API Request Function
void apiRequest(int quantity) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Requesting " + String(quantity) + " vouchers...");
        client.setInsecure();
        http.begin(client, SERVER_API_URL + String(quantity));
        http.addHeader("GEEKX-API-KEY", SERVER_API_KEY);
        http.addHeader("Accept", "application/json");
        http.setTimeout(8000);

        int httpCode = http.GET();
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);

        if (httpCode > 0) {
            Serial.println("✅ Request successful, parsing JSON...");
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, http.getString());

            if (!error) {
                JsonObject data = doc.as<JsonObject>();
                if (data.containsKey("vouchers")) {
                    for (JsonVariant voucherCode : data["vouchers"].as<JsonArray>()) {
                        printVoucher(voucherCode.as<String>(), data["product"]["name"].as<String>(), data["paid_at"].as<String>());
                    }
                }
                failedCoins = 0; // Clear failed coins
            } else {
                Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
                failedCoins += quantity;
            }
        } else {
            Serial.println("❌ Request failed, HTTP code: " + String(httpCode));
            failedCoins += quantity;
        }

        http.end();
    } else {
        Serial.println("❌ Wi-Fi not connected");
        failedCoins += quantity;
    }

    // Merge coins inserted during API request
    totalFivePesoCoins = newCoinsDuringAPI;
    newCoinsDuringAPI = 0;
    apiInProgress = false;
}

// Setup Function
void setup() {
    Serial.begin(115200);
    mySerial.begin(9600);

    // Initialize the printer
    printer.begin();
    printer.setHeatConfig(200, 255, 250);

    wifiConnection();

    pinMode(COIN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(COIN_PIN), coinInserted, FALLING);
}

void loop() {
    if (!apiInProgress && resetTimerActive && (millis() - resetStartTime >= RESET_DELAY)) {
        resetTimerActive = false; // Stop timer BEFORE making API request

        if (totalFivePesoCoins > 0 || failedCoins > 0) {
            int totalToRequest = totalFivePesoCoins + failedCoins;
            Serial.println("⏳ 3 Seconds Passed! Final Total: " + String(totalToRequest));

            apiInProgress = true;
            apiRequest(totalToRequest);
        }
    }

    delay(100);
}

void setPrinterFont(uint8_t fontType) {
    uint8_t fontCommand[] = { 0x1B, 0x4D, fontType };
    mySerial.write(fontCommand, sizeof(fontCommand));
}

// Function to Print a Voucher
void printVoucher(String code, String productName, String paidAt) {
  Serial.println("🖨 Printing voucher: " + code);

  // Initialize printer and ensure settings are applied
  printer.wake(); // Ensure the printer is awake
  printer.feed(1); // Feed one line to clear any residual data

  printer.justify('C');
  printer.boldOn();
  printer.println("5GXMART TECHNOLOGIES CORP.");
  printer.boldOff();

  setPrinterFont(2);
  printer.println("Vat Reg: 646-091-737-00000");

  printer.setSize('S');
  printer.println("________________________________");

  // Print product name with medium font
  printer.justify('C'); // Ensure text is centered
  printer.setSize('M'); // Medium font size
  printer.println(productName);
  setPrinterFont(2);
  printer.println("Voucher Code");
  printer.setSize('L');
  printer.boldOn();
  printer.println(code);
  printer.boldOff();
  printer.setSize('S');
  printer.println("________________________________");
  printer.justify('L');
  setPrinterFont(2);
  printer.println(
    "How to connect? Follow these steps:\n"
    "1. Connect to GeekX_5G\n"
    "2. Popup shows, enter the code\n"
    "3. Open browser and type:\n"
    "   geekxfiber.technology/auth.htm\n"
    "4. SURF NOW\n\n"
    "Date Time:   " + paidAt + "\n"
    "Subtotal:    Php 4.40\n"
    "Vat:         Php 0.60\n"
    "Grand Total: Php 5.00\n"
  );

  printer.justify('C');
  setPrinterFont(1);
  printer.boldOn();
  printer.println("Powered by CoolGeeks");
  printer.boldOff();

  setPrinterFont(0);
  printer.println("================================\n\n");
  printer.feed(2);
}
