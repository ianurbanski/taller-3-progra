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
VOLUMEN_INACTIVO = 1

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


# ==========================================
# AUDIO
# ==========================================

players = []

print("Inicializando capas de audio...")

for archivo in ARCHIVOS_AUDIO:

    ruta_absoluta = os.path.abspath(archivo)

    if not os.path.exists(ruta_absoluta):
        print(f"ERROR: No existe {ruta_absoluta}")
        continue

    player = vlc.MediaPlayer(ruta_absoluta)

    player.play()

    # Dar tiempo a VLC para abrir el archivo
    time.sleep(0.2)

    player.audio_set_volume(VOLUMEN_INACTIVO)

    players.append(player)

print("Armando sincronía...")
time.sleep(1.5)

print("\nEstado inicial de reproductores:")

for i, player in enumerate(players):
    print(
        f"{ARCHIVOS_AUDIO[i]} | "
        f"Estado={player.get_state()} | "
        f"Volumen={player.audio_get_volume()}"
    )


def setear_volumen(player, volumen):
    """
    VLC a veces ignora cambios rápidos.
    Forzamos el volumen dos veces.
    """

    player.audio_set_volume(volumen)
    time.sleep(0.02)
    player.audio_set_volume(volumen)


def actualizar_capas(cantidad_personas):

    print("\n================================")
    print(f"PERSONAS DETECTADAS: {cantidad_personas}")
    print("================================")

    for i, player in enumerate(players):

        if i <= cantidad_personas:
            setear_volumen(player, VOLUMEN_ACTIVO)
            estado = "ACTIVO"

        else:
            setear_volumen(player, VOLUMEN_INACTIVO)
            estado = "INACTIVO"

        print(
            f"{ARCHIVOS_AUDIO[i]} | "
            f"{estado} | "
            f"Volumen real={player.audio_get_volume()} | "
            f"Estado VLC={player.get_state()}"
        )


# Estado inicial
actualizar_capas(0)

print("\nCapas listas y sincronizadas.")

# ==========================================
# SERIAL
# ==========================================

puerto = encontrar_arduino()

if puerto is None:
    print("No se encontró Arduino.")
    quit()

ser = serial.Serial(
    puerto,
    115200,
    timeout=1
)

print("Conectado por puerto serie.\n")

# ==========================================
# LOOP PRINCIPAL
# ==========================================

placas_activas = {}

estado_actual = 0

while True:

    try:

        linea = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        if not linea:
            continue

        print(f"[Serial] {linea}")

        match = re.search(
            r"/boton(\d+)\s+(\d)",
            linea
        )

        if match:

            id_boton = int(match.group(1))
            estado_boton = int(match.group(2))

            placas_activas[id_boton] = (estado_boton == 1)

            total_personas = sum(
                1
                for activa in placas_activas.values()
                if activa
            )

            total_personas = min(total_personas, 3)

            if total_personas != estado_actual:

                print(
                    f"\nCambio de estado: "
                    f"{estado_actual} -> {total_personas}"
                )

                estado_actual = total_personas

                actualizar_capas(
                    total_personas
                )

    except KeyboardInterrupt:

        print("Programa finalizado.")

        try:
            ser.close()
        except:
            pass

        break

    except Exception as e:

        print(f"Error: {e}")