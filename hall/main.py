import os
import subprocess
import time
import firebase_admin
from firebase_admin import credentials, db
import psutil

# === Firebase setup ===
cred = credentials.Certificate("/home/team1/Downloads/hitheshj.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
})

firebase_path = '/RaspberryPi/2c:cf:67:55:59:e3/mode'
current_state = None

def get_state():
    try:
        ref = db.reference(firebase_path)
        return ref.get()
    except Exception as e:
        print(f"[ERROR] Reading Firebase failed: {e}")
        return None

def kill_all():
    print("[ACTION] Killing mjpg_streamer, ngrok, motion_logger, and stream_toggle.py...")
    os.system("pkill -f mjpg_streamer")
    os.system("pkill -f ngrok")
    os.system("pkill -f motion_logger")
    os.system("pkill -f stream_toggle.py")

def load_kernel_module():
    try:
        subprocess.run(['sudo', 'insmod', '/home/team1/home_automation/hall/soil_sensor.ko'], check=True)
        print("[INFO] Kernel module loaded.")
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Kernel module load failed: {e}")

def run_kernal_py():
    try:
        subprocess.Popen(['python3', '/home/team1/home_automation/hall/kernal.py'])
        print("[INFO] Started kernal.py")
    except Exception as e:
        print(f"[ERROR] Could not start kernal.py: {e}")

def is_stream_toggle_running():
    for proc in psutil.process_iter(['cmdline']):
        try:
            if proc.info['cmdline'] and 'stream_toggle.py' in ' '.join(proc.info['cmdline']):
                return True
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue
    return False

def start_stream_toggle():
    if not is_stream_toggle_running():
        print("[ACTION] Starting stream_toggle.py...")
        subprocess.Popen(['python3', '/home/team1/home_automation/hall/stream_toggle.py'])
    else:
        print("[INFO] stream_toggle.py is already running.")

def state_listener(event):
    global current_state
    new_state = str(event.data).strip().lower()
    print(f"[FIREBASE] State changed: {new_state}")
    current_state = new_state

    if new_state == "home":
        kill_all()
    elif new_state == "vacation":
        start_stream_toggle()
    else:
        print(f"[INFO] Unknown state '{new_state}' received. No action taken.")

def main():
    global current_state

    print("[BOOT] Initializing...")
    load_kernel_module()
    run_kernal_py()

    current_state = get_state()
    print(f"[INIT] Current state: {current_state}")

    if current_state == "home":
        kill_all()
    elif current_state == "vacation":
        start_stream_toggle()

    print(f"[LISTENING] Firebase path: {firebase_path}")
    db.reference(firebase_path).listen(state_listener)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("[EXIT] Shutting down...")
        kill_all()

if __name__ == '__main__':
    main()
