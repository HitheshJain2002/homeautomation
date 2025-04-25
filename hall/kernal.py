#!/usr/bin/env python3
import subprocess
import re
import time
import datetime
import threading
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import lgpio

# GPIO setup using lgpio
LED_PIN = 25  # BCM GPIO25
h = lgpio.gpiochip_open(0)
lgpio.gpio_claim_output(h, LED_PIN, 0)  # Initially OFF

# Firebase setup
cred = credentials.Certificate("/home/team1/Downloads/hitheshj.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
})

# Firebase paths
soil_path = "/RaspberryPi/2c:cf:67:55:59:e3/rooms/Hall Room/devices/Soil/moisture"
led_path = "/RaspberryPi/2c:cf:67:55:59:e3/rooms/Hall Room/devices/LED/isOn"

soil_ref = db.reference(soil_path)
led_ref = db.reference(led_path)

# Regex for soil sensor logs
pattern = re.compile(r'\[\s*\d+\.\d+\]\s*\[SoilSensor\]\s*Status:\s*(Dry|Wet)')


def monitor_soil_sensor():
    print("Monitoring soil sensor from /dev/soil_device...")

    last_status = None
    while True:
        try:
            with open("/dev/soil_device", "r") as f:
                status = f.read().strip()
                if status and status != last_status:
                    soil_ref.set(status)
                    current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    print(f"[{current_time}] Soil status: {status} -> Firebase updated")
                    last_status = status
        except Exception as e:
            print(f"[ERROR] Reading from /dev/soil_device failed: {e}")
        
        time.sleep(2)  # match kernel polling interval


def listen_for_led_command():
    print("Listening for LED commands from Firebase...")

    def callback(event):
        value = event.data
        print(f"Firebase LED isOn changed to: {value}")

        # Normalize input: support True/False, "true"/"false", 1/0
        if str(value).lower() in ["1", "true", "on"]:
            lgpio.gpio_write(h, LED_PIN, 1)
            print("LED turned ON")
        else:
            lgpio.gpio_write(h, LED_PIN, 0)
            print("LED turned OFF")

    led_ref.listen(callback)


if __name__ == "__main__":
    try:
        # Start soil sensor thread
        soil_thread = threading.Thread(target=monitor_soil_sensor, daemon=True)
        soil_thread.start()

        # Start LED listener (blocking)
        listen_for_led_command()

    except KeyboardInterrupt:
        print("\nExiting. Cleaning up GPIO...")
        lgpio.gpiochip_close(h)
