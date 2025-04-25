import sys
import os
import socket
import fcntl
import struct
import cloudinary
import cloudinary.uploader
import firebase_admin
from firebase_admin import credentials, db
from datetime import datetime

# ==== MAC Address ====
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

# ==== Cloudinary Config ====
cloudinary.config(
    cloud_name="dipi8jwic",
    api_key="967683396464718",
    api_secret="jw47dfivxq1TFNlb8TNlNf8_zvI",
    secure=True
)

# ==== Firebase Setup ====
if not firebase_admin._apps:
    cred = credentials.Certificate("/home/team1/Downloads/hitheshj.json")
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
    })

# ==== Get MAC and Paths ====
mac_address = get_mac_address('wlan0')
if not mac_address:
    print("[ERROR] Failed to get MAC address")
    sys.exit(1)

room_name = "Hall Room"
device_id = "Camera"
base_path = f"RaspberryPi/{mac_address}/rooms/{room_name}/devices/Camera"

# ==== Input Arguments ====
image_path = sys.argv[1]
video_path = sys.argv[2]
image_filename = os.path.basename(image_path)
video_filename = os.path.basename(video_path)
today = datetime.now().strftime("%Y-%m-%d")
timestamp = datetime.now().strftime("%H_%M_%S")

# ==== Upload to Cloudinary ====
print(f"[INFO] Uploading image: {image_path}")
img_result = cloudinary.uploader.upload(image_path, resource_type="image")
print(f"[INFO] Image URL: {img_result['secure_url']}")

print(f"[INFO] Uploading video: {video_path}")
vid_result = cloudinary.uploader.upload(video_path, resource_type="video")
print(f"[INFO] Video URL: {vid_result['secure_url']}")

# ==== Upload to Firebase ====
upload_data = {
    "image_url": img_result["secure_url"],
    "video_url": vid_result["secure_url"],
    "image_name": image_filename,
    "video_name": video_filename
}
log_path = f"{base_path}/{today}/{timestamp}"
db.reference(log_path).set(upload_data)
print(f"[?] Media logged at {log_path}")

# ==== Update Metadata ====
db.reference(base_path).update({
    "mac_address": mac_address,
    "last_update": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    "cloudinary_url": vid_result["secure_url"],
    "snapshot_url": img_result["secure_url"],
    "file_list": f"{video_filename},{image_filename}"
})
print(f"[?] Metadata updated at {base_path}")
