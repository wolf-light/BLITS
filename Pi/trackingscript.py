# trackingscript.py
from PyQt5.QtCore import QTimer
import firebase_admin
from firebase_admin import credentials, db
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

# Initialize Firebase Realtime Database
try:
    cred = credentials.Certificate(r"./bulldog-rocketry-a06cc-firebase-adminsdk-7hs4e-4c786d56ca.json")
    firebase_admin.initialize_app(cred, {'databaseURL': 'https://bulldog-rocketry-a06cc-default-rtdb.firebaseio.com'})
    print("Connected to Firebase Realtime Database")
except Exception as e:
    print(f"Failed to connect to Firebase Realtime Database: {e}")

# Specify the path to your data.txt file
data_file_path = './data.txt'

# Reference to the Firebase Realtime Database
ref = db.reference('/')

# Class to handle file system events (e.g., file changes)
class MyHandler(FileSystemEventHandler):
    def on_modified(self, event):
        if event.src_path == data_file_path:
            process_data()

def process_data():
    # Read the new data from data.txt
    with open(data_file_path, 'r') as file:
        lines = file.readlines()

    # Upload data to Firebase starting from the last processed line
    last_processed_line = get_last_processed_line_number()
    new_data = lines[last_processed_line:]

    # Upload data to Firebase Realtime Database
    upload_data_to_firebase(new_data)

    # Update the last processed line number
    update_last_processed_line_number(last_processed_line + len(new_data))

def get_last_processed_line_number():
    # Read the last processed line number from Firebase Realtime Database
    try:
        last_processed_line = ref.child('last_processed_line').get()
        if last_processed_line is not None:
            return int(last_processed_line)
    except Exception as e:
        print(f"Error getting last processed line number from Firebase: {e}")
    
    # Return 0 if not found or if starting fresh
    return 0

def update_last_processed_line_number(line_number):
    # Update the last processed line number in Firebase Realtime Database
    try:
        ref.update({'last_processed_line': line_number})
    except Exception as e:
        print(f"Error updating last processed line number in Firebase: {e}")

def upload_data_to_firebase(data):
    # Upload data to Firebase Realtime Database
    for line in data:
        ref.push().set({'data': line.strip()})

if __name__ == "__main__":
    # Initialize the file system event handler
    print(f"script intialized\n")
    event_handler = MyHandler()
    observer = Observer()
    observer.schedule(event_handler, path='.', recursive=False)
    observer.start()
    
    timer = QTimer()
    timer.timeout.connect(process_data)
    timer.start(10000)
    
    try:
        while True:
            observer.join(1)
    except KeyboardInterrupt:
        observer.stop
    
    observer.join()

    try:
        while True:
            time.sleep(10)  # Wait for 10 seconds
            process_data()
    except KeyboardInterrupt:
        observer.stop()

    observer.join()
