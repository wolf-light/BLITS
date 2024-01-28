import time
#import pyrebase
#from pyrebase import pyrebase
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

def upload_data_to_firebase():
    with open('dataref.txt', 'r') as file:
        line_number = 1
        for line in file:
            line = line.strip()
            doc_ref.child('STIStest').push(line)
            line_number += 1

# Example usage
if __name__ == "__main__":
    data_file_path = "data.txt"
    upload_data_to_firebase()
