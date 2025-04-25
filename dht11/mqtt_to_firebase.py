# firebase_listener.py
import os
import firebase_admin
from firebase_admin import credentials, db

cred_path = '/home/team1/Downloads/home-automation-66846-firebase-adminsdk-fbsvc-f3da9c5a9a.json'
cred = credentials.Certificate(cred_path)

if not firebase_admin._apps:
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })
    print("? Firebase connected.")

PIPE_PATH = "/tmp/mqtt_pipe"
if not os.path.exists(PIPE_PATH):
    os.mkfifo(PIPE_PATH)

dht11_ref = db.reference("dht11/temperature")

print("? Waiting for temperature data via pipe...")

while True:
    try:
        with open(PIPE_PATH, "r") as pipe:
            for line in pipe:
                temp_value = line.strip()
                if temp_value:
                    print(f"??  Temp received: {temp_value}")
                    dht11_ref.set(temp_value)
                    print("? Firebase updated.")
    except Exception as e:
        print(f"? Pipe read error: {e}")
