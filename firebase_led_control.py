import os
import firebase_admin
from firebase_admin import credentials, db

# ? Firebase Admin SDK Initialization
cred_path = '/home/team1/Downloads/home-automation-66846-firebase-adminsdk-fbsvc-f3da9c5a9a.json'
cred = credentials.Certificate(cred_path)

# Initialize Firebase only once
if not firebase_admin._apps:
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })
    print("? Connected to Firebase!")

PIPE_PATH = "/tmp/mqtt_pipe"

# ? Create named pipe if it doesn't exist
if not os.path.exists(PIPE_PATH):
    os.mkfifo(PIPE_PATH)

# ? Function to handle data changes from Firebase
def listener(event):
    if event.data is None:
        print("?? No data received.")
        return

    led_state = "ON" if str(event.data).upper() == "ON" else "OFF"
    print(f"? LED state changed: {led_state}")

    try:
        with open(PIPE_PATH, "w") as pipe:
            pipe.write(led_state)
            print(f"? Sent to C via pipe: {led_state}")
    except Exception as e:
        print(f"? Pipe write error: {e}")

# ? Listen to LED changes without polling
def start_listener():
    print("? Listening for LED state changes...")
    led_ref = db.reference('led')
    led_ref.listen(listener)

# ? Start Firebase listener
start_listener()
