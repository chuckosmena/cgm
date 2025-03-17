#include <ESP8266WiFi.h>

const char* ssid = "Your_SSID";     // Change this to your Wi-Fi SSID
const char* password = "Your_PASS"; // Change this to your Wi-Fi Password

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected! ✅");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi connection failed ❌");
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost! Reconnecting...");
    WiFi.disconnect();
    delay(1000);
    connectToWiFi();
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Station mode
  connectToWiFi();
}

void loop() {
  checkWiFiConnection();
  delay(5000); // Check connection every 5 seconds
}
