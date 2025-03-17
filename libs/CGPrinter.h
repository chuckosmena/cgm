#ifndef CGPRINTER_H
#define CGPRINTER_H

#include "CGConfig.h"

#include <SoftwareSerial.h>
#include "Adafruit_Thermal.h"

class CGPrinter {
private:
    SoftwareSerial printerSerial;
    Adafruit_Thermal printer;
    int rxPin, txPin, baudRate;
    int heatDots, heatTime, heatInterval;
    int fontCurrentType, fontCurrentIsBold;
    char fontCurrentSize, fontCurrentAlignment;

public:

    // Constructor with default values
    // CGPrinter(int rxPin = 10, int txPin = 11, int baudRate = 19200, int heatDots = 60, int heatTime = 2, int heatInterval = 9) 
    CGPrinter(
        int rxPin = PIN_PRINTER_RX, int txPin = PIN_PRINTER_TX, int baudRate = PRINTER_BAUD_RATE, 
        int heatDots = PRINTER_HEAT_DOTS, int heatTime = PRINTER_HEAT_TIME, int heatInterval = PRINTER_HEAT_INTERVAL
    ) : printerSerial(rxPin, txPin), printer(&printerSerial) {
        this->baudRate = baudRate;
        this->rxPin = rxPin;
        this->txPin = txPin;
        this->heatDots = heatDots;
        this->heatTime = heatTime;
        this->heatInterval = heatInterval;
    }

    // Return reference to printerSerial
    SoftwareSerial& serial() {
        return printerSerial;
    }

    void init() {
        begin();
        setHeatConfig(this->heatDots, this->heatDots, this->heatInterval);
        resetFontStyle();
    }

    // Initialize the printer
    void begin() {
        printerSerial.begin(baudRate);
        printer.begin();
    }

    // Set heat configuration
    CGPrinter& setHeatConfig(int heatDots, int heatTime, int heatInterval) {
        this->heatDots = heatDots;
        this->heatTime = heatTime;
        this->heatInterval = heatInterval;

        printer.setHeatConfig(heatDots, heatTime, heatInterval);

        return *this;
    }

    // Set heat configuration by Profile
    CGPrinter& setHeatConfigProfile(const char* profile) {
        if (strcmp(profile, "N") == 0 || strcmp(profile, "Normal") == 0) {
            Serial.println("[Normal] Balanced Quality & Speed (Most Common)");
            setHeatConfig(60, 2, 9);
        } else if (strcmp(profile, "F") == 0 || strcmp(profile, "Fast") == 0) {
            Serial.println("[Fast] Fastest Printing (Less Power Usage)");
            setHeatConfig(50, 1, 7);
        } else if (strcmp(profile, "H") == 0 || strcmp(profile, "HQ") == 0 || strcmp(profile, "High") == 0) {
            Serial.println("[High] Highest Quality (Darker Prints)");
            setHeatConfig(80, 3, 11);
        } else if (strcmp(profile, "E") == 0 || strcmp(profile, "Echo") == 0) {
            Serial.println("[Echo] Low-Power Mode (For Battery-Powered Devices)");
            setHeatConfig(40, 3, 5);
        }

        return *this;
    }

    void setFontStyle(char size = 'S', char alignment = 'L', bool isBold = false, int type = 9) {
        setSize(size);
        setBold(isBold);
        justify(alignment);

        if (type != 9)
        {
            setFontType(type);
        }
    }

    // fontType: 0 = default, 1 = small, 2 = extra small
    void setFontType(uint8_t fontType) {
        fontCurrentType = fontType;
        uint8_t fontCommand[] = {0x1B, 0x4D, fontType};
        printerSerial.write(fontCommand, sizeof(fontCommand));
    }

    // 'S' → Normal text (default), 'M' → Double height, 'L' → Double width and double height
    void setSize(char size) {
        fontCurrentSize = size;
        printer.setSize(size);
    }

    void setBold(bool isBold) {
        fontCurrentIsBold = isBold;
        isBold ? printer.boldOn() : printer.boldOff();
    }

    void resetFontStyle() {
        setFontStyle('S', 'L', 0, 0);
    }

    // Print a string
    void println(const char* text, char size = 'S', char alignment = 'L', bool isBold = false, int type = 9) {
        setFontStyle(size, alignment, isBold, type);
        printer.println(text);
    }

    // Print a string and Reset Style
    void printlnr(const char* text, char size = 'S', char alignment = 'L', bool isBold = false, int type = 9) {
        println(text, size, alignment, isBold, type);
        resetFontStyle();
    }

    // Set text justification
    void justify(char alignment) {
        fontCurrentAlignment = alignment;
        printer.justify(alignment);
    }

    void align(char alignment) {
        justify(alignment);
    }

    // Feed paper
    void feed(int lines) {
        printer.feed(lines);
    }

    // Enable bold text
    void boldOn() {
        setBold(1);
    }

    // Disable bold text
    void boldOff() {
        setBold(0);
    }

    // Wake up printer
    void wake() {
        printer.wake();
    }

    // Sleep printer
    void sleep() {
        printer.sleep();
    }

    // Reset printer
    void reset() {
        printer.reset();
    }
};

#endif // CGPRINTER_H
