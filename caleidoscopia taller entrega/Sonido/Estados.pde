void cambiarEstado(int estado){
  
  efectoActivado = false; 

  switch(estado){
    case 0:
      temporizadorActivo = false;
      volumenObjetivo[1] = 0.0;
      volumenObjetivo[2] = 0.0;
      volumenObjetivo[3] = 0.0;
      break;

    case 1:
      temporizadorActivo = false;
      volumenObjetivo[1] = 2.0;
      volumenObjetivo[2] = 0.0;
      volumenObjetivo[3] = 0.0;
      break;

    case 2:
      temporizadorActivo = false;
      volumenObjetivo[1] = 2.0;
      volumenObjetivo[2] = 1.0;
      volumenObjetivo[3] = 0.0;
      break;

    case 3:
      tiempoInicioEstado3 = millis();
      temporizadorActivo = true;
      
      volumenObjetivo[1] = 2.0;
      volumenObjetivo[2] = 1.0;
      volumenObjetivo[3] = 2.0;
      break;
      
    case 4: // Al apagarse el sistema, solo las capas interactivas (1, 2 y 3) van a cero
      temporizadorActivo = false;
      volumenObjetivo[1] = 0.0;
      volumenObjetivo[2] = 0.0;
      volumenObjetivo[3] = 0.0;
      break;
  }
}
