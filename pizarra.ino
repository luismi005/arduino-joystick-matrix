#include <Arduino.h>
// Matriz LED 8x8 controlada con joystick
// Arduino Uno + 2 x 74HC595 en cascada
// Registro 1 -> filas, Registro 2 -> columnas

// ---------- Pines ----------
const int PIN_DATA  = 11;  // DS (pin 14) del primer 595
const int PIN_LATCH = 10;  // ST_CP (pin 12) de ambos 595
const int PIN_CLOCK = 13;  // SH_CP (pin 11) de ambos 595
const int PIN_X  = A0;     // VRx del joystick
const int PIN_Y  = A1;     // VRy del joystick
const int PIN_SW = 2;      // SW del joystick (pulsador)

// ---------- Ajustes segun matriz ----------
const bool ROW_ACTIVE_HIGH = true;  // fila activa con HIGH (anodos en filas)
const bool COL_ACTIVE_LOW  = true;  // columna activa con LOW (catodos en columnas)

const bool INVERT_X = false;
const bool INVERT_Y = false;

const int DEADZONE = 200;              // zona muerta del joystick (0-512)
const unsigned long MOVE_MS = 150;     // tiempo entre pasos
const unsigned long DEBOUNCE_MS = 200; // antirrebote del boton

// ---------- Estado ----------
byte frame[8];        // dibujo guardado: un byte por fila, un bit por columna
int px = 3, py = 3;   // posicion del cursor
bool paintMode = false;
unsigned long lastMove = 0, lastBtn = 0;
bool lastBtnState = HIGH;

void writeRegs(byte rowBits, byte colBits) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, colBits);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, rowBits);
  digitalWrite(PIN_LATCH, HIGH);
}

void blank() {
  writeRegs(0x00, 0xFF);   // todas las filas a LOW, todas las columnas a HIGH. Matriz apagada
}

// Multiplexado: enciende una fila cada vez, muy rapido
void refresh() {
  for (int r = 0; r < 8; r++) {
    byte cols = frame[r];
    if (r == py) cols |= (1 << px);  // el cursor siempre visible

    byte rowBits = ROW_ACTIVE_HIGH ? (1 << r) : (byte)~(1 << r);
    byte colBits = COL_ACTIVE_LOW ? (byte)~cols : cols;

    writeRegs(rowBits, colBits);
    delayMicroseconds(1500);
    blank();  // evita "fantasmas" al cambiar de fila
  }
}

void readJoystick() {
  if (millis() - lastMove < MOVE_MS) return;

  int x = analogRead(PIN_X) - 512;
  int y = analogRead(PIN_Y) - 512;
  if (INVERT_X) x = -x;
  if (INVERT_Y) y = -y;

  bool moved = false;
  if (x >  DEADZONE && px < 7) { px++; moved = true; }
  if (x < -DEADZONE && px > 0) { px--; moved = true; }
  if (y >  DEADZONE && py < 7) { py++; moved = true; }
  if (y < -DEADZONE && py > 0) { py--; moved = true; }

  if (moved) {
    lastMove = millis();
    if (paintMode) frame[py] |= (1 << px);  // deja rastro
  }
}

void readButton() {
  bool state = digitalRead(PIN_SW);
  if (state == LOW && lastBtnState == HIGH && millis() - lastBtn > DEBOUNCE_MS) {
    lastBtn = millis();
    paintMode = !paintMode;
    if (paintMode) {
      frame[py] |= (1 << px);
    } else {
      for (int i = 0; i < 8; i++) frame[i] = 0;  // al salir, borra el dibujo
    }
  }
  lastBtnState = state;
}

void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_SW, INPUT_PULLUP);
  for (int i = 0; i < 8; i++) frame[i] = 0;
  blank();
}

void loop() {
  readJoystick();
  readButton();
  refresh();
}
