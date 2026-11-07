#include <Arduino.h>
// Prueba de filas: enciende la columna 1, fila a fila
const int PIN_DATA  = 11;  // SER (pin 14) del chip 1
const int PIN_LATCH = 10;  // RCLK (pin 12) de ambos chips
const int PIN_CLOCK = 13;  // SRCLK (pin 11) de ambos chips

void writeRegs(byte filas, byte columnas) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, columnas);  // chip 2
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, filas);     // chip 1
  digitalWrite(PIN_LATCH, HIGH);
}

void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  writeRegs(0x00, 0xFF);  // todo apagado
}

void loop() { // una fila a HIGH cada vez
  for (int f = 0; f < 8; f++) {
    writeRegs(1 << f, 0xFF);   // El 0xFF es un relleno para cumplir con la función. CI 2 no conectado aún
    delay(1000);
  }
}
