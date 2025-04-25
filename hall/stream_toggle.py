import os
import subprocess
import time
import requests
import socket
import fcntl
import struct
import firebase_admin
from firebase_admin import credentials, db
import signal

# ==== Get MAC Address ====
def get_mac_address(interface='wlan0'):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927, struct.pack('256s', interface.encode()[:15]))
        return ':'.join(['%02x' % char for char in info[18:24]])
    except IOError:
        try:
            mac = os.popen(f"ifconfig {interface} | grep -o -E '([0-9a-fA-F]{{2}}:){{5}}[0-9a-fA-F]{{2}}'").read().strip()
            if mac:
                return mac
        except:
            pass
        try:
            with open(f'/sys/class/net/{interface}/address', 'r') as f:
                return f.read().strip()
        except:
            pass
    return None

# ==== Firebase Setup ====
cred = credentials.Certificate("/home/team1/Downloads/hitheshj.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
})

mac_address = get_mac_address('wlan0')
if not mac_address:
    print("[ERROR] Failed to get MAC address")
    exit(1)

room_name = "Hall Room"
device_id = "Camera"  # Use existing node
device_path = f"RaspberryPi/{mac_address}/rooms/{room_name}/devices/{device_id}"

stream_url_ref = db.reference(f"{device_path}/stream_url")
trigger_ref = db.reference(f"{device_path}/stream_toggle")
cloudinary_ref = db.reference(f"{device_path}/cloudinary_url")
snapshot_ref = db.reference(f"{device_path}/snapshot_url")
file_list_ref = db.reference(f"{device_path}/file_list")

mjpg_proc = None
ngrok_proc = None
motion_proc = None
FLAG_PATH = "/tmp/stream_mode.flag"

# ==== Helper Functions ====
def kill_processes():
    os.system("pkill -f mjpg_streamer")
    os.system("pkill -f ngrok")
    os.system("pkill -f motion_logger")

def write_stream_flag():
    with open(FLAG_PATH, "w") as f:
        f.write("streaming")

def remove_stream_flag():
    if os.path.exists(FLAG_PATH):
        os.remove(FLAG_PATH)

def upload_dummy_media():
    # Placeholder logic for uploading
    cloudinary_ref.set("https://res.cloudinary.com/demo/video/upload/sample.mp4")
    snapshot_ref.set("https://res.cloudinary.com/demo/image/upload/sample.jpg")
    file_list_ref.set("sample1.mp4,sample2.jpg")

def start_stream():
    global mjpg_proc, ngrok_proc
    print("[INFO] Starting livestream...")
    kill_processes()
    time.sleep(1)
    write_stream_flag()

    mjpg_cmd = (
        'mjpg_streamer -i "input_uvc.so -d /dev/video0 -y -r 640x480 -f 10" '
        '-o "output_http.so -p 8080 -w /usr/local/share/mjpg-streamer/www"'
    )
    mjpg_proc = subprocess.Popen(mjpg_cmd, shell=True, preexec_fn=os.setsid)
    time.sleep(5)

    ngrok_proc = subprocess.Popen(["ngrok", "http", "8080"], preexec_fn=os.setsid)
    time.sleep(10)

    try:
        response = requests.get("http://localhost:4040/api/tunnels", timeout=10)
        tunnels = response.json().get("tunnels", [])
        public_url = ""
        for tunnel in tunnels:
            if "ngrok-free.app" in tunnel.get("public_url", ""):
                public_url = tunnel["public_url"]
                break
        if public_url:
            full_url = public_url + "/?action=stream"
            stream_url_ref.set(full_url)
            print(f"[âœ”] Stream URL sent to Firebase: {full_url}")
        else:
            print("[!] No ngrok tunnel found.")
    except Exception as e:
        print(f"[!] Failed to fetch ngrok URL: {e}")

    upload_dummy_media()

def stop_stream():
    global motion_proc
    print("[INFO] Stopping livestream...")
    kill_processes()
    time.sleep(1)
    remove_stream_flag()
    stream_url_ref.set("")
    try:
        motion_proc = subprocess.Popen(
            ["/home/team1/TEAM1/ultra/motion_logger"],
            preexec_fn=os.setsid
        )
        print("[âœ”] motion_logger restarted.")
    except Exception as e:
        print(f"[!] Failed to start motion_logger: {e}")

# ==== Firebase Listener ====
def listener(event):
    print(f"[FIREBASE] stream_toggle changed: {event.data}")
    if event.data:
        start_stream()
    else:
        stop_stream()

# ==== Startup ====
device_info = {
    "mac_address": mac_address,
    "last_startup": time.strftime("%Y-%m-%d %H:%M:%S")
}
db.reference(device_path).update(device_info)
print(f"[INFO] Device info updated at {device_path}")
print(f"[âœ”] Listening to Firebase stream_toggle at {device_path}/stream_toggle")

trigger_ref.listen(listener)

# ==== Keep Alive ====
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("[INFO] Script terminated by user")
    kill_processes()
    remove_stream_flag()
