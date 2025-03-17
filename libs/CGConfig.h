#ifndef CGCONFIG_H
#define CGCONFIG_H

// Constant Variables
const char* kVersion        = "1.0.0";
const char* kWiFiSsid       = "GeekX_5G";
const char* kWiFiPass       = "12345678";
const char* kServerApiKey   = "mb7noFfVqpZjPwfP0qn5IRAHqq0OMSjQPpg2axQvvNjE009124cb8a936e52";
const char* kServerApiUrl   = "https://www.coolgeeks.technology/api/machine/vouchers/buy/5/";
const char* kDevicePrinter  = "T7-S - 58mm Embedded Type Thermal Printer Serial (TTL and RS-232 Level)";
const char* kDeviceLCD      = "LCD2004 - Screen 20X4 Character LCD Display IIC I2C Serial Interface Adapter Module";

// Pin Definitions
#define PIN_LCD_SCL         D1  // LCD SCL
#define PIN_LCD_SDA         D2  // LCD SDA
#define PIN_COIN_RELAY      D5  // Coin slot switch/controller
#define PIN_COIN_PULSE      D6  // Coin slot pulse output
#define PIN_PRINTER_TX      D7  // Printer TX (Connected to RX of printer)
#define PIN_PRINTER_RX      D8  // Not used

// Coin Slot Settings
#define LCD_ADDRESS                 0x27    // The LCD address, common ones are: 0x3F, 0x27, 0x20, 0x38
#define COIN_VALID_PULSES           5       // 5 pulses = 1 valid 5-peso coin
#define COIN_PULSE_TIMEOUT_MS       500     // Max time between pulses (ms) before reset
#define COIN_MIN_PULSE_INTERVAL_MS  100     // Ignore pulses that come too fast (debounce)
#define COIN_RESET_DELAY_MS         2000    // 2 seconds after last coin to print & reset
#define PRINTER_BAUD_RATE           9600    // Printer: Serial baud rate
#define PRINTER_HEAT_DOTS           60      // Printer: HeatConfig dots
#define PRINTER_HEAT_TIME           2       // Printer: HeatConfig time
#define PRINTER_HEAT_INTERVAL       9       // Printer: HeatConfig interval
#define PRINTER_FONT_SIZE           'S'     // Printer: Font size  ['S' = Normal text (default), 'M' = Double height, 'L' = Double width and double height]
#define PRINTER_FONT_ALIGN          'L'     // Printer: Font align ['L' = Left, 'C' = Center, 'R' = Right]
#define PRINTER_FONT_BOLD           false   // Printer: Font bold
#define PRINTER_FONT_TYPE           0       // Printer: Font type  [0 = default, 1 = small, 2 = extra small]

// Global Variables
volatile int gPulseCounter          = 0;        // Counts coin pulses
volatile int gTotalFivePesoCoins    = 0;        // Total valid coins inserted
volatile int gFailedCoins           = 0;        // Stores coins from failed API requests
volatile int gNewCoinsDuringApi     = 0;        // Tracks coins inserted during API call
unsigned long gLastPulseTime        = 0;        // Last pulse timestamp
unsigned long gResetStartTime       = 0;        // Track when the reset timer starts
bool gResetTimerActive              = false;    // Flag for reset countdown
bool gApiInProgress                 = false;    // Flag to track API request status

#endif // CGCONFIG_H
