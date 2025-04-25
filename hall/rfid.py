import os
import json
import firebase_admin
from firebase_admin import credentials, db
 
cred_path = "/home/team1/Downloads/hitheshj.json"
 
try:
    cred = credentials.Certificate(cred_path)
    if not firebase_admin._apps:
        firebase_admin.initialize_app(cred, {
            'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
        })
    print("? Connected to Firebase!")
except Exception as e:
    print(f"? Error initializing Firebase: {e}")
    exit(1)
 
PIPE_PATH = '/tmp/uid_pipe'
 
users = [
    {"uid": "cZS8rrcQ3hQgPRlgcrdZWlajQp62", "name": "Hithesh Jain"},
    {"uid": "kes9iPrUF5OdyJ7BYfJCfVJGrLJ2", "name": "User 2"},
    {"uid": "zOEVrPeGUXONnd1SXXXz9tbZrKQ2", "name": "User 3"}
]
 
main_mode_path = "/RaspberryPi/2c:cf:67:55:59:e3/mode"
main_mode_ref = db.reference(main_mode_path)
 
def update_main_mode():
    user_modes = []
    for user in users:
        data = db.reference(f"Users/{user['uid']}").get()
        if data:
            user_modes.append(data.get("mode", "Vacation"))
        else:
            user_modes.append("Vacation")  # Default to Vacation if user missing
 
    if "Home" in user_modes:
        main_mode_ref.set("Home")
        print("? At least one user is Home ? Updated main mode to: home")
    else:
        main_mode_ref.set("vacation")
        print("? All users are on Vacation ? Updated main mode to: vacation")
 
def toggle_user_mode(uid, action):
    ref = db.reference(f"Users/{uid}")
    user_data = ref.get()
 
    if user_data:
        updated = False
        if action == "Home" and user_data.get("mode") != "Home":
            user_data["mode"] = "Home"
            updated = True
        elif action == "Vacation" and user_data.get("mode") != "Vacation":
            user_data["mode"] = "Vacation"
            updated = True
 
        if updated:
            ref.update({
                "mode": user_data["mode"],
                "action": action,
            })
            print(f"? Updated user {uid}: Mode set to {user_data['mode']}, Action: {action}")
            update_main_mode()
        else:
            print(f"?? No change in mode for user {uid}.")
    else:
        print(f"? User with UID {uid} not found.")
 
def is_valid_uid(uid):
    return any(user['uid'] == uid for user in users)
 
def listen_to_pipe():
    while True:
        if os.path.exists(PIPE_PATH):
            try:
                pipe = os.open(PIPE_PATH, os.O_RDONLY)
                pipe_data = os.read(pipe, 1024).decode('utf-8').strip()
                os.close(pipe)
 
                if pipe_data:
                    try:
                        data = json.loads(pipe_data)
                        uid = data.get('uid')
                        action = data.get('action')
 
                        if uid and action:
                            if is_valid_uid(uid):
                                print(f"? Received data - UID: {uid}, Action: {action}")
                                toggle_user_mode(uid, action)
                            else:
                                print(f"? Invalid UID received: {uid}")
                    except Exception as e:
                        print(f"?? Error processing pipe data: {e}")
            except Exception as e:
                print(f"?? Error opening pipe: {e}")
        else:
            print(f"? Waiting for pipe input at {PIPE_PATH}...")
 
if __name__ == "__main__":
    listen_to_pipe()
