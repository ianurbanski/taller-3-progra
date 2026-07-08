#include <Adafruit_NeoPixel.h>

const int PIN_ANILLO1 = 16;  
const int PIN_ANILLO2 = 27;  
const int LEDS_POR_ANILLO = 24;

Adafruit_NeoPixel anillo1(LEDS_POR_ANILLO, PIN_ANILLO1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel anillo2(LEDS_POR_ANILLO, PIN_ANILLO2, NEO_GRB + NEO_KHZ800);

const int PIN_MOTOR_IN1 = 25;  
const int PIN_MOTOR_ENA = 26;  

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

// --- VARIABLES PARA EL APAGADO Y REINICIO ---
unsigned long tiempoInicioEstado3 = 0;
bool temporizadorActivo = false;
bool sistemaApagado = false;
float factorFadeOut = 1.0; 

unsigned long tiempoFinFadeOut = 0;   
bool esperandoReiniciar = false;      
bool audioTerminoFade = false; 

unsigned long tiempoInicioPatada = 0;
bool patadaActiva = false;
const unsigned long duracionPatada = 150; 

// --- NUEVA VARIABLE DE SEGURIDAD ---
bool bloqueoInicial = true; // Arranca en true para obligar a que limpien los botones al inicio

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  
  digitalWrite(PIN_MOTOR_IN1, HIGH);

  for (int i = 0; i < NUM_BOTONES; i++) {
    pinMode(pinesBotones[i], INPUT_PULLUP);
  }

  anillo1.begin();
  anillo2.begin();
  anillo1.show();
  anillo2.show();
}

void loop() {
  // ESCUCHAR REPORTE DE FADE OUT DESDE PROCESSING
  if (Serial.available() > 0) {
    char lector = Serial.read();
    if (lector == 'R') {
      audioTerminoFade = true; 
      if (sistemaApagado && !esperandoReiniciar) {
        tiempoFinFadeOut = millis(); 
        esperandoReiniciar = true;
      }
    }
  }

  // Leer los sensores físicos siempre para saber el estado real de la instalación
  leerSensoresReales();

  // CONTROL DE COMPROBACIÓN DEL BLOQUEO
  if (bloqueoInicial) {
    if ((int)cantidadPersonas == 0) {
      bloqueoInicial = false; // Se liberaron los botones, el sistema puede operar
      Serial.println("0");    // Le avisamos a Processing que limpie cualquier estado visual residual
    }
  }

  if (!sistemaApagado) {
    // Si el bloqueo está activo, forzamos al sistema a quedarse en el molde (estado 0)
    if (bloqueoInicial) {
      brilloObjetivo = BR_REPOSO;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      temporizadorActivo = false;
    } else {
      // Funcionamiento interactivo normal
      actualizarEstados();
      
      if ((int)cantidadPersonas == 3) {
        if (!temporizadorActivo) {
          tiempoInicioEstado3 = millis();
          temporizadorActivo = true;
        } else if (millis() - tiempoInicioEstado3 >= 20000) { 
          sistemaApagado = true;
          Serial.println("4"); 
        }
      } else {
        temporizadorActivo = false;
      }
    }
  } else {
    // --- LÓGICA DE FADE OUT FÍSICO ---
    if (factorFadeOut > 0.0) {
      factorFadeOut -= 0.01; 
      if (factorFadeOut < 0.0) factorFadeOut = 0.0;
    }

    // --- LÓGICA DE ESPERA Y REINICIO ---
    if (esperandoReiniciar && audioTerminoFade) {
      if ((int)cantidadPersonas < 3) {
        if (millis() - tiempoFinFadeOut >= 5000) {
          reiniciarSistemaCompleto();
        }
      } else {
        tiempoFinFadeOut = millis();
      }
    }
  }

  if (patadaActiva) {
    if (millis() - tiempoInicioPatada >= duracionPatada) {
      patadaActiva = false; 
    }
  }

  controlarMotor();
  controlarLuces();
  
  if (!sistemaApagado && !bloqueoInicial) {
    imprimirEstadoSerial();
  }

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

  switch (estadoActual) {
    case 0:
      brilloObjetivo = BR_REPOSO;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      patadaActiva = false; 
      break;
    case 1:
      brilloObjetivo = BR_ESTADO1;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      patadaActiva = false;
      break;
    case 2:
      brilloObjetivo = BR_ESTADO2;
      velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
      patadaActiva = false;
      break;
    case 3:
      brilloObjetivo = BR_MAXIMO;
      velocidadMotorObjetivo = VEL_MAX_MOTOR;

      if (estadoAnterior < 3) {
        tiempoInicioPatada = millis();
        patadaActiva = true;
      }
      break;
  }
  estadoAnterior = estadoActual;
}

void reiniciarSistemaCompleto() {
  sistemaApagado = false;
  esperandoReiniciar = false;
  audioTerminoFade = false;
  temporizadorActivo = false;
  patadaActiva = false;
  factorFadeOut = 1.0;
  
  brilloActual = BR_REPOSO;
  brilloObjetivo = BR_REPOSO;
  velocidadMotorObjetivo = VEL_REPOSO_MOTOR;
  estadoAnterior = 0;
  
  bloqueoInicial = true; // Volvemos a bloquear hasta que se bajen por completo de la plataforma
  
  Serial.println("5"); 
}

void controlarMotor() {
  int velocidadBase;
  if (patadaActiva && !sistemaApagado) {
    velocidadBase = 255;
  } else {
    velocidadBase = velocidadMotorObjetivo;
  }
  int velocidadFinal = velocidadBase * factorFadeOut;
  analogWrite(PIN_MOTOR_ENA, velocidadFinal);
}

void controlarLuces() {
  brilloActual += (brilloObjetivo - brilloActual) * suavizado;
  float brilloFinal = brilloActual;

  if ((int)cantidadPersonas == 3 && !sistemaApagado && !bloqueoInicial) {
    float velocidadLatido = 0.004;
    float factorOscilacion = (sin(millis() * velocidadLatido) + 1.0) / 2.0;
    float brilloMinLatido = 50.0;
    brilloFinal = brilloMinLatido + (factorOscilacion * (BR_MAXIMO - brilloMinLatido));
  }

  float brilloLimitado = constrain(brilloFinal * factorFadeOut, 0.0, BR_MAXIMO);

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