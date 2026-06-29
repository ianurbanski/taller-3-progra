#include <Adafruit_NeoPixel.h>

const int PIN_ANILLO1 = 16;  // Pin físico D5 (GPIO 16)
const int PIN_ANILLO2 = 27;  // Pin físico D6 (GPIO 27)
const int LEDS_POR_ANILLO = 24;

Adafruit_NeoPixel anillo1(LEDS_POR_ANILLO, PIN_ANILLO1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel anillo2(LEDS_POR_ANILLO, PIN_ANILLO2, NEO_GRB + NEO_KHZ800);

const int PIN_MOTOR_IN1 = 25;  // GPIO 25
const int PIN_MOTOR_ENA = 26;  // GPIO 26

const int frecuenciaPWM = 5000;
const int resolucionPWM = 8;

const int NUM_BOTONES = 3;
const int pinesBotones[NUM_BOTONES] = { 14, 12, 13 };

float cantidadPersonas = 0;
float brilloActual = 0;
float brilloObjetivo = 0;

int velocidadMotorObjetivo = 0;
int estadoAnterior = 0;

float suavizado = 0.05;

unsigned long ultimoTiempoSerial = 0;
const unsigned long intervaloSerial = 500;

const float BR_REPOSO = 0.0;
const float BR_ESTADO1 = 45.0;
const float BR_ESTADO2 = 90.0;
const float BR_MAXIMO = 115.0;

const int VEL_REPOSO_MOTOR = 0;
const int VEL_MAX_MOTOR = 115;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_MOTOR_IN1, OUTPUT);
  ledcAttach(PIN_MOTOR_ENA, frecuenciaPWM, resolucionPWM);
  digitalWrite(PIN_MOTOR_IN1, HIGH);

  for (int i = 0; i < NUM_BOTONES; i++) {
    pinMode(pinesBotones[i], INPUT_PULLUP);
  }

  anillo1.begin();
  anillo2.begin();
  anillo1.show();
  anillo2.show();

  Serial.println("--- SISTEMA CON PATADA INICIAL EN ESTADO MÁXIMO ---");
}

void loop() {
  leerSensoresReales();
  actualizarEstados();
  controlarMotor();
  controlarLuces();
  imprimirEstadoSerial();

  delay(30);
}

void leerSensoresReales() {
  int contadorActivos = 0;

  for (int i = 0; i < NUM_BOTONES; i++) {
    if (digitalRead(pinesBotones[i]) == LOW) {
      contadorActivos++;
    }
  }
  cantidadPersonas = contadorActivos;
}

void actualizarEstados() {
  int estadoActual = (int)cantidadPersonas;
  estadoAnterior = estadoActual;

  switch (estadoActual) {
    case 0:
      brilloObjetivo = BR_REPOSO;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      break;
    case 1:
      brilloObjetivo = BR_ESTADO1;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      break;
    case 2:
      brilloObjetivo = BR_ESTADO2;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      break;
    case 3:
      brilloObjetivo = BR_MAXIMO;
      velocidadMotorObjetivo = VEL_MAX_MOTOR;

      if (estadoAnterior < 3) {
        Serial.println("-> ¡Patada Inicial al Motor! Rompiendo inercia...");
        ledcWrite(PIN_MOTOR_ENA, 255);  // Mandamos fuerza máxima momentánea
        delay(150);                     // Sostiene el pulso por 150ms
      }
      break;
  }
}

void controlarMotor() {
  ledcWrite(PIN_MOTOR_ENA, velocidadMotorObjetivo);
}

void controlarLuces() {
  brilloActual += (brilloObjetivo - brilloActual) * suavizado;
  float brilloFinal = brilloActual;

  if ((int)cantidadPersonas == 3) {
    float velocidadLatido = 0.004;
    float factorOscilacion = (sin(millis() * velocidadLatido) + 1.0) / 2.0;
    float brilloMinLatido = 50.0;

    brilloFinal = brilloMinLatido + (factorOscilacion * (BR_MAXIMO - brilloMinLatido));
  }

  float brilloLimitado = constrain(brilloFinal, 0.0, BR_MAXIMO);

  for (int i = 0; i < LEDS_POR_ANILLO; i++) {
    int r = (255 * brilloLimitado) / BR_MAXIMO;
    int g = (197 * brilloLimitado) / BR_MAXIMO;
    int b = (110 * brilloLimitado) / BR_MAXIMO;

    anillo1.setPixelColor(i, anillo1.Color(r, g, b));
    anillo2.setPixelColor(i, anillo2.Color(r, g, b));
  }

  anillo1.show();
  anillo2.show();
}

void imprimirEstadoSerial() {
  unsigned long tiempoActual = millis();

  if (tiempoActual - ultimoTiempoSerial >= intervaloSerial) {
    ultimoTiempoSerial = tiempoActual;
    Serial.println((int)cantidadPersonas);
  }
}