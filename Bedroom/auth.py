
import os
import time
import json
import threading
import firebase_admin
from firebase_admin import credentials, db

cred_path = '/home/team1/Downloads/hitheshj.json'

# Named pipe paths
PIPE_TEMP = "/tmp/temp_pipe"
#PIPE_LED = "/tmp/led_pipe"
PIPE_SMOKE = "/tmp/smoke_pipe"
PIPE_AC = "/tmp/ac_pipe"
PIPE_SERVO = "/tmp/servo_pipe"
PIPE_GEYSER = "/tmp/geyser_pipe"

pipes = {
    "temp": PIPE_TEMP,
    #"led": PIPE_LED,
    "smoke": PIPE_SMOKE,
    "ac": PIPE_AC,
    "servo": PIPE_SERVO,
    "geyser": PIPE_GEYSER
}

# Get MAC address
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
temp_fire = f"/RaspberryPi/2c:cf:67:55:59:e3/rooms/Bedroom"

firebase_path_base = f"/RaspberryPi/2c:cf:67:55:59:e3/rooms/Bedroom/devices"
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

# Ensure all named pipes exist
for name, path in pipes.items():
    if not os.path.exists(path):
        os.mkfifo(path)
        print(f"? Created pipe: {path}")
    else:
        print(f"? Pipe exists: {path}")

# Listen to named pipe and update Firebase
def listen_to_pipe(name, path):
    print(f"? Listening on {path} for {name.upper()}...")

    while True:
        try:
            with open(path, "r") as pipe:
                data = pipe.readline().strip()
                if not data:
                    continue

                print(f"{name.upper()} Data: {data}")

                if name == "temp":
                    try:
                        parsed = json.loads(data)

                        if "temperature" in parsed:
                            db.reference(f"{temp_fire}/temperature").set(float(parsed["temperature"]))
                            print(f"? Temperature: {parsed['temperature']}")

                       # if "humidity" in parsed:
                        #    db.reference(f"{firebase_path_base}/Humidity/value").set(float(parsed["humidity"]))
                         #   print(f"? Humidity: {parsed['humidity']}")

                        #if "led" in parsed:
                        #    db.reference(f"{firebase_path_base}/Bedroom_LED/isOn").set(bool(parsed["led"]))
                         #   print(f"? LED: {bool(parsed['led'])}")

                        if "geyser" in parsed:
                            db.reference(f"{firebase_path_base}/Geyser/isOn").set(bool(parsed["geyser"]))
                            print(f"? Geyser: {bool(parsed['geyser'])}")

                    except Exception as je:
                        print(f"? JSON Error in TEMP pipe: {je}")

                elif name == "led":
                    state = bool(int(data))
                    db.reference(f"{firebase_path_base}/Bedroom_LED/isOn").set(state)
                    print(f"? Updated LED: {state}")

                elif name == "smoke":
                    state = bool(int(data))
                    db.reference(f"{firebase_path_base}/SmokeSensor/isOn").set(state)
                    print(f"? Updated Smoke: {state} {firebase_path_base}/SmokeSensor/isOn")
                    

                elif name == "ac":
                    state = bool(data)
                    db.reference(f"{firebase_path_base}/AC/isOn").set(state)
                    print(f"? Updated AC: {state}")

              #  elif name == "servo":
               #     angle = str(data)
                #    db.reference(f"{firebase_path_base}/Curtain/status").set(angle)
                 #   print(f"? Updated Curtain Angle: {angle}")

                elif name == "servo":
                    state = data.strip().lower()
                    # Only accept known discrete servo states
                    if state in ("open", "closed"):
                        db.reference(f"{firebase_path_base}/Curtain/status").set(state)
                        print(f"? Updated Curtain Sensor Status: {state}")
                    elif state == "off":
                        db.reference(f"{firebase_path_base}/Curtain/sensor_status").set(state)
                        print(f"? Updated Curtain Sensor Status: {state}")
                        
                    else:
                        print(f"?? Unknown servo state received: '{data.strip()}'")
        except Exception as e:
            print(f"? Error in {name.upper()} pipe: {e}")
        time.sleep(0.5)

# Monitor Firebase for user changes and write to named pipes
def monitor_firebase():
    
    last_states = {
        #"led": None,
        "ac": None,
        
       # "servo_angle": None,
        #"servo_status": None,
        "geyser": None
    }
    last_servo = None


    while True:
        try:
            #led_state = db.reference(f"{firebase_path_base}/Bedroom_LED/isOn").get()
            servo_cmd    = db.reference(f"{firebase_path_base}/Curtain/status").get()          # "open" or "close"
            servo_sensor = db.reference(f"{firebase_path_base}/Curtain/sensor_status").get()             
            ac_state = db.reference(f"{firebase_path_base}/AC/isOn").get()
            #servo_angle = db.reference(f"{firebase_path_base}/Curtain/status").get()
            #servo_man = db.reference(f"{firebase_path_base}/Curtain/sensor_status").get()
            geyser_state = db.reference(f"{firebase_path_base}/Geyser/isOn").get()


            #if led_state != last_states["led"]:
            #   print(f"? Firebase LED update: {led_state}")
            #with open(PIPE_LED, 'w') as pipe:
             #       pipe.write("1\n" if led_state else "0\n")
              #  last_states["led"] = led_state

            if geyser_state != last_states["geyser"]:
                print(f"? Firebase Geyser update: {geyser_state}")
                with open(PIPE_GEYSER, 'w') as pipe:
                    pipe.write("1\n" if geyser_state else "0\n")
                last_states["geyser"] = geyser_state

            if ac_state != last_states["ac"]:
                print(f"? Firebase AC update: {ac_state}")
                with open(PIPE_AC, 'w') as pipe:
                    pipe.write("1\n" if ac_state else "0\n")
                last_states["ac"] = ac_state

        #    if servo_angle != last_states["servo_angle"]:
         #       print(f"? Firebase Curtain update: {servo_angle}")
#                with open(PIPE_SERVO, 'w') as pipe:
 #                   pipe.write(f"{servo_angle}\n")
  #              last_states["servo"] = servo_angle
   #         if servo_man != last_states["servo_status"]:
    #            print(f"? Firebase Curtain sensor update: {servo_man}")
     #           with open(PIPE_SERVO, 'w') as pipe:
      #              pipe.write(f"{servo_man}\n")
       #         last_states["servo_status"] = servo_man



            # only write when it really changes
            sensor = str(servo_sensor).strip().lower()   # strip whitespace then lower :contentReference[oaicite:2]{index=2}
            cmd    = str(servo_cmd   ).strip().lower()   # strip whitespace then lower :contentReference[oaicite:3]{index=3}

            # 3. Decide payload: "off" if sensor says off, else use user command
            if sensor == "off":
                payload = "off"
            else:
                payload = cmd   # must be "open" or "close"

            # 4. Only write when the payload actually changes
            if payload != last_servo:
                print(f"? Sending servo payload: {payload}")
                with open(PIPE_SERVO, 'w') as pipe:
                    pipe.write(f"{payload}\n")
                last_servo = payload

        except Exception as e:
            print(f"? Error in Firebase monitor: {e}")

        time.sleep(2)

# Start threads
print("? Bedroom Realtime IoT Module Running. Ctrl+C to stop.")
try:
    threads = []

    # Named pipe listeners
    for name, path in pipes.items():
        t = threading.Thread(target=listen_to_pipe, args=(name, path))
        t.daemon = True
        t.start()
        threads.append(t)

    # Firebase monitoring thread
    firebase_thread = threading.Thread(target=monitor_firebase)
    firebase_thread.daemon = True
    firebase_thread.start()
    threads.append(firebase_thread)

    while True:
        time.sleep(1)

except KeyboardInterrupt:
    print("\n? Shutdown")

