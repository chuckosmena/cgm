// 2025-02-14
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>  // Use WiFiClientSecure for HTTPS
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include "MyLCD2004.h"

// Define RX and TX for SoftwareSerial
#define TX_PIN D7 // D7 -> Connected to printer RX
#define RX_PIN D8 // Not used
#define PRINTER_BAUD 9600


 // Pin Definitions
#define RELAY_PIN D5 // CoinSlot Controller
#define COIN_PIN D6 // D6 -> Coin Slot Pulse Output
#define VALID_PULSES 5 // 5 pulses = 1 valid 5-peso coin
#define PULSE_TIMEOUT 500 // Max time between pulses (ms) before reset
#define MIN_PULSE_INTERVAL 100 // Ignore pulses that come too fast (debounce)
#define RESET_DELAY 2000 // 2 seconds after last coin to print & reset

// Global Variables
volatile int pulseCounter = 0;
volatile int totalFivePesoCoins = 0;
volatile int failedCoins = 0; // Stores coins from failed API requests
volatile int newCoinsDuringAPI = 0; // Track coins inserted during API call
unsigned long lastPulseTime = 0;
bool resetTimerActive = false; // Track if reset countdown started
unsigned long resetStartTime = 0; // Track when the reset timer starts
bool apiInProgress = false; // Add a new flag to track API request status

const char * VERSION = "1.0.0";
// const char * WIFI_SSID = "COOLGEEKS2025-2G";
// const char * WIFI_PASS = "COOLGEEKS2025";
const char * WIFI_SSID = "GeekX_5G";
const char * WIFI_PASS = "12345678";
const char * SERVER_API_URL = "https://www.coolgeeks.technology/api/machine/vouchers/buy/5/";
const char * SERVER_API_KEY = "mb7noFfVqpZjPwfP0qn5IRAHqq0OMSjQPpg2axQvvNjE009124cb8a936e52";

// Set up HTTP client
HTTPClient http;
WiFiClientSecure client;

// Initialize SoftwareSerial and Printer
SoftwareSerial mySerial(RX_PIN, TX_PIN);
Adafruit_Thermal printer( & mySerial);

// LCD
//MyLCD2004 lcd(0x27, D3, D4); // Change 0x27 to your LCD address, D3/D4 to your SDA/SCL pins
MyLCD2004 lcd;


// Interrupt Service Routine (ISR) for Coin Insertion
void IRAM_ATTR coinInserted() {
  unsigned long now = millis();

  // Ignore noise: If two pulses come too fast (<MIN_PULSE_INTERVAL), discard it
  if(now - lastPulseTime < MIN_PULSE_INTERVAL) {
    return;
  }

  // Double-check the pin state before counting the coin
  if(digitalRead(COIN_PIN) == HIGH) {
    return; // Ignore false pulses
  }

  // If more than PULSE_TIMEOUT ms passed, assume new coin
  if(now - lastPulseTime > PULSE_TIMEOUT) {
    pulseCounter = 0;
  }

  lastPulseTime = now;
  pulseCounter++;

  if(pulseCounter >= VALID_PULSES) {
    // Track coins inserted during API request
    if(apiInProgress) {
      newCoinsDuringAPI++; // Buffer coins during API call
    } else {
      totalFivePesoCoins++; // Register 1 valid coin
    }
    pulseCounter = 0; // Reset pulse counter for the next coin
    Serial.println("✅ 5 Peso Coin Inserted! Total: " + String(totalFivePesoCoins + newCoinsDuringAPI));
    lcd.coinInsertedText();

    // Start/reset the reset timer
    resetTimerActive = true;
    resetStartTime = millis();
  }
}

// Function to enable the coin slot
void enableCoinSlot() {
    digitalWrite(RELAY_PIN, HIGH);  // Turns the relay ON (coin slot enabled)
}

// Function to disable the coin slot
void disableCoinSlot() {
    digitalWrite(RELAY_PIN, LOW);  // Turns the relay OFF (coin slot disabled)
}

// Wi-Fi Connection
void wifiConnection() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.println("🔄 Connecting to Wi-Fi...");
    lcd.printCenter("Connecting... Wait!", 1);
    
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) { 
        delay(1000);
        Serial.print(".");
        retryCount++;
        // Serial.print(" Status: ");
        // Serial.println(WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED) {
        lcd.insertCoinText();
        Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());
        digitalWrite(LED_BUILTIN, LOW);
        enableCoinSlot();
    } else {
        lcd.printCenter("Wi-Fi Failed!", 1);
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

    if (WiFi.status() == WL_CONNECTED) {
        unsigned long currentMillis = millis();

        // Blink LED every 500ms when connected
        if (currentMillis - previousMillis >= 500) {
            previousMillis = currentMillis;
            ledState = !ledState; // Toggle LED state
            digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH); // Active LOW logic
        }

        if (wasDisconnected) {
            enableCoinSlot();
            Serial.println("✅ Reconnected! IP: " + WiFi.localIP().toString());
            lcd.insertCoinText();
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
            lcd.printCenter("Reconnecting...", 1);

            WiFi.mode(WIFI_STA);  // Ensure STA mode
            WiFi.disconnect();  // Reset connection
            delay(100);  // Short delay to stabilize
            WiFi.begin(WIFI_SSID, WIFI_PASS); // Reconnect

            lastReconnectAttempt = currentMillis;
            wasDisconnected = true;
        }
    }
}

// API Request Function
void apiRequest(int quantity) {
  if (WiFi.status() == WL_CONNECTED) {
    lcd.printCenter("Buying Voucher", 1);
    Serial.println("Requesting to " + String(SERVER_API_URL) + String(quantity));
    client.setInsecure();
    http.begin(client, SERVER_API_URL + String(quantity));
    http.addHeader("GEEKX-API-KEY", SERVER_API_KEY);
    http.addHeader("Accept", "application/json");
    http.setTimeout(8000);

    int httpCode = http.GET();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);

    if(httpCode > 0) {
      Serial.println("Request successful, parsing JSON...");
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, http.getString());

      if(!error) {
        JsonObject data = doc.as < JsonObject > ();
        if(data.containsKey("vouchers")) {
          for(JsonVariant voucherCode: data["vouchers"].as < JsonArray > ()) {
            printVoucher(voucherCode.as < String > (), data["product"]["name"].as < String > (), data["paid_at"].as < String > ());
          }
        }
        failedCoins = 0; // Clear failed coins since API worked
      } else {
        Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
        failedCoins += quantity; // Add failed coins back
      }
    } else {
      Serial.println("❌ Request failed, HTTP code: " + String(httpCode));
      failedCoins += quantity; // Add failed coins back
    }

    http.end();
  } else {
    Serial.println("❌ Wi-Fi not connected");
    failedCoins += quantity; // Add failed coins back
  }

  // ✅ Merge new coins that were inserted during API request
  totalFivePesoCoins = newCoinsDuringAPI;
  newCoinsDuringAPI = 0;
  apiInProgress = false; // ✅ Allow new coins to be counted
}

void setPrinterFont(uint8_t fontType) {
  uint8_t fontCommand[] = {
    0x1B,
    0x4D,
    fontType
  };
  mySerial.write(fontCommand, sizeof(fontCommand));
}

// Function to Print a Voucher
void printVoucher(String code, String productName, String paidAt) {
  Serial.println("🖨 Printing Voucher: " + code);
  lcd.printCenter("Printing Voucher", 1);

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

  // ✅ Estimate print time (3ms per character, adjust as needed)
  String fullText = code + productName + paidAt;
  int estimatedPrintTime = strlen(fullText.c_str()) * 3 + 3000;
  delay(estimatedPrintTime);

  lcd.insertCoinText();
}

// Setup Function
void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  // Initialize MyLCD
  lcd.begin();
  lcd.reset();

  // Initialize the printer
  printer.begin();
  printer.setHeatConfig(200, 255, 250); // Adjust heat settings if necessary

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Start with LED OFF

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Start with the coin slot OFF

  wifiConnection();

  // Setup Coin Interrupt
  pinMode(COIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COIN_PIN), coinInserted, FALLING);
}

void loop() {
	checkWiFiReconnect(); // Check Wi-Fi and reconnect if necessary

  // Only process reset if API is NOT in progress
  if(!apiInProgress && resetTimerActive && (millis() - resetStartTime >= RESET_DELAY)) {
    resetTimerActive = false; // Stop timer BEFORE making API request

    if(totalFivePesoCoins > 0 || failedCoins > 0) {
      int totalToRequest = totalFivePesoCoins + failedCoins;
      Serial.println("⏳ 3 Seconds Passed! Final Total: " + String(totalToRequest));

      // Mark API request as in progress
      apiInProgress = true;
      apiRequest(totalToRequest);
    }
  }

  delay(100);
}