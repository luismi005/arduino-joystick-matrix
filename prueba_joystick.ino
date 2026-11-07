#include <Arduino.h>
void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
}

void loop() {
  Serial.print("X: ");
  Serial.print(analogRead(A0));
  Serial.print("   Y: ");
  Serial.print(analogRead(A1));
  Serial.print("   Boton: ");
  Serial.println(digitalRead(2) == LOW ? "PULSADO" : "suelto");
  delay(200);
}
