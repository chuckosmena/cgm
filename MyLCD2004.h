#ifndef MYLCD2004_H
#define MYLCD2004_H

#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

class MyLCD2004 {
private:
    LiquidCrystal_PCF8574 lcd;
    int sda, scl;

public:
    // Constructor with default values
    MyLCD2004(uint8_t address = 0x27, int sdaPin = D2, int sclPin = D1) 
        : lcd(address), sda(sdaPin), scl(sclPin) {}

    void begin() {
        Wire.begin(sda, scl);
        lcd.begin(20, 4);
        lcd.setBacklight(255);
        lcd.clear();
    }

    void reset() {
        Wire.beginTransmission(0x27);  // Replace lcd.getAddress() with actual I2C address
        Wire.write(0x01);  // Send a clear command
        Wire.endTransmission();
        
        lcd.begin(20, 4);   // Reinitialize LCD
        lcd.setBacklight(255);
        lcd.clear();
        lcd.home();
        printCenter("GeekX_5G", 0);
        printCenter("Powered by CoolGeeks", 3);
    }

    // ✅ Blink text on a specific row
    void blinkText(const char *text, int row, int duration, int speed) {
        int col = (20 - strlen(text)) / 2;  // Center the text
        unsigned long startTime = millis();

        while (millis() - startTime < duration) {
            lcd.setCursor(col, row);
            lcd.print(text);  // Show text
            delay(speed);

            lcd.setCursor(col, row);
            for (int i = 0; i < strlen(text); i++) {
                lcd.print(" ");  // Print spaces to hide text
            }
            delay(speed);
        }
    }

    void insertCoinText() {
        printCenter("Insert Coin!", 1);
    }

    void coinInsertedText() {
        printCenter("Coin inserted!", 1);
    }

    void printCenter(const char *text, int row, int clear = 1) {
        if (clear == 1)
        {
          clearRow(row);
        }

        int length = strlen(text);
        int col = (20 - length) / 2;
        lcd.setCursor(col, row);
        lcd.print(text);
    }

    void printLeft(const char *text, int row, int clear = 1) {
        if (clear == 1)
        {
          clearRow(row);
        }
        
        lcd.setCursor(0, row);
        lcd.print(text);
    }

    void printRight(const char *text, int row, int clear = 1) {
        if (clear == 1)
        {
          clearRow(row);
        }
        
        int length = strlen(text);
        int col = 20 - length;
        lcd.setCursor(col, row);
        lcd.print(text);
    }

    void clearRow(int targetRow) {
        lcd.setCursor(0, targetRow);
        lcd.print("                    "); // 20 spaces to clear the row
    }

    void marqueeText(const char *text, int row, int speed) {
        int textLength = strlen(text);
        char buffer[21];

        while (true) {
            for (int i = 0; i < textLength + 20; i++) {
                clearRow(row);

                int start = max(0, i - 20);
                int end = min(textLength, i);
                int j = 0;
                for (int k = start; k < end; k++) {
                    buffer[j++] = text[k];
                }
                buffer[j] = '\0';

                lcd.setCursor(0, row);
                lcd.print(buffer);

                delay(speed);
            }
        }
    }
};

#endif
