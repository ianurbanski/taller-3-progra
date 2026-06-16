#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// --- 📶 CONFIGURACIÓN WI-FI (MODO PUNTO DE ACCESO) ---
const char* ssid = "Red_Instalacion_FBA"; // El nombre del Wi-Fi que creará la Wemos
const char* password = "clave_segura_123";  // La contraseña para conectarte desde la PC

// IP por defecto del ESP32 en modo AP es 192.168.4.1. 
// Al conectarse, la PC suele recibir automáticamente la IP 192.168.4.2
const char* ipPC = "192.168.4.2"; 
const unsigned int puertoTD = 10000; // El puerto de escucha para TouchDesigner

WiFiUDP udp;

// --- CONFIGURACIÓN DE LOS DOS ANILLOS NEOPIXEL ---
const int PIN_ANILLO1 = 16;  // Pin físico D5 (GPIO 16)
const int PIN_ANILLO2 = 27;  // Pin físico D6 (GPIO 27)
const int LEDS_POR_ANILLO = 24;

Adafruit_NeoPixel anillo1(LEDS_POR_ANILLO, PIN_ANILLO1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel anillo2(LEDS_POR_ANILLO, PIN_ANILLO2, NEO_GRB + NEO_KHZ800);

// --- CONFIGURACIÓN DEL MOTOR ---
const int PIN_MOTOR_ENA = 26; // Pin físico D2
const int PIN_MOTOR_IN1 = 25; // Pin físico D3
const int PIN_MOTOR_IN2 = 17; // Pin físico D4

const int frecuenciaPWM = 30000;
const int resolucionPWM = 8; 

// --- CONFIGURACIÓN DE LOS 3 BOTONES / PLACAS ---
const int NUM_BOTONES = 3;
const int pinesBotones[NUM_BOTONES] = {14, 12, 13}; // Pines físicos D7, D8, D11

// Guardan el estado físico de los botones (HIGH = libre, LOW = pisado)
int estadosAnteriores[NUM_BOTONES] = {HIGH, HIGH, HIGH}; 

// --- VARIABLES DE CONTROL INTERACTIVO ---
float cantidadPersonas = 0;
float brilloActual = 15;
float brilloObjetivo = 15;
float velocidadMotorActual = 60; 
float velocidadMotorObjetivo = 60;
float suavizado = 0.05; 

// --- LÍMITES SEGUROS ---
const float BR_REPOSO = 15.0;      
const float BR_MAXIMO = 140.0;     
const float VEL_MIN_MOTOR = 60.0;  
const float VEL_MAX_MOTOR = 110.0; 

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

  // 📶 INICIALIZAR LA RED WI-FI PROPIA
  WiFi.softAP(ssid, password);
  
  Serial.println("--- SISTEMA INALÁMBRICO OSC INICIADO ---");
  Serial.println("Red Wi-Fi: Red_Instalacion_FBA");
  Serial.println("Conectá la PC a esta red y configurá el puerto 10000 en TD.");
}

void loop() {
  procesarBotonesYAudio(); // Controla la lógica física y envía eventos por OSC
  actualizarEstados();  
  controlarMotor();     
  controlarLuces();     

  delay(30); 
}

// 🔊 SENSADO INDEPENDIENTE Y ENVÍO DE EVENTOS OSC (Sincronizado con TD)
void procesarBotonesYAudio() {
  int contadorActivos = 0;

  for (int i = 0; i < NUM_BOTONES; i++) {
    int estadoLectura = digitalRead(pinesBotones[i]);

    // ¿Hubo un cambio en el botón respecto a la vuelta anterior?
    if (estadoLectura != estadosAnteriores[i]) {
      
      // Creamos el mensaje OSC con la dirección (/boton1, /boton2, /boton3)
      String direccionOSC = "/boton" + String(i + 1);
      OSCMessage msg(direccionOSC.c_str());

      if (estadoLectura == LOW) {
        // Alguien PISÓ el botón: mandamos un 1
        msg.add(1);
        Serial.println(direccionOSC + " 1 (PISADO)");
      } 
      else {
        // Alguien SOLTÓ el botón: mandamos un 0
        msg.add(0);
        Serial.println(direccionOSC + " 0 (SOLTADO)");
      }
      
      // Enviamos el paquete de datos UDP por el aire directamente a la PC
      udp.beginPacket(ipPC, puertoTD);
      msg.send(udp);
      udp.endPacket();
      msg.empty(); // Liberamos la memoria del mensaje
      
      estadosAnteriores[i] = estadoLectura; // Actualizar el registro
    }

    // Contabilizar para el motor y luces si sigue presionado
    if (estadoLectura == LOW) {
      contadorActivos++;
    }
  }

  cantidadPersonas = contadorActivos;
}

void actualizarEstados() {
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