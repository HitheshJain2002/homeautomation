import subprocess
import time
import firebase_admin
from firebase_admin import credentials, db

# --- Firebase Setup ---
cred_path = "/home/team1/Downloads/home-automation-66846-firebase-adminsdk-fbsvc-f3da9c5a9a.json"
firebase_url = "https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/"

cred = credentials.Certificate(cred_path)

if not firebase_admin._apps:
    firebase_admin.initialize_app(cred, {
        'databaseURL': firebase_url
    })
    print("? Connected to Firebase")

# Create ref (will auto-create if doesn't exist)
ref = db.reference("tiltSensor")

# --- Function to read from C code ---
def read_tilt():
    try:
        output = subprocess.check_output(["./tilt_sensor"]).decode().strip()
        return int(output)
    except Exception as e:
        print("? Error reading tilt:", e)
        return None

# --- Firebase update ---
def update_firebase(tilt_value):
    status = "TILTED" if tilt_value == 0 else "STABLE"
    data = {
        "tilt": tilt_value,
        "status": status,
        "timestamp": int(time.time() * 1000)
    }
    ref.set(data)
    print(f"? Firebase Updated: {data}")

# --- Main loop ---
def main():
    print("? Starting tilt sensor monitoring...\n")
    last_value = None
    while True:
        tilt = read_tilt()
        if tilt is not None and tilt != last_value:
            update_firebase(tilt)
            last_value = tilt
        else:
            print(f"? No change: {tilt}")
        time.sleep(1)

if __name__ == "__main__":
    main()
