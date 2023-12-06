import sys
from PyQt5 import QtCore, QtWidgets, uic, QtSerialPort
from PyQt5.QtCore import QRunnable, pyqtSlot
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import QFileDialog, QMessageBox

from MainWindow import Ui_MainWindow

import serial.tools.list_ports
import serial
import time

import http.client as httplib

from datetime import datetime

def send_serial_message(port, message):
    try:
        ser = serial.Serial(port, baudrate=9600, timeout=1)
        print(f"Connected to {ser.name}")
    
        time.sleep(2)
    
        encoded_message = message.encode('utf-8')
    
        ser.write(encoded_message)
        print(f"message sent: {message}")
    
        ser.close()
    except serial.SerialException as e:
        print(f"Error: {e}")
        
serial_port = 'COM3' 

message_to_send = "test message"

send_serial_message(serial_port, message_to_send)
    
    