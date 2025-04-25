import os
import time
import firebase_admin
from firebase_admin import credentials, db

cred_path = '/home/team1/Downloads/hitheshj.json'
PIPE_PATH = "/tmp/mqtt_kitchen"
GAS_THRESHOLD = 150
GAS_DANGER_THRESHOLD = 60  # Threshold for critical alert

def get_mac(interface="wlan0"):
    try:
        with open(f'/sys/class/net/{interface}/address') as f:
            return f.read().strip().lower()
    except FileNotFoundError:
        return None

mac_address = get_mac()
if not mac_address:
    print("? Failed to fetch MAC address. Check network interface name.")
    exit(1)

firebase_path_base = f"/RaspberryPi/{mac_address}/rooms/Kitchen/devices"
print(f"? Using Firebase Path: {firebase_path_base}")

# Initialize Firebase
try:
    cred = credentials.Certificate(cred_path)
    if not firebase_admin._apps:
        firebase_admin.initialize_app(cred, {
            'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
        })
    print("? Connected to Firebase!")
except Exception as e:
    print(f"? Failed to initialize Firebase: {e}")
    exit(1)

if not os.path.exists(PIPE_PATH):
    os.mkfifo(PIPE_PATH)
    print(f"? Created named pipe at {PIPE_PATH}")
else:
    print(f"? Using existing named pipe at {PIPE_PATH}")

def read_from_pipe():
    print("? Listening for sensor data via pipe...")
    while True:
        try:
            with open(PIPE_PATH, "r") as pipe:
                data = pipe.readline().strip()
                if data:
                    print(f"? Raw Data: {data}")
                   
                    parts = data.split()
                    gas_part = next((p for p in parts if 'gas' in p), None)
                    flame_part = next((p for p in parts if 'fire' in p), None)
                   
                    # Default values
                    gas_value = 0
                    is_flame_on = False
                   
                    # --- GAS ---
                    if gas_part:
                        gas_value = int(gas_part.split(':')[1])
                        gas_ref = db.reference(f"{firebase_path_base}/Gas/percentage")
                        gas_ref.set(gas_value)
                        print(f"? Updated Gas Percentage: {gas_value}")
                   
                    # --- FLAME ---
                    if flame_part:
                        flame_value = flame_part.split(':')[1]
                        is_flame_on = True if flame_value == "1" else False
                        flame_ref = db.reference(f"{firebase_path_base}/Flame/isOn")
                        flame_ref.set(is_flame_on)
                        print(f"? Updated Flame isOn: {is_flame_on}")
                   
                    # --- DEVICE CONTROLS ---
                    fan_ref = db.reference(f"{firebase_path_base}/kitchen_fan_1/isOn")
                    buzzer_ref = db.reference(f"{firebase_path_base}/kitchen_buzzer_1/isOn")
                   
                    # Initialize states
                    buzzer_state = False
                    fan_state = False
                   
                    # CRITICAL CONDITION: if gas > 60 OR flame is detected, turn ON both fan and buzzer
                    if gas_value > GAS_DANGER_THRESHOLD or is_flame_on:
                        buzzer_state = True
                        fan_state = True
                        if gas_value > GAS_DANGER_THRESHOLD and is_flame_on:
                            print("? CRITICAL ALERT: Gas level above 60% AND flame detected! Activating all safety devices.")
                        elif gas_value > GAS_DANGER_THRESHOLD:
                            print("? ALERT: Gas level above 60%! Activating all safety devices.")
                        else:
                            print("? ALERT: Flame detected! Activating all safety devices.")
                   
                    # Update device states in Firebase
                    buzzer_ref.set(buzzer_state)
                    fan_ref.set(fan_state)
                    print(f"? Buzzer set to: {buzzer_state}")
                    print(f"? Fan set to: {fan_state}")
                   
        except Exception as e:
            print(f"?? Error: {e}")
        time.sleep(1)

print("? Realtime IoT Monitoring Running. Ctrl+C to stop.")
try:
    read_from_pipe()
except KeyboardInterrupt:
    print("\n? Shutdown requested. Exiting.")
