import serial
import serial.tools.list_ports
import vlc
import re
import time
import os

ARCHIVOS_AUDIO = [
    "Audio 0.mp3",
    "Audio 1.mp3",
    "Audio 2.mp3",
    "Audio 3.mp3"
]

VOLUMEN_ACTIVO = 100
VOLUMEN_INACTIVO = 0

def encontrar_arduino():
    puertos = serial.tools.list_ports.comports()

    for puerto in puertos:
        descripcion = puerto.description.lower()

        if (
            "usb" in descripcion
            or "serial" in descripcion
            or "ch340" in descripcion
            or "cp210" in descripcion
            or "arduino" in descripcion
            or "esp32" in descripcion
        ):
            print(f"Arduino encontrado en {puerto.device}")
            return puerto.device

    return None


players = []

print("Inicializando capas de audio...")
for archivo in ARCHIVOS_AUDIO:
    # 1. Forzamos la ruta absoluta para que Python y VLC encuentren el archivo sí o sí
    ruta_absoluta = os.path.abspath(archivo)
    
    if not os.path.exists(ruta_absoluta):
        print(f"ERROR CRÍTICO: El archivo no existe en la ruta: {ruta_absolute}")
        print("Asegurate de que el nombre sea exacto (ojo con .mp3.mp3 ocultos)")
        continue

    # 2. Creamos el reproductor con la ruta fija
    media = vlc.MediaPlayer(ruta_absoluta)
    media.play()
    
    # 3. Le damos un momento y verificamos si VLC realmente lo está reproduciendo
    time.sleep(0.3)
    estado = media.get_state()
    
    # VLC State 3 significa "Playing" (Reproduciendo)
    # VLC State 5/6 significa "Error" o "Ended"
    if estado.value != 3:
        print(f"VLC no pudo reproducir '{archivo}'. Código de estado: {estado}")
    else:
        print(f"{archivo} cargado y reproduciéndose correctamente.")
        
    players.append(media)

time.sleep(1.0) 

def inicializar_volumenes_limpios():
    for i, player in enumerate(players):
        if i == 0:
            player.audio_set_volume(VOLUMEN_ACTIVO)
        else:
            player.audio_set_volume(VOLUMEN_INACTIVO)

if players:
    inicializar_volumenes_limpios()
    print("Capas listas y sincronizadas.")
else:
    print("No se pudo cargar ningún reproductor. Saliendo...")
    quit()
# ==========================
# SERIAL
# ==========================
puerto = encontrar_arduino()

if puerto is None:
    print("No se encontró Arduino.")
    quit()

ser = serial.Serial(
    puerto,
    115200,
    timeout=1
)

print("Conectado.")
print()

estado_actual = -1

# ==========================
# FUNCIÓN DE CAPAS
# ==========================
def actualizar_capas(personas):
    for i, player in enumerate(players):
        if i <= personas:
            player.audio_set_volume(VOLUMEN_ACTIVO)
        else:
            player.audio_set_volume(VOLUMEN_INACTIVO)

# ==========================
# LOOP PRINCIPAL
# ==========================

placas_activas = {}
estado_actual = -1

while True:
    try:
        linea = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        if not linea:
            continue

        print(f"[Serial] {linea}")

        # Buscamos el formato real: "/botonX" seguido de un "1" o "0"
        # Ejemplo: /boton3 1 (PISADO) -> capturamos el '3' y el '1'
        match = re.search(r"/boton(\d+)\s+(\d)", linea)

        if match:
            id_boton = int(match.group(1))
            estado_boton = int(match.group(2)) # 1 si está pisado, 0 si está soltado

            # Actualizamos el diccionario de estados de las placas
            if estado_boton == 1:
                placas_activas[id_boton] = True
            else:
                placas_activas[id_boton] = False

            # Contamos cuántas placas/botones tienen valor True en este momento
            personas = sum(1 for activa in placas_activas.values() if activa)

            # Si el total de personas cambió, actualizamos el audio
            if personas != estado_actual:
                estado_actual = personas
                print(f"\nCambio detectado -> {personas} placa(s) activa(s) en total\n")
                actualizar_capas(personas)

    except KeyboardInterrupt:
        print("Programa finalizado.")
        
        try:
            ser.close()
        except:
            pass
        break

    except Exception as e:
        print(f"Error en loop: {e}")