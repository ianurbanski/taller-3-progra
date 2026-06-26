#include <Adafruit_NeoPixel.h>

const int PIN_ANILLO1 = 16;  // Pin físico D5 (GPIO 16)
const int PIN_ANILLO2 = 27;  // Pin físico D6 (GPIO 27)
const int LEDS_POR_ANILLO = 24;

Adafruit_NeoPixel anillo1(LEDS_POR_ANILLO, PIN_ANILLO1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel anillo2(LEDS_POR_ANILLO, PIN_ANILLO2, NEO_GRB + NEO_KHZ800);

const int PIN_MOTOR_ENA = 26;  // Pin físico D2
const int PIN_MOTOR_IN1 = 25;  // Pin físico D3
const int PIN_MOTOR_IN2 = 17;  // Pin físico D4

const int frecuenciaPWM = 30000;
const int resolucionPWM = 8;

//Pines físicos D7, D8 y D11 (GPIOs 14, 12 y 13 de la Wemos D1 R32)
const int NUM_BOTONES = 3;
const int pinesBotones[NUM_BOTONES] = { 14, 12, 13 };


float cantidadPersonas = 0;
float brilloActual = 15;
float brilloObjetivo = 15;

float velocidadMotorActual = 60;
float velocidadMotorObjetivo = 60;

float suavizado = 0.05;

unsigned long ultimoTiempoSerial = 0;
const unsigned long intervaloSerial = 500;


const float BR_REPOSO = 1.0;
const float BR_MAXIMO = 140.0;
const float VEL_MIN_MOTOR = 60.0;
const float VEL_MAX_MOTOR = 110.0;

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(115200);
  delay(500);


  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  ledcAttach(PIN_MOTOR_ENA, frecuenciaPWM, resolucionPWM);


  for (int i = 0; i < NUM_BOTONES; i++) {
    pinMode(pinesBotones[i], INPUT_PULLUP);
  }


  anillo1.begin();
  anillo2.begin();
  anillo1.show();
  anillo2.show();

  Serial.println("--- SISTEMA INTERACTIVO CON 3 BOTONES LISTO ---");
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
  brilloObjetivo = mapFloat(cantidadPersonas, 0.0, 3.0, BR_REPOSO, BR_MAXIMO);
  velocidadMotorObjetivo = mapFloat(cantidadPersonas, 0.0, 3.0, VEL_MIN_MOTOR, VEL_MAX_MOTOR);

  if (cantidadPersonas == 0) {
    brilloObjetivo = BR_REPOSO;
  } else {
    brilloObjetivo = mapFloat(cantidadPersonas, 1.0, 3.0, BR_REPOSO, BR_MAXIMO);
  }
}

void controlarMotor() {
  velocidadMotorActual += (velocidadMotorObjetivo - velocidadMotorActual) * suavizado;

  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);

  ledcWrite(PIN_MOTOR_ENA, (int)velocidadMotorActual);
}

void controlarLuces() {
  // 1. Calculamos el desvanecimiento (suavizado) normal del brillo
  brilloActual += (brilloObjetivo - brilloActual) * suavizado;

  // 2. Creamos una variable base para el brillo que usaremos al final
  float brilloFinal = brilloActual;

  // 3. Si estamos en el tercer estado, modificamos 'brilloFinal' con el latido
  if ((int)cantidadPersonas == 3) {
    float velocidadLatido = 0.004;
    float factorOscilacion = (sin(millis() * velocidadLatido) + 1.0) / 2.0;
    float brilloMinLatido = 40.0;

    // Ahora sí, modificamos la variable que declaramos arriba
    brilloFinal = brilloMinLatido + (factorOscilacion * (BR_MAXIMO - brilloMinLatido));
  }

  // 4. Limitamos el brillo final (sea el normal o el latido) para no pasarnos de los rangos
  float brilloLimitado = constrain(brilloFinal, BR_REPOSO, BR_MAXIMO);

  // 5. Aplicamos el brillo limitado a los LEDs
  for (int i = 0; i < LEDS_POR_ANILLO; i++) {
    int r = (255 * brilloLimitado) / BR_MAXIMO;
    int g = (197 * brilloLimitado) / BR_MAXIMO;
    int b = (110 * brilloLimitado) / BR_MAXIMO;

    anillo1.setPixelColor(i, anillo1.Color(r, g, b));
    anillo2.setPixelColor(i, anillo2.Color(r, g, b));
  }

  // 6. ¡No olvides mostrar los cambios! (Asumo que los tenías justo abajo)
  anillo1.show();
  anillo2.show();
}

void imprimirEstadoSerial() {
  unsigned long tiempoActual = millis();

  if (tiempoActual - ultimoTiempoSerial >= intervaloSerial) {
    ultimoTiempoSerial = tiempoActual;

    Serial.println((int)cantidadPersonas);

    /*Serial.print("Personas en las placas: ");
    Serial.print((int)cantidadPersonas);

    Serial.print("  |  Velocidad Motor: ");
    Serial.print((int)velocidadMotorActual);

    Serial.print("  |  Brillo Anillos: ");
    Serial.println((int)brilloActual); */
  }
}