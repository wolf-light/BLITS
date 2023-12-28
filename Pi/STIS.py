# -*- coding: utf-8 -*-
"""
Created on Fri Jun 23 16:29:43 2023

@author: Chris
"""

import sys
from PyQt5 import QtCore, QtWidgets, uic, QtSerialPort
from PyQt5.QtCore import QRunnable, pyqtSlot
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import QFileDialog, QMessageBox

from MainWindow import Ui_MainWindow

import serial.tools.list_ports
import serial

import http.client as httplib

from datetime import datetime

import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from firebase_admin import firestore

class MainWindow(QtWidgets.QMainWindow, Ui_MainWindow):
    def __init__(self, *args, obj=None, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)
        self.setupUi(self)
        
        
        
        #print("Initializing")
        # Initialize Firebase Admin SDK
        try:
            cred = credentials.Certificate(r"./realtimetest-11796-firebase-adminsdk-tbluh-04f6034e20.json")  # Replace with your service account JSON file path
            firebase_admin.initialize_app(cred)
            print("connected to firebase")
        except:
            print("failed to connect to firebase")
        
        self.comConnect1.clicked.connect(lambda: self.connectToSerial(1))
        self.comConnect2.clicked.connect(lambda: self.connectToSerial(2))
        self.comConnect3.clicked.connect(lambda: self.connectToSerial(3))
        self.bt_refreshInternet.clicked.connect(self.updateInternetStatus)
        self.bt_refreshComs.clicked.connect(self.populateComPorts)
        
        #Tab Terminal Buttons
        self.tab1_export.clicked.connect(lambda: self.exportTerminal(1))
        self.tab2_export.clicked.connect(lambda: self.exportTerminal(2))
        self.tab3_export.clicked.connect(lambda: self.exportTerminal(3))
        self.tab1_clear.clicked.connect(lambda: self.clearTerminal(1))
        self.tab2_clear.clicked.connect(lambda: self.clearTerminal(2))
        self.tab3_clear.clicked.connect(lambda: self.clearTerminal(3))
        
        self.bt_disconnectSerialPorts.clicked.connect(self.disconnectAllSerialPorts)
        self.errorFormat = '<span style="color:red;">{}</span>'
        self.warningFormat = '<span style="color:orange;">{}</span>'
        self.infoFormat = '<span id="info" style="color:black;">{}</span>'
        
        # Maximum of 3 serial channels, more are possible
        self.serial1 = QtSerialPort.QSerialPort('/dev/ttyUSB0', baudRate=QtSerialPort.QSerialPort.Baud115200, readyRead=lambda: self.receive(1))
        self.serial1Options = {}
        self.serial2 = QtSerialPort.QSerialPort('COM6', baudRate=QtSerialPort.QSerialPort.Baud9600, readyRead=lambda: self.receive(2))
        self.serial2Options = {}
        self.serial3 = QtSerialPort.QSerialPort('COM6', baudRate=QtSerialPort.QSerialPort.Baud9600, readyRead=lambda: self.receive(3))
        self.serial3Options = {}
        self.serialChannels = [self.serial1, self.serial2, self.serial3]
        
        self.tabs = [self.tab, self.tab2, self.tab3]
        self.terminalOutputs = [self.terminalOutput, self.terminalOutput2, self.terminalOutput3]
        
        self.populateComPorts()
        
        self.updateInternetStatus()
            
    
    def populateComPorts(self):
        ports = serial.tools.list_ports.comports()
        self.comSelection1.clear()
        self.comSelection2.clear()
        self.comSelection3.clear()
        
        for port, desc, hwid in sorted(ports):
                if len(desc) > 20:
                    desc = f"{desc[:20]}..."
                self.comSelection1.addItem(f"{port}: {desc}")
                self.comSelection2.addItem(f"{port}: {desc}")
                self.comSelection3.addItem(f"{port}: {desc}")
        self.sendMessageToDebug("COM ports populated", "INFO")
        
    def disconnectAllSerialPorts(self):
        comStatus = [self.comStatus1, self.comStatus2, self.comStatus3]
        comButton = [self.comConnect1, self.comConnect2, self.comConnect3]
        count = 0
        for serialChannel in self.serialChannels:
            comButton[count].setText("Connect")
            comButton[count].setChecked(False)
            comStatus[count].setText("Status: Disconnected")
            comStatus[count].setStyleSheet("color: black; font-weight: bold;")
            serialChannel.close()
            count = count+1
            
        count = 0
        for tab in self.tabs:
            color = QColor("red")
            if (self.tabWidget.tabBar().tabText(count) != "NOCOM"):
                self.tabWidget.tabBar().setTabTextColor(count, color)
            self.tabWidget.setTabText(count, "NOCOM")
            count = count+1
        
        
        self.sendMessageToDebug("Disconnected all serial ports", "WARN")
    
    def connectToSerial(self, buttonNumber):
        comSelection = [self.comSelection1, self.comSelection2, self.comSelection3]
        comStatus = [self.comStatus1, self.comStatus2, self.comStatus3]
        comButton = [self.comConnect1, self.comConnect2, self.comConnect3]
        comPort = comSelection[buttonNumber-1].currentText().split(':')[0]
        
        selectedChannel = self.serialChannels[buttonNumber-1]
        
        info = QtSerialPort.QSerialPortInfo()
        for port in info.availablePorts():
            if(port.portName() == comPort):
                print(f"Setting to: {port.portName()}")
                selectedChannel.setPortName(port.portName())
                selectedChannel.setBaudRate(QtSerialPort.QSerialPort.Baud9600)
        
        if comButton[buttonNumber-1].isChecked():
            comButton[buttonNumber-1].setText("Disconnect")
            if selectedChannel.open(QtCore.QIODevice.ReadWrite):
                self.sendMessageToDebug(f"Connection to {comPort} successful", 'INFO')
                selectedChannel.setDataTerminalReady(True)
                self.tabs[buttonNumber-1].setEnabled(True)
                color = QColor("black")
                self.tabWidget.tabBar().setTabTextColor(buttonNumber-1, color)
                self.tabWidget.setTabText(buttonNumber-1, comPort)
                comStatus[buttonNumber-1].setText("Status: Success")
                comStatus[buttonNumber-1].setStyleSheet("color: green; font-weight: bold;")
            else:
                self.sendMessageToDebug(f"Connection to {comPort} Failed", 'ERR')
                self.sendMessageToDebug(selectedChannel.errorString(), "ERR")
                comStatus[buttonNumber-1].setText("Status: Failed")
                comStatus[buttonNumber-1].setStyleSheet("color: red; font-weight: bold;")
                comButton[buttonNumber-1].setChecked(False)
                comButton[buttonNumber-1].setText("Connect")
        else:
            color = QColor("red")
            self.sendMessageToDebug(f"{comPort} disconnected", 'WARN')
            comButton[buttonNumber-1].setText("Connect")
            comStatus[buttonNumber-1].setText("Status: Disconnected")
            comStatus[buttonNumber-1].setStyleSheet("color: black; font-weight: bold;")
            self.tabWidget.setTabText(buttonNumber-1, "NOCOM")
            self.tabWidget.tabBar().setTabTextColor(buttonNumber-1, color)
            selectedChannel.close()
    
    '''
    @pyqtSlot()
    def receive(self, buttonNumber):
        terminals = [self.terminalOutput, self.terminalOutput2, self.terminalOutput3]
        while self.serialChannels[buttonNumber-1].canReadLine():
            text = self.serialChannels[buttonNumber-1].readLine()
            text = text.data().decode()
            text = text.rstrip('\r\n')
            terminals[buttonNumber-1].append(text)
            
            #preliminary save function
            try:
                with open('./data.txt', 'w') as file:
                    while True:
                        if self.serial1.in_waiting > 0:
                            data = self.serial1.readline().decode().strip()
                            file.write(data +'\n')
                            file.flush()
            except:
                print("failed to live write to file")
    '''
    
    @pyqtSlot()
    def receive(self, buttonNumber):
        terminals = [self.terminalOutput, self.terminalOutput2, self.terminalOutput3]
        file_name = f"data.txt"  # Change the file name as needed

        with open(file_name, 'a') as file:
            while self.serialChannels[buttonNumber-1].canReadLine():
                text = self.serialChannels[buttonNumber-1].readLine()
                text = text.data().decode()
                text = text.rstrip('\r\n')

                # Append received data to terminalOutput
                terminals[buttonNumber-1].append(text)

                # Write received data to file
                file.write(text + '\n')
                file.flush()  # Ensure data is written immediately

                # Optionally, you can print the received data
                #print(text)
            
    def sendMessageToDebug(self, msg, msgType):
        now = datetime.now()
        currentTime = now.strftime("%H:%M:%S")
        if msgType == 'ERR':
            self.debugOutput.append(self.errorFormat.format(f"[{currentTime} ERROR]: {msg}"))
        elif msgType == 'WARN':
            self.debugOutput.append(self.warningFormat.format(f"[{currentTime} WARN]: {msg}"))
        else:
            self.debugOutput.append(self.infoFormat.format(f"[{currentTime} INFO]: {msg}"))
            
    def closeEvent(self, event):
        #When application is x'd out of
        super(MainWindow, self).closeEvent(event)
        self.disconnectAllSerialPorts()
        
    def updateInternetStatus(self):
        if(internetConnectionPresent()):
            self.internetStatus.setText("Internet Connected")
            self.internetStatus.setStyleSheet("color: green; font-weight: bold;")
            self.sendMessageToDebug("Internet connection present", "INFO")
        else:
            self.internetStatus.setText("Internet Disconnected")
            self.internetStatus.setStyleSheet("color: red; font-weight: bold;")
            self.sendMessageToDebug("No internet connection present", "WARN")
            
    def exportTerminal(self, buttonNumber):
        terminal = self.terminalOutputs[buttonNumber-1]
        
        data = terminal.toPlainText().split("\n")
        filename = self.saveFileDialog()
        try:
            with open(filename, "w") as file:
                file.write(terminal.toPlainText())
        except:
            self.sendMessageToDebug("Failed to open file, export failed", "ERR")
        
    def saveFileDialog(self):
        options = QFileDialog.Options()
        #options |= QFileDialog.DontUseNativeDialog
        fileName, _ = QFileDialog.getSaveFileName(self,"QFileDialog.getSaveFileName()","","Text Files (*.txt);;All Files (*)", options=options)
        if fileName:
            return fileName
        
    def clearTerminal(self, buttonNumber):
        terminal = self.terminalOutputs[buttonNumber-1]
        
        msgBox = QMessageBox
        response = msgBox.question(self, '', "Are you sure you want to clear the terminal data?", msgBox.Yes | msgBox.No)
        if response == msgBox.Yes:
            self.sendMessageToDebug("Terminal Cleared", "WARN")
            terminal.clear()
        else:
            self.sendMessageToDebug("Clear Terminal Aborted", "INFO")
            
# Checks Internet Connection
#-=-=-=-=NOT SURE THIS WORKS QUITE YET NEEDS TESTING=-=-=-=-=-
def internetConnectionPresent(url="www.google.com", timeout=3):
    connection = httplib.HTTPConnection(url, timeout=timeout)
    try:
        connection.request("HEAD", "/")
        connection.close()
        return True
    except:
        return False

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.show()
    app.exec()
else:
    print("error running STIS.py, main file may have changed, please update documentation")

'''{
  "type": "service_account",
  "project_id": "realtimetest-11796",
  "private_key_id": "04f6034e2095f40eda2c596850fd2431cfb5afb1",
  "private_key": "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCjrDzmueIaGQtm\nIEKg0pQdJFnpQ/yXDZI2YFvE88HMDS7o3p5vVLD0N7yiOZ0nV4JY8bAZZRIP+YmH\nwi2HsQ2QZQmskSgYatycmLygsxCNFy5fCx+I+Iytowzwi4QO1W+vgy+9jbFazt35\nAF20evz1FSc+cM/VcI1OKo1lmQSKRbl2VWbPRv9+g2nUA5J11aoZqO5KQh1I7Mn7\nda0NwFZYNRI61aPyJbmL3RDE1SG85LfwmY1I9WOCMQ+v73y+cmDxNShU7pLe12zU\n3SQ3R433zQM2siR67Q3lbyHqLDS9PLFjHm6lTmGkRfNP8CD6nvoB+qL1ZulbtF+T\n1X1S+CGxAgMBAAECggEAC14aUb0N0Nm18rchYYmWm2qLqxmbeMNKySeKoCR87Ibq\n3+YcU2eqrZ+UZKEDQvj/Nt28aRsdtMnUTT9FicPYiROXZyqiwgVkAqVSDog5SU9X\nm2bKnykCHkoxiyBpgN0+l05RecUWURQGkzV9DJyxJnV8ZwgPS0f4rHnkna6Rw+ek\ntVM5yG6QprYJg/9G66iPYAWwOYgOVYOChLvebflil96NNe077rxx9tyCrW1N8AE3\nXH0xTNqm0eLiNdeqDOsjl8s8z6KXEoUZpWypS3ik+zg1KdAs3AJnPM2tZr+Xonfy\ni+eJi8QxnFSxFWSiIX5xBwDeIcWzMrMTKW7VdUwo8QKBgQDkjLgfQqukJieXvjsD\nSd9GNTvz8aT6/LycPyPq4xlDjhi1+YRkpN2ia1wTnVy9u2wgjQ5K1j856+hcRyNY\ndXagTUvQ14evwR8+Rj+mtfmN6/hN88H8Vq47VZV+3ZRrAVTJo4HnhdMPAOw/rqeo\npMm+8OqjuzFUQE8nNw09uzuGuQKBgQC3VLsuHnOBLjkFq9XArrZe6QUZUrwaUkVo\ngkxEAqWURcsHoEVuJzoEvyZzcuuKOAudol0GvpW6pbmdj4b0trN7EpPOQJFLzfj2\noxHtXPnHwWl2AILeYjzP4PhY/x+QI3/M34KTK3nW3KTjXFifz2HSG6C6ekOzbVpY\nWYU6Xm72uQKBgQCnCyLX1kbVZv2xLty6/pcuHWKAXbGTsH26RXvfBbVFZuowtvpJ\nBSbou0dFHtLV6dKRyAcWM5lqoD3olCYFVXUqnNQ5BA6sKp7HSVjkZETNshZ+AxdZ\nc5FM1LS7UQBVZVevOM6NhYZJhQcnVGQ+IpNB95vHp93aElbH9ri60cokIQKBgAY6\nz92CVWR3YKKrsyjIBxR0kKLKp++xCgDCJLtO57eBI6QPsyxKEvOkVzcnGY/p1aM2\nqDiZc1cTWJpdDTX1m2W8PN+OXCGaTzJ6WhqZxTcfgGjtHYVXCz/5IhdMHBN20Vnx\n4msD2DEP8F51feF4V5efLrgHfZN0TNShviA8o0KpAoGBAIiMfvpjn0qsUzGVSC0t\nCAASGHN/bEb/JhiGOA3gzzgoY5q1Uwf9AJpURowU7+wwM6QuAHNw6DfeuQtSpVaL\nYOyksCA1NvpHAzSCBkxcg5EMCJbHGY6kAeMk8cMZt+Y5tUttz+g3AHJv6QvTLTDT\nqRjRh6wprjqRMkZ6ptg5yV+E\n-----END PRIVATE KEY-----\n",
  "client_email": "firebase-adminsdk-tbluh@realtimetest-11796.iam.gserviceaccount.com",
  "client_id": "114272851737000473699",
  "auth_uri": "https://accounts.google.com/o/oauth2/auth",
  "token_uri": "https://oauth2.googleapis.com/token",
  "auth_provider_x509_cert_url": "https://www.googleapis.com/oauth2/v1/certs",
  "client_x509_cert_url": "https://www.googleapis.com/robot/v1/metadata/x509/firebase-adminsdk-tbluh%40realtimetest-11796.iam.gserviceaccount.com",
  "universe_domain": "googleapis.com"
}'''
