import os
import json
import firebase_admin
from firebase_admin import credentials, db
import time

cred_path = "/home/hithesh/Documents/hitheshj.json"

try:
    cred = credentials.Certificate(cred_path)
    if not firebase_admin._apps:
        firebase_admin.initialize_app(cred, {
            'databaseURL': 'https://habiteck-45fcb-default-rtdb.firebaseio.com/'
        })
    print("✅ Connected to Firebase!")
except Exception as e:
    print(f"❌ Error initializing Firebase: {e}")
    exit(1)

PIPE_PATH = '/tmp/uid_pipe'

users = [
    {"uid": "cZS8rrcQ3hQgPRlgcrdZWlajQp62", "name": "Hithesh Jain"},
    {"uid": "kes9iPrUF5OdyJ7BYfJCfVJGrLJ2", "name": "User 2"},
    {"uid": "zOEVrPeGUXONnd1SXXXz9tbZrKQ2", "name": "User 3"}
]

main_mode_path = "/RaspberryPi/2c:cf:67:55:59:e3/mode"
main_mode_ref = db.reference(main_mode_path)

# Store previous main mode to avoid redundant updates
previous_main_mode = None

def update_main_mode():
    global previous_main_mode
    actions = {}

    # Retrieve the action of each user from Firebase
    for user in users:
        data = db.reference(f"Users/{user['uid']}").get()
        if data:
            actions[user['uid']] = data.get("action", "Vacation")
        else:
            actions[user['uid']] = "Vacation"  # Default to Vacation if missing

    print(f"🧾 Current actions: {actions}")

    # Check if all users have 'Vacation' as their action
    if all(action == "Vacation" for action in actions.values()):
        if previous_main_mode != "Vacation":
            main_mode_ref.set("Vacation")
            previous_main_mode = "Vacation"
            print("🚦 Main mode updated to: Vacation")
        else:
            print("ℹ️ Main mode remains Vacation.")
    else:
        if previous_main_mode != "Home":
            main_mode_ref.set("Home")
            previous_main_mode = "Home"
            print("🚦 Main mode updated to: Home")
        else:
            print("ℹ️ Main mode remains Home.")

def toggle_user_action(uid, action):
    ref = db.reference(f"Users/{uid}")
    user_data = ref.get()

    if user_data:
        current_action = user_data.get("action", "Vacation")

        # Only update user action if it's different
        if current_action != action:
            ref.update({
                "action": action  # Update only action, no mode update here
            })
            print(f"✅ Updated user {uid}: Action → {action}")
        else:
            print(f"ℹ️ User {uid} already in action {action}")
        update_main_mode()
    else:
        print(f"❌ User with UID {uid} not found.")

def is_valid_uid(uid):
    return any(user['uid'] == uid for user in users)

def listen_to_pipe():
    print("👂 Listening on pipe...")
    while True:
        if os.path.exists(PIPE_PATH):
            try:
                with open(PIPE_PATH, 'r') as pipe:
                    pipe_data = pipe.read().strip()
                    if pipe_data:
                        try:
                            data = json.loads(pipe_data)
                            uid = data.get('uid')
                            action = data.get('action')

                            if uid and action:
                                print(f"📨 Pipe input: UID={uid}, Action={action}")
                                if is_valid_uid(uid):
                                    toggle_user_action(uid, action)
                                else:
                                    print(f"❌ Invalid UID received: {uid}")
                        except json.JSONDecodeError as e:
                            print(f"❌ JSON decode error: {e}")
            except Exception as e:
                print(f"❌ Error reading from pipe: {e}")
        else:
            print(f"⌛ Waiting for pipe input at {PIPE_PATH}...")
        
        # Prevents high CPU usage by adding a delay before the next check
        time.sleep(1)

if __name__ == "__main__":
    listen_to_pipe()
