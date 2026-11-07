#include <Arduino.h>
void setup() {
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);     // negativo

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);   // positivo
}

void loop() {
}
