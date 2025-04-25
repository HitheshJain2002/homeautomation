import time
import firebase_admin
from firebase_admin import credentials, db
import RPi.GPIO as GPIO

# Use GPIO pin 17 (BCM)
LED_PIN = 17

# Set up GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(LED_PIN, GPIO.OUT)

# Firebase setup
cred = credentials.Certificate("/home/team1/Downloads/home-automation-66846-firebase-adminsdk-fbsvc-f3da9c5a9a.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# Function to control LED
def control_led(value):
    if value:
        print("LED ON")
        GPIO.output(LED_PIN, GPIO.HIGH)
    else:
        print("LED OFF")
        GPIO.output(LED_PIN, GPIO.LOW)

# Firebase event listener
def listener(event):
    print(f"Data changed: {event.data}")
    control_led(event.data)

# Set up listener
led_ref = db.reference('led')
led_ref.listen(listener)

# Initial state
initial_state = led_ref.get()
print(f"Initial LED state: {initial_state}")
control_led(initial_state)

print("LED control app running. Press Ctrl+C to exit.")
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    GPIO.cleanup()
    print("Program terminated.")
