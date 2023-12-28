import serial.tools.list_ports
import serial
import time
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from firebase_admin import firestore

import http.client as httplib

from datetime import datetime

def realtimeTest():
    cred = credentials.Certificate(r"./realtimetest-11796-firebase-adminsdk-tbluh-04f6034e20.json")  # Replace with your service account JSON file path
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://realtimetest-11796-default-rtdb.firebaseio.com/'
    })
    
    ref = db.reference('/')
    print("connected to firebase")
    
    nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    for x in nums:
        print(x)
        ref.child('loop test').push(nums[x])
        
    
    
    print("\nDATA PUSHED TO REALTIME\n")
    
def main():
    print("main function\n")
    realtimeTest()
    
if __name__ == "__main__":
    main()
    
