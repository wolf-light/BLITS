import time
import pyrebase
from pyrebase import pyrebase
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from firebase_admin import firestore

# Initialize Firebase Realtime Database connection

try:
    cred = credentials.Certificate(r"./bulldog-rocketry-a06cc-firebase-adminsdk-7hs4e-4c786d56ca.json")
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://bulldog-rocketry-a06cc-default-rtdb.firebaseio.com/testdata'
    })
    print("Connected to Firebase Realtime Database")
    doc_ref = db.reference('/')
    doc_ref.child('STIStest').push("tracking script functioning")
except Exception as e:
    print(f"Failed to connect to Firebase Realtime Database: {e}")

def upload_data_to_firebase(file_path, batch_size=10):
    
    doc_ref = db.reference('/')
    while True:
        # Read data from the file
        with open(file_path, 'r') as file:
            data_lines = file.readlines()

        # Check if there is new data
        if not data_lines:
            time.sleep(1)  # Sleep for a second before checking again
            continue

        # Upload data in batches to Firebase
        for i in range(0, len(data_lines), batch_size):
            batch = data_lines[i:i + batch_size]
            # Upload batch to Firebase (assuming 'STIStest' as the child node)
            doc_ref.child('STIStest').push(''.join(batch))

        # Clear the file after uploading
        with open(file_path, 'w') as file:
            file.truncate()

        # Optionally, you can add a delay before the next check
        time.sleep(1)

# Example usage
if __name__ == "__main__":
    data_file_path = "./data.txt"
    upload_data_to_firebase(data_file_path)
