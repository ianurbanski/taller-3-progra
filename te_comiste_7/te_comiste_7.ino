#include <Adafruit_NeoPixel.h>

// --- CONFIGURACIÓN DE LOS DOS ANILLOS NEOPIXEL (Pines independientes) ---
const int PIN_ANILLO1 = 16;  // Pin físico D5 (GPIO 16)
const int PIN_ANILLO2 = 27;  // Pin físico D6 (GPIO 27)
const int LEDS_POR_ANILLO = 24;

Adafruit_NeoPixel anillo1(LEDS_POR_ANILLO, PIN_ANILLO1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel anillo2(LEDS_POR_ANILLO, PIN_ANILLO2, NEO_GRB + NEO_KHZ800);

// --- CONFIGURACIÓN DEL MOTOR (Wemos D1 R32) ---
const int PIN_MOTOR_ENA = 26; // Pin físico D2
const int PIN_MOTOR_IN1 = 25; // Pin físico D3
const int PIN_MOTOR_IN2 = 17; // Pin físico D4

const int frecuenciaPWM = 30000;
const int resolucionPWM = 8; 

// --- 🕹️ CONFIGURACIÓN DE LOS 3 BOTONES / PLACAS DE PRESIÓN ---
// Definimos los pines físicos D7, D8 y D11 (GPIOs 14, 12 y 13 de la Wemos D1 R32)
const int NUM_BOTONES = 3;
const int pinesBotones[NUM_BOTONES] = {14, 12, 13}; 

// --- VARIABLES DE CONTROL INTERACTIVO ---
float cantidadPersonas = 0; // Ahora se calcula sumando los botones presionados

float brilloActual = 15;
float brilloObjetivo = 15;

float velocidadMotorActual = 60; 
float velocidadMotorObjetivo = 60;

float suavizado = 0.05; // Mantiene las transiciones fluidas

// --- VARIABLES PARA EL TEMPORIZADOR SERIAL ---
unsigned long ultimoTiempoSerial = 0;
const unsigned long intervaloSerial = 500; // Reporte en consola cada medio segundo

// --- LÍMITES SEGUROS Y CALIBRACIÓN ---
const float BR_REPOSO = 15.0;      
const float BR_MAXIMO = 140.0;     
const float VEL_MIN_MOTOR = 60.0;  // Ralentí lento constante en 0 personas
const float VEL_MAX_MOTOR = 110.0; // Velocidad máxima segura para 3 personas

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(115200); 
  delay(500); 

  // Configuración de pines del motor
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  ledcAttach(PIN_MOTOR_ENA, frecuenciaPWM, resolucionPWM);

  // Inicialización de los pines de los botones con Pull-Up interno
  for (int i = 0; i < NUM_BOTONES; i++) {
    pinMode(pinesBotones[i], INPUT_PULLUP);
  }

  // Inicialización de luces
  anillo1.begin();
  anillo2.begin();
  anillo1.show();
  anillo2.show();
  
  Serial.println("--- SISTEMA INTERACTIVO CON 3 BOTONES LISTO ---");
}

void loop() {
  leerSensoresReales(); // LINEAS NUEVAS: Interpreta las señales físicas
  actualizarEstados();  
  controlarMotor();     
  controlarLuces();     
  imprimirEstadoSerial();

  delay(30); 
}

// 🧠 AQUÍ SE INTERPRETAN LAS SEÑALES DE LOS TRES BOTONES
void leerSensoresReales() {
  int contadorActivos = 0;

  // Recorremos el arreglo de botones uno por uno
  for (int i = 0; i < NUM_BOTONES; i++) {
    // Al usar INPUT_PULLUP, LOW significa que alguien está pisando/presionando
    if (digitalRead(pinesBotones[i]) == LOW) {
      contadorActivos++; 
    }
  }

  // Guardamos el resultado (puede ser 0, 1, 2 o 3 personas simultáneas)
  cantidadPersonas = contadorActivos;
}

void actualizarEstados() {
  // Ahora el rango máximo de entrada en el mapeo es 3.0 (porque tenemos 3 botones)
  brilloObjetivo = mapFloat(cantidadPersonas, 0.0, 3.0, BR_REPOSO, BR_MAXIMO);
  velocidadMotorObjetivo = mapFloat(cantidadPersonas, 0.0, 3.0, VEL_MIN_MOTOR, VEL_MAX_MOTOR);
}

void controlarMotor() {
  velocidadMotorActual += (velocidadMotorObjetivo - velocidadMotorActual) * suavizado;

  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);

  ledcWrite(PIN_MOTOR_ENA, (int)velocidadMotorActual);
}

void controlarLuces() {
  brilloActual += (brilloObjetivo - brilloActual) * suavizado;
  float brilloLimitado = constrain(brilloActual, BR_REPOSO, BR_MAXIMO);

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
    
    Serial.print("Personas en las placas: ");
    Serial.print((int)cantidadPersonas); // Muestra el entero directo (0, 1, 2 o 3)
    
    Serial.print("  |  Velocidad Motor: ");
    Serial.print((int)velocidadMotorActual);
    
    Serial.print("  |  Brillo Anillos: ");
    Serial.println((int)brilloActual);
  }
}