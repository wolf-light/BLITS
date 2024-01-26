import time
import serial
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
import http.client as httplib
from threading import Thread, RLock
from typing import Dict
# Alterable Variables
COM_PORT = "COM4"
LOCK = RLock()

# Checks Internet Connection
def internetConnectionPresent(url="www.google.com", timeout=3):
    connection = httplib.HTTPConnection(url, timeout=timeout)
    try:
        connection.request("HEAD", "/")
        connection.close()
        return True
    except:
        return False
 
class DBfuncs:
    def __init__(self, max_points = 200):
        self.max_points = 200
        self.num_points_uploaded = 0
        self.stopped = False
        self.data = {}
        self.internetConnection = internetConnectionPresent()
        # Check for internet connection
        # If true: Set up Firebase, clear current ThrustData
        if (self.internetConnection):
            cred = credentials.Certificate("firebase-key.json")
            firebase_admin.initialize_app(cred)
            self.db = firestore.client()
            self.db.collection("DataTest").document("ourDoc").set({"ThrustData":{}}, merge = True)
            print(f"Set internet connection to {self.internetConnection}")

    def start(self):
        t = Thread(target=self.upload, args=())
        t.daemon = True
        t.start()
        return self
    
    def stop(self):
        self.stopped = True
    
    def upload(self):
        while True: 
            if self.stopped:
                return
            with LOCK:
                if (self.data and self.num_points_uploaded < 200 and len(self.data) >= 2):
                    self.pushtodb(self.data)
                    self.num_points_uploaded += len(self.data)
                    self.data = {}

    def pushtodb(self, data: Dict[str,str]) -> bool:
        if(internetConnectionPresent() == False):
            internetConnection = False
            return
        self.db.collection("DataTest").document("ourDoc").set({"ThrustData": data}, merge = True)


def sendtothread(serial_read: str, dbfunctions: DBfuncs):
    if serial_read == "COLLECT" or serial_read == "SAFING":
        return
    
    temp = serial_read.split(",")
    temp[1] = float(temp[1]) * -1
    temp[1] = str(temp[1])
    with LOCK:
        dbfunctions.data[temp[0]] = temp[1]

#================Main================
def main():
    # Attempt a Serial Connection
    internetConnection = False
    collect = False
    dbcount = 0
    while True:
        try:
            ser = serial.Serial(port = COM_PORT, baudrate=9600, bytesize = 8, timeout=2, stopbits=serial.STOPBITS_ONE)
        except serial.serialutil.SerialException:
            print(f'Failed to connect to serial port, is there a device connected to {COM_PORT}?', flush=True)
            time.sleep(5)
            continue
        break

    print("Serial connected!")
    print("initializing Database")
    dbfunctions = DBfuncs().start()
    print("Waiting for collect command. Outputting raw info now")

    # Constantly read and output what it reads from serial port
    # If it receives a "COLLECT", start writing to a file
    while True:
            if (ser.in_waiting > 0):
                serial_read = ser.readline().strip().decode()
                if(serial_read == "COLLECT") or collect == True:
                    collect = True
                    print(f"Writing: {serial_read}")
                    with open('data.txt', 'a') as file:
                        file.write(serial_read + "\n")
                    sendtothread(serial_read, dbfunctions)
                else:
                    print(serial_read)
    
def test_dbfuncs():
    dbfunctions = DBfuncs().start()
    with open('data_2.txt', 'r') as f:
        lines = f.readlines()
    for line in lines:
        sendtothread(line, dbfunctions)
        time.sleep(0.01)
    time.sleep(3)
    dbfunctions.stop()

if __name__ == "__main__":
    #test_dbfuncs()
    main()