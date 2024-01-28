import time
import firebase_admin
from firebase_admin import credentials, db

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

def batch_push(lines):
    # You might want to adjust 'STIStest' based on your data structure
    doc_ref.child('STIStest').push(lines)
    time.sleep(.01)

def upload_data_to_firebase():
    with open('data.txt', 'r') as file:
        lines_to_push = []
        line_number = 1

        for line in file:
            line = line.strip()
            lines_to_push.append(line)

            # Push in batches of 100 lines
            if len(lines_to_push) == 50:
                batch_push(lines_to_push)
                lines_to_push = []

            line_number += 1

        # Push any remaining lines
        if lines_to_push:
            batch_push(lines_to_push)
        
        

# Example usage
if __name__ == "__main__":
    data_file_path = "data.txt"
    try:
        upload_data_to_firebase()
    except Exception as e:
        print(f"failed to connect")
