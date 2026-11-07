#include <Arduino.h>
// Prueba de la matriz: enciende los LEDs uno a uno,
// de izquierda a derecha y de arriba abajo.

const int PIN_DATA  = 11;  // SER (pin 14) del chip 1
const int PIN_LATCH = 10;  // RCLK (pin 12) de ambos chips
const int PIN_CLOCK = 13;  // SRCLK (pin 11) de ambos chips

const unsigned long PASO_MS = 150;   // tiempo que esta encendido cada LED

void writeRegs(byte rowBits, byte colBits) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, colBits);  // chip 2 (columnas)
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, rowBits);  // chip 1 (filas)
  digitalWrite(PIN_LATCH, HIGH);
}

void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  writeRegs(0x00, 0xFF);  // todo apagado
}

void loop() {
  for (int fila = 0; fila < 8; fila++) {
    for (int col = 0; col < 8; col++) {
      byte rowBits = 1 << fila;            // fila a HIGH
      byte colBits = ~(1 << col);          // columna a LOW
      writeRegs(rowBits, colBits);
      delay(PASO_MS);
    }
  }
  writeRegs(0x00, 0xFF);
  delay(1000);
}
