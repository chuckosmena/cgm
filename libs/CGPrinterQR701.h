#ifndef CGPRINTERQR701_H
#define CGPRINTERQR701_H

#include <SoftwareSerial.h>

class CGPrinterQR701 {
private:
    SoftwareSerial printer;
    int rxPin, txPin, baudRate;

public:
    char fontCurrentSize;
    char fontCurrentAlignment;
    int fontCurrentType;
    int fontCurrentIsBold;

    // Constructor with default values
    CGPrinterQR701(int rxPin = 10, int txPin = 11, int baudRate = 19200)
        : printer(rxPin, txPin) {
        this->baudRate = baudRate;
        this->rxPin = rxPin;
        this->txPin = txPin;
    }

    // Return reference to printerSerial
    SoftwareSerial& serial() {
        return printerSerial;
    }

    void init() {
        begin();
    }

    // Initialize the printer
    void begin() {
        printer.begin(baudRate);
    }

    // Function to Print Text with Formatting
    void println(const char *text, bool bold, bool underline, int size) {
        printer.write(0x1B); printer.write(0x21); // Select font
        printer.write(size == 2 ? 0x10 : 0x00);  // Double height if size=2

        if (bold) { 
            printer.write(0x1B); printer.write(0x45); printer.write(1); // Bold ON
        } else {
            printer.write(0x1B); printer.write(0x45); printer.write(0);  // Bold OFF
        }

        if (underline) { 
            printer.write(0x1B); printer.write(0x2D); printer.write(1); // Underline ON
        } else {
            printer.write(0x1B); printer.write(0x2D); printer.write(0); // Underline OFF
        }

        printer.println(text);
        printer.write(0x0A); // Newline
    }

    // Function to Print a QR Code
    void printQRCode(const char *data) {
        printer.write(0x1D); printer.write(0x28); printer.write(0x6B); // QR code command
        printer.write(0x03); printer.write(0x00); // Store data
        printer.print(data);
        printer.write(0x0A); // Newline
    }

    // Function to Print a Barcode
    void printBarcode(const char *code) {
        printer.write(0x1D); printer.write(0x6B); // Select barcode mode
        printer.write(0x02); // Code 128
        printer.print(code);
        printer.write(0x0A); // Newline
    }

    // Function to Feed Paper
    void feedPaper(int lines) {
        for (int i = 0; i < lines; i++) {
            printer.write(0x0A);
        }
    }

    // Function to Cut Paper (if supported)
    void cutPaper() {
        printer.write(0x1D); printer.write(0x56); printer.write(0x00);
    }
};

#endif // CGPRINTERQR701_H
