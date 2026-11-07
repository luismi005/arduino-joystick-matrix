#include <Arduino.h>
// Snake en matriz LED 8x8 con joystick
// Mismo montaje: 2 x 74HC595 (D10 latch, D11 datos, D13 reloj)
// Joystick: VRx -> A0, VRy -> A1, SW -> D2
//
// Como se juega:
//  - El joystick cambia la direccion de la serpiente.
//  - La comida es el punto que parpadea. Al comerla, la serpiente crece
//    y va un poco mas rapido.
//  - Pierdes si chocas contra ti misma (o contra la pared, si PAREDES_MATAN).
//  - Al perder, la serpiente parpadea. Pulsa el joystick para jugar otra vez.
//  - La puntuacion aparece en el Monitor Serie (9600 baudios).

// ---------- Pines ----------
const int PIN_DATA  = 11;
const int PIN_LATCH = 10;
const int PIN_CLOCK = 13;
const int PIN_X  = A0;
const int PIN_Y  = A1;
const int PIN_SW = 2;

// ---------- Ajustes ----------
const bool INVERT_X = false;       // true si el eje X va al reves
const bool INVERT_Y = false;       // true si el eje Y va al reves
const bool PAREDES_MATAN = true;   // false: al salir por un borde, entra por el otro
const int  DEADZONE = 250;         // cuanto hay que mover la palanca

const unsigned long VEL_INICIAL = 350;  // ms entre pasos al empezar
const unsigned long VEL_MINIMA  = 120;  // ms entre pasos como maximo de velocidad
const unsigned long ACELERACION = 15;   // ms que se restan por cada comida

// ---------- Estado del juego ----------
byte cuerpoX[64], cuerpoY[64];   // posicion 0 = cabeza
int longitud;
int dirX, dirY;                  // direccion actual
int nuevaDirX, nuevaDirY;        // direccion pedida con el joystick
int comidaX, comidaY;
unsigned long ultimoPaso, intervalo, inicioFin;

enum Estado { JUGANDO, PERDIDO, GANADO };
Estado estado;

byte pantalla[8];                // lo que se dibuja: un byte por fila, un bit por columna

// ---------- Matriz ----------
void writeRegs(byte filas, byte columnas) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, columnas);  // chip 2
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, filas);     // chip 1
  digitalWrite(PIN_LATCH, HIGH);
}

void apagar() {
  writeRegs(0x00, 0xFF);   // filas a LOW, columnas a HIGH
}

// Multiplexado: una fila cada vez. Las filas vacias se saltan,
// asi las que tienen LEDs pasan mas tiempo encendidas y brillan mas.
void refresh() {
  for (int r = 0; r < 8; r++) {
    if (pantalla[r] == 0) continue;
    writeRegs(1 << r, ~pantalla[r]);
    delayMicroseconds(1500);
    apagar();
  }
}

void encenderPixel(int x, int y) {
  pantalla[y] |= (1 << x);
}

// ---------- Logica del juego ----------
bool ocupada(int x, int y) {
  for (int i = 0; i < longitud; i++) {
    if (cuerpoX[i] == x && cuerpoY[i] == y) return true;
  }
  return false;
}

void nuevaComida() {
  if (longitud >= 64) {          // no queda sitio: has ganado
    estado = GANADO;
    inicioFin = millis();
    Serial.println("Has ganado!");
    return;
  }
  do {
    comidaX = random(8);
    comidaY = random(8);
  } while (ocupada(comidaX, comidaY));
}

void iniciar() {
  longitud = 3;
  cuerpoX[0] = 2; cuerpoY[0] = 3;   // cabeza
  cuerpoX[1] = 1; cuerpoY[1] = 3;
  cuerpoX[2] = 0; cuerpoY[2] = 3;
  dirX = 1; dirY = 0;               // empieza moviendose a la derecha
  nuevaDirX = 1; nuevaDirY = 0;
  intervalo = VEL_INICIAL;
  estado = JUGANDO;
  nuevaComida();
  ultimoPaso = millis();
}

void perder() {
  estado = PERDIDO;
  inicioFin = millis();
  Serial.print("Fin. Puntuacion: ");
  Serial.println(longitud - 3);
}

void leerJoystick() {
  int x = analogRead(PIN_X) - 512;
  int y = analogRead(PIN_Y) - 512;
  if (INVERT_X) x = -x;
  if (INVERT_Y) y = -y;

  int dx = 0, dy = 0;
  // Nos quedamos con el eje que mas se ha movido
  if (abs(x) > abs(y) && abs(x) > DEADZONE) dx = (x > 0) ? 1 : -1;
  else if (abs(y) > DEADZONE)               dy = (y > 0) ? 1 : -1;

  if (dx == 0 && dy == 0) return;           // palanca en reposo
  if (dx == -dirX && dy == -dirY) return;   // no se puede dar media vuelta
  nuevaDirX = dx;
  nuevaDirY = dy;
}

void paso() {
  dirX = nuevaDirX;
  dirY = nuevaDirY;

  int hx = cuerpoX[0] + dirX;
  int hy = cuerpoY[0] + dirY;

  if (hx < 0 || hx > 7 || hy < 0 || hy > 7) {
    if (PAREDES_MATAN) { perder(); return; }
    hx = (hx + 8) % 8;
    hy = (hy + 8) % 8;
  }

  bool come = (hx == comidaX && hy == comidaY);

  // Choque consigo misma. Si no come, la cola se mueve,
  // asi que la ultima posicion no cuenta.
  int revisar = come ? longitud : longitud - 1;
  for (int i = 0; i < revisar; i++) {
    if (cuerpoX[i] == hx && cuerpoY[i] == hy) { perder(); return; }
  }

  if (come) longitud++;
  for (int i = longitud - 1; i > 0; i--) {   // cada parte avanza a la posicion de la anterior
    cuerpoX[i] = cuerpoX[i - 1];
    cuerpoY[i] = cuerpoY[i - 1];
  }
  cuerpoX[0] = hx;
  cuerpoY[0] = hy;

  if (come) {
    if (intervalo > VEL_MINIMA + ACELERACION) intervalo -= ACELERACION;
    else intervalo = VEL_MINIMA;
    nuevaComida();
  }
}

void construirPantalla() {
  for (int i = 0; i < 8; i++) pantalla[i] = 0;

  bool parpadeo;
  if (estado == JUGANDO) {
    parpadeo = true;                          // serpiente fija
  } else {
    parpadeo = (millis() / 250) % 2;          // al terminar, la serpiente parpadea
  }

  if (parpadeo) {
    for (int i = 0; i < longitud; i++) encenderPixel(cuerpoX[i], cuerpoY[i]);
  }

  if (estado == JUGANDO && (millis() / 150) % 2) {
    encenderPixel(comidaX, comidaY);          // la comida parpadea rapido
  }
}

// ---------- Programa principal ----------
void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_SW, INPUT_PULLUP);
  Serial.begin(9600);
  randomSeed(analogRead(A2));   // A2 esta libre: su ruido sirve como semilla aleatoria
  apagar();
  iniciar();
}

void loop() {
  if (estado == JUGANDO) {
    leerJoystick();
    if (millis() - ultimoPaso >= intervalo) {
      ultimoPaso = millis();
      paso();
    }
  } else {
    // Espera 1 segundo y luego reinicia al pulsar el joystick
    if (millis() - inicioFin > 1000 && digitalRead(PIN_SW) == LOW) iniciar();
  }

  construirPantalla();
  refresh();
}
