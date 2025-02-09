#define RELAY_COIN_SLOT D1  // Relay connected to the coin slot

void setup() {
  pinMode(RELAY_COIN_SLOT, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  Serial.println("Coin slot ON ✅");
  digitalWrite(RELAY_COIN_SLOT, HIGH);  // Turn ON coin slot
  delay(5000);  // Keep it ON for 5 seconds

  Serial.println("Coin slot OFF ❌");
  digitalWrite(RELAY_COIN_SLOT, LOW);   // Turn OFF coin slot
  delay(5000);  // Keep it OFF for 5 seconds
}
