import processing.sound.*;
import processing.serial.*;

SoundFile[] capas = new SoundFile[4];

Serial puerto;

float[] volumenActual = new float[4];
float[] volumenObjetivo = new float[4];
float velocidadFade = 0.02;

int personas = 0;

int tiempoInicioEstado3 = 0;
boolean temporizadorActivo = false;
boolean efectoActivado = false;

void setup() {

  size(300, 300);

  capas[0] = new SoundFile(this, "Audio 0.mp3");
  capas[1] = new SoundFile(this, "Audio 1.mp3");
  capas[2] = new SoundFile(this, "Audio 2.mp3");
  capas[3] = new SoundFile(this, "Audio 3.mp3");

  for (int i=0; i<4; i++) {

    capas[i].loop();

    volumenActual[i] = 0;
    volumenObjetivo[i] = 0;

    capas[i].amp(0);
  }

  cambiarEstado(0);

  println(Serial.list());

  puerto = new Serial(this, "COM7", 115200);

  delay(2000);

  puerto.bufferUntil('\n');
}

void draw() {

  background(30);

  textSize(30);
  fill(255);

  text("Personas: " + personas, 80, 100);

  if (temporizadorActivo && !efectoActivado) {
    if (millis() - tiempoInicioEstado3 >= 5000) {
      subirVolumen();
      efectoActivado = true;
    }
  }

  for (int i=0; i<4; i++) {

    volumenActual[i] += (volumenObjetivo[i] - volumenActual[i]) * velocidadFade;

    capas[i].amp(volumenActual[i]);

    println(nf(volumenActual[0], 1, 2) + " | " + nf(volumenActual[1], 1, 2) + " | " + nf(volumenActual[2], 1, 2) + " | " + nf(volumenActual[3], 1, 2));
  }
}

/*void keyPressed() {
 
 if (key >= '0' && key <= '3') {
 
 personas = key - '0';
 
 cambiarEstado(personas);
 }
 }*/

void serialEvent(Serial puerto) {

  String linea = puerto.readStringUntil('\n');

  if (linea == null) return;

  linea = trim(linea);

  try {

    int nuevoEstado = int(linea);

    if (nuevoEstado >= 0 && nuevoEstado <= 3) {

      if (nuevoEstado != personas) {

        personas = nuevoEstado;

        println("Estado cambiado -> " + personas);

        cambiarEstado(personas);
      }
    }
  }
  catch(Exception e) {

    println("Dato ignorado: " + linea);
  }
}

void subirVolumen() {

  for (int i = 0; i < 4; i++) {
    volumenObjetivo[i] = 1.5;
  }
}
