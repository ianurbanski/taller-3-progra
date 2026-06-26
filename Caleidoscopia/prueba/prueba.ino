#include <Adafruit_NeoPixel.h>

const int PIN_ANILLO1 = 16;  // Pin físico D5 (GPIO 16)
const int PIN_ANILLO2 = 27;  // Pin físico D6 (GPIO 27)
const int LEDS_POR_ANILLO = 24;

Adafruit_NeoPixel anillo1(LEDS_POR_ANILLO, PIN_ANILLO1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel anillo2(LEDS_POR_ANILLO, PIN_ANILLO2, NEO_GRB + NEO_KHZ800);

// CONFIGURACIÓN DEL MOTOR
const int PIN_MOTOR_IN1 = 25;  // Control de encendido (GPIO 25)
const int PIN_MOTOR_ENA = 26;  // Control de velocidad PWM (GPIO 26)

const int frecuenciaPWM = 5000; 
const int resolucionPWM = 8;    

// Pines físicos de los botones
const int NUM_BOTONES = 3;
const int pinesBotones[NUM_BOTONES] = { 14, 12, 13 };

float cantidadPersonas = 0;
float brilloActual = 0; 
float brilloObjetivo = 0;

// Cambiado a int para control directo y constante
int velocidadMotorObjetivo = 0; 

float suavizado = 0.05; // Aplica solo a las luces

unsigned long ultimoTiempoSerial = 0;
const unsigned long intervaloSerial = 500;

// Ajustes de límites para luces
const float BR_REPOSO = 0.0;    // Estado 0 = Totalmente apagado
const float BR_ESTADO1 = 45.0;  // Estado 1 = Brillo tenue inicial
const float BR_ESTADO2 = 90.0;  // Estado 2 = Brillo medio
const float BR_MAXIMO = 140.0;  // Estado 3 = Brillo máximo (antes del latido)

// Ajustes de límites para el motor (Valores de 0 a 255)
const int VEL_MOTOR_APAGADO = 0;  // Estados 0, 1 y 2 = Motor detenido
const int VEL_MOTOR_MAXIMA = 255; // Estado 3 = Motor a máxima velocidad constante

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

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

  Serial.println("--- SISTEMA CONFIGURADO: MOTOR SOLO EN ESTADO 3 ---");
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
  int estado = (int)cantidadPersonas;
  
  switch (estado) {
    case 0:
      brilloObjetivo = BR_REPOSO;           
      velocidadMotorObjetivo = VEL_MOTOR_APAGADO; 
      break;
    case 1:
      brilloObjetivo = BR_ESTADO1;          
      velocidadMotorObjetivo = VEL_MOTOR_APAGADO; // Sigue apagado
      break;
    case 2:
      brilloObjetivo = BR_ESTADO2;          
      velocidadMotorObjetivo = VEL_MOTOR_APAGADO; // Sigue apagado
      break;
    case 3:
      brilloObjetivo = BR_MAXIMO;           
      velocidadMotorObjetivo = VEL_MOTOR_MAXIMA;  // ¡Arranca al máximo!
      break;
  }
}

void controlarMotor() {
  // Se quitó el suavizado. El motor cambia de forma instantánea y constante.
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