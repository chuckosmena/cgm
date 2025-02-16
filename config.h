#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char *WIFI_SSID = "COOLGEEKS2025-2G";
const char *WIFI_PASS = "COOLGEEKS2025";

// API Configurations
const char *SERVER_API_URL = "https://www.coolgeeks.technology/api/machine/vouchers/buy/5/";
const char *SERVER_API_KEY = "anOYwkrGolqM2RtLo8hJiQmXaQB75Qv751HRQKOjuRHbb0276bad809d73e4";

// Coin Slot Settings
#define COIN_PIN D5        // D5 -> Coin Slot Pulse Output
#define VALID_PULSES 5     // 5 pulses = 1 valid 5-peso coin
#define PULSE_TIMEOUT 500  // Max time between pulses (ms) before reset
#define MIN_PULSE_INTERVAL 100 // Ignore pulses that come too fast (debounce)
#define RESET_DELAY 2000   // 2 seconds after last coin to print & reset

// Define RX and TX for SoftwareSerial
#define TX_PIN 13 // D7 -> Connected to printer RX
#define RX_PIN 12 // Not used
#define PRINTER_BAUD 9600

// Global Variables
volatile int pulseCounter = 0;
volatile int totalFivePesoCoins = 0;
volatile int failedCoins = 0; // Stores coins from failed API requests
volatile int newCoinsDuringAPI = 0; // Track coins inserted during API call
unsigned long lastPulseTime = 0;
bool resetTimerActive = false; // Track if reset countdown started
unsigned long resetStartTime = 0; // Track when the reset timer starts
bool apiInProgress = false; // Add a new flag to track API request status

#endif // CONFIG_H
