import firebase_admin
from firebase_admin import credentials, db
import lgpio
import time

# LED GPIO pin
LED_PIN = 18  # Change if using a different pin

# Open GPIO chip
h = lgpio.gpiochip_open(0)

# Set GPIO 18 as output
lgpio.gpio_claim_output(h, LED_PIN)

# Path to your Firebase service account key
cred = credentials.Certificate('/home/team1/Downloads/home-automation-66846-firebase-adminsdk-fbsvc-f3da9c5a9a.json')

# Initialize Firebase app
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# Function to control LED
def control_led(value):
    if value:
        print("LED ON")
        lgpio.gpio_write(h, LED_PIN, 1)  # Turn LED ON
    else:
        print("LED OFF")
        lgpio.gpio_write(h, LED_PIN, 0)  # Turn LED OFF

# Listen for changes in Firebase
def listener(event):
    print(f"Data changed: {event.data}")
    control_led(event.data)

# Set up the Firebase listener
led_ref = db.reference('led')
led_ref.listen(listener)

# Get the initial LED state from Firebase
initial_state = led_ref.get()
print(f"Initial LED state: {initial_state}")
control_led(initial_state)

print("LED control app running. Press Ctrl+C to exit.")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Program terminated")
    lgpio.gpiochip_close(h)  # Clean up lgpio
