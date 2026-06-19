import vlc
import time

player = vlc.MediaPlayer("Audio 0.mp3")

player.play()

print("Reproduciendo...")

time.sleep(10)

player.stop()