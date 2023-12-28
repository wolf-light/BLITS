import serial.tools.list_ports
import serial
import time
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from firebase_admin import firestore

import http.client as httplib

from datetime import datetime

def firestoreTest():
    cred = credentials.Certificate(r"./realtimetest-11796-firebase-adminsdk-tbluh-04f6034e20.json")  # Replace with your service account JSON file path
    firebase_admin.initialize_app(cred)
    print("connected to firebase")
    
    db = firestore.client()
    doc_ref = db.collection('ArduinoData').document('test')
    doc_snap = doc_ref.get()
    
    x = doc_snap.get('v1')
    
    print(x)
    
def main():
    print("main function\n")
    firestoreTest()
    
