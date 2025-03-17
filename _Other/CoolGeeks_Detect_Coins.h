//*** DETECT 5 & 10 PESO COIN! ***//

#define COIN_SIGNAL D2  // Use GPIO4 (D2)

volatile int pulseCount = 0;
unsigned long lastPulseTime = 0;
bool coinInsertedFlag = false;
const int timeThreshold = 200;  // Time in milliseconds to group pulses

int totalFivePeso = 0;   // Total ₱5 coins inserted
int totalTenPeso = 0;    // Total ₱10 coins inserted

void IRAM_ATTR coinInserted() {
  unsigned long currentTime = millis();

  if (currentTime - lastPulseTime > timeThreshold) {
    // If a long time has passed, start a new coin count
    pulseCount = 1;
    coinInsertedFlag = true;
  } else {
    // Otherwise, count this as part of the same coin
    pulseCount++;
  }

  lastPulseTime = currentTime;
}

void setup() {
  Serial.begin(115200);
  pinMode(COIN_SIGNAL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COIN_SIGNAL), coinInserted, FALLING);
}

void loop() {
  if (coinInsertedFlag && millis() - lastPulseTime > timeThreshold) {
    Serial.print("Coin inserted! Pulses detected: ");
    Serial.println(pulseCount);

    int remainingPulses = pulseCount;  // Store total pulses for breakdown

    while (remainingPulses >= 3) { // Ensure at least a valid coin amount is detected
      if (remainingPulses >= 10) {
        totalTenPeso++;
        Serial.println("₱10 Coin detected! ✅");
        remainingPulses -= 10;  // Remove ₱10 pulse count
      } else if (remainingPulses >= 5) {
        totalFivePeso++;
        Serial.println("₱5 Coin detected! ✅");
        remainingPulses -= 5;  // Remove ₱5 pulse count
      }
    }

    // Print total coins collected
    Serial.print("Total ₱10 Coins: ");
    Serial.println(totalTenPeso);
    Serial.print("Total ₱5 Coins: ");
    Serial.println(totalFivePeso);

    // Reset pulse count for next detection
    pulseCount = 0;
    coinInsertedFlag = false;
  }
}
