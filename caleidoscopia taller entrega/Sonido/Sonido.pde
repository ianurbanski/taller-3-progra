import processing.sound.*;
import processing.serial.*;

SoundFile[] capas = new SoundFile[4];
Serial puerto;

float[] volumenActual = new float[4];
float[] volumenObjetivo = new float[4];
float velocidadFade = 0.02; // Sigue controlando la suavidad de las capas 1, 2 y 3

int personas = 0;

int tiempoInicioEstado3 = 0;
boolean temporizadorActivo = false;
boolean efectoActivado = false;
boolean apagadoGeneralEfectuado = false;
boolean senalResetEnviada = false; 
boolean esperandoLiberacion = true; 

void setup() {
  size(300, 300);

  capas[0] = new SoundFile(this, "Audio 0.mp3");
  capas[1] = new SoundFile(this, "Audio 1.mp3");
  capas[2] = new SoundFile(this, "Audio 2.mp3");
  capas[3] = new SoundFile(this, "Audio 3.mp3");

  // --- CAPA 0 FIXED: Encendida al máximo desde el inicio ---
  capas[0].loop();
  volumenActual[0] = 3.0;
  volumenObjetivo[0] = 3.0;
  capas[0].amp(volumenActual[0]);

  // Inicialización normal para las capas interactivas (1 a 3)
  for (int i = 1; i < 4; i++) {
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

  if (!apagadoGeneralEfectuado) {
    if (esperandoLiberacion) {
      fill(255, 255, 100);
      textSize(22);
      text("POR FAVOR, BAJARSE\nDE LA PLATAFORMA", 40, 100);
    } else {
      textSize(30);
      text("Personas: " + personas, 80, 100);

      if (temporizadorActivo && !efectoActivado) {
        if (millis() - tiempoInicioEstado3 >= 5000) {
          subirVolumen();
          efectoActivado = true;
        }
      }
    }
  } else {
    fill(255, 100, 100);
    text("FADE OUT GENERAL", 30, 100);
  }

  // --- MODIFICACIÓN DEL BUCLE DE AUDIO ---
  // El Audio 0 se mantiene fijo en su volumen. 
  // Solo aplicamos interpolaciones a partir de la capa 1 (i = 1) en adelante.
  capas[0].amp(2.0); 

  for (int i = 1; i < 4; i++) {
    volumenActual[i] += (volumenObjetivo[i] - volumenActual[i]) * velocidadFade;
    capas[i].amp(volumenActual[i]);
  }
  
  // Sincronización: Ahora controlamos el Fade Out chequeando que se apague la Capa 1 (la primera interactiva)
  if (apagadoGeneralEfectuado && !senalResetEnviada) {
    if (volumenActual[1] < 0.01) { 
      puerto.write('R');           
      senalResetEnviada = true;
      println("Capas interactivas en cero. Avisando a Arduino para iniciar cuenta de reinicio...");
    }
  }
  
  println(nf(2.0, 1, 2) + " | " + nf(volumenActual[1], 1, 2) + " | " + nf(volumenActual[2], 1, 2) + " | " + nf(volumenActual[3], 1, 2));
}

void serialEvent(Serial puerto) {
  String linea = puerto.readStringUntil('\n');
  if (linea == null) return;
  linea = trim(linea);

  if (linea.length() == 0 || linea.startsWith("-") || linea.startsWith("!")) {
    return; 
  }

  try {
    int nuevoEstado = int(linea);

    if (nuevoEstado == 0) {
      esperandoLiberacion = false;
    }

    if (nuevoEstado == 4) {
      println("Señal de Arduino: Apagado General por Tiempo Límite.");
      apagadoGeneralEfectuado = true;
      cambiarEstado(4);
    } 
    else if (nuevoEstado == 5) {
      println("Señal de Arduino: Reinicio del sistema listo. Bloqueo activado.");
      apagadoGeneralEfectuado = false; 
      senalResetEnviada = false; 
      esperandoLiberacion = true; 
      personas = 0;
      
      // Ya no hace falta forzar volumenActual[0] acá, porque nunca se apagó.
      cambiarEstado(0);
    }
    else if (nuevoEstado >= 0 && nuevoEstado <= 3 && !apagadoGeneralEfectuado && !esperandoLiberacion) {
      if (nuevoEstado != personas) {
        personas = nuevoEstado;
        println("Estado cambiado -> " + personas);
        cambiarEstado(personas);
      }
    }
  }
  catch(Exception e) {
    println("Dato de ruido ignorado de forma segura: " + linea);
  }
}

void subirVolumen() {
  // Solo amplificamos las capas interactivas que correspondan al estado máximo
  for (int i = 1; i < 4; i++) {
    volumenObjetivo[i] = 1.5;
  }
}
