import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import time

# Path to your service account key file
cred = credentials.Certificate('/home/team1/Downloads/home-automation-66846-firebase-adminsdk-fbsvc-f3da9c5a9a.json')

# Your Firebase database URL
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# Reference to the 'led' node
led_ref = db.reference('led')

# Function to log database changes
def listener(event):
    print(f"Data changed: {event.data}")
    print(f"Timestamp: {time.strftime('%H:%M:%S')}")

# Set up the listener
led_ref.listen(listener)

# Read initial value
initial_value = led_ref.get()
print(f"Initial LED state: {initial_value}")

print("Connection established! Listening for changes...")
print("Press Ctrl+C to exit")

try:
    # Keep the script running
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Program terminated")
