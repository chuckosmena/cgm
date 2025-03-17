#include <ESP8266HTTPClient.h>

HTTPClient http;

// Define headers as a struct
struct HttpHeader {
    const char* key;
    const char* value;
};

// Array of headers
HttpHeader headers[] = {
    {"Content-Type", "application/json"},
    {"Authorization", "Bearer YOUR_API_KEY"},
    {"User-Agent", "ESP8266Client"}
};

// Number of headers
const int headerCount = sizeof(headers) / sizeof(headers[0]);

void setup() {
    Serial.begin(115200);
    WiFi.begin("GeekX_5G", "12345678");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    // Start HTTP request
    http.begin("https://www.example.com/api");

    // Loop through headers and add them
    for (int i = 0; i < headerCount; i++) {
        http.addHeader(headers[i].key, headers[i].value);
    }

    int httpCode = http.GET();
    if (httpCode > 0) {
        Serial.println("Response: " + http.getString());
    } else {
        Serial.println("HTTP Request failed!");
    }

    http.end();
}

void loop() {
}
