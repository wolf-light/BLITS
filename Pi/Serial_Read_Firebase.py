import time
import serial
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
import http.client as httplib

# Alterable Variables
COM_PORT = "COM4"

# Checks Internet Connection
def internetConnectionPresent(url="www.google.com", timeout=3):
    connection = httplib.HTTPConnection(url, timeout=timeout)
    try:
        connection.request("HEAD", "/")
        connection.close()
        return True
    except:
        return False
 

# Pushes data to Firebase Database    
def pushtodb(db, dbcount, serial_read, obj1):
    try:
        if serial_read == "COLLECT" or serial_read == "SAFING":
            return
            
        temp = serial_read.split(",")
        temp[1] = float(temp[1]) * -1
        temp[1] = str(temp[1])
        obj1[temp[0]] = temp[1]
        

        if(dbcount < 200 and dbcount % 10  == 0):
            if(internetConnectionPresent() == False):
                internetConnection = False
                return
            db.collection("DataTest").document("ourDoc").set({"ThrustData":obj1}, merge = True) 
            obj1 = {}
    except:
        internetConnection = False


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
    print("Checking WiFi connection...")

    # Check for internet connection
    # If true: Set up Firebase, clear current ThrustData
    if (internetConnectionPresent()):
        cred = credentials.Certificate("firebase-key.json")
        firebase_admin.initialize_app(cred)
        db = firestore.client()
        db.collection("DataTest").document("ourDoc").set({"ThrustData":{}}, merge = True)
        internetConnection = True
        print(f"Set internet connection to {internetConnection}")

    print("Waiting for collect command. Outputting raw info now")

    # Constantly read and output what it reads from serial port
    # If it receives a "COLLECT", start writing to a file
    uploadCount = 0
    global obj1 
    obj1  = {}
    while True:
        with open('data.txt', 'a') as file:
            if (ser.in_waiting > 0):
                serial_read = ser.readline().strip().decode()
                if(serial_read == "COLLECT") or collect == True:
                    collect = True
                    print(f"Writing: {serial_read}")
                    file.write(serial_read)
                    file.write("\n")
                    if (internetConnection):
                        pushtodb(db,dbcount, serial_read, obj1)
                    dbcount += 1
                else:
                    print(serial_read)
            
if __name__ == "__main__":
    main()