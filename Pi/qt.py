from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
import http.client as httplib
from threading import  RLock
from typing import Dict
import time
import traceback, sys
import firebase_admin
from firebase_admin import credentials, firestore

def internetConnectionPresent(url="www.google.com", timeout=3):
    connection = httplib.HTTPConnection(url, timeout=timeout)
    try:
        connection.request("HEAD", "/")
        connection.close()
        return True
    except:
        return False
    
class WorkerSignals(QObject):
    '''
    Defines the signals available from a running worker thread.

    Supported signals are:

    finished
        No data

    error
        tuple (exctype, value, traceback.format_exc() )

    result
        object data returned from processing, anything

    progress
        int indicating % progress

    '''
    finished = pyqtSignal()
    error = pyqtSignal(tuple)
    result = pyqtSignal(object)
    progress = pyqtSignal(int)


class Worker(QRunnable):
    '''
    Worker thread

    Inherits from QRunnable to handler worker thread setup, signals and wrap-up.

    :param callback: The function callback to run on this worker thread. Supplied args and
                     kwargs will be passed through to the runner.
    :type callback: function
    :param args: Arguments to pass to the callback function
    :param kwargs: Keywords to pass to the callback function

    '''

    def __init__(self, fn, *args, **kwargs):
        super(Worker, self).__init__()

        # Store constructor arguments (re-used for processing)
        self.fn = fn
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()

        # Add the callback to our kwargs
        self.kwargs['progress_callback'] = self.signals.progress

    @pyqtSlot()
    def run(self):
        '''
        Initialise the runner function with passed args, kwargs.
        '''

        # Retrieve args/kwargs here; and fire processing using them
        try:
            result = self.fn(*self.args, **self.kwargs)
        except:
            traceback.print_exc()
            exctype, value = sys.exc_info()[:2]
            self.signals.error.emit((exctype, value, traceback.format_exc()))
        else:
            self.signals.result.emit(result)  # Return the result of the processing
        finally:
            self.signals.finished.emit()  # Done



class MainWindow(QMainWindow):


    def __init__(self, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)

        self.counter = 0

        layout = QVBoxLayout()

        self.l = QLabel("Start")
        b = QPushButton("DANGER!")
        b.pressed.connect(self.start_upload)
        b.pressed.connect(self.start_test_dbfuncs)

        s = QPushButton("stop")
        s.pressed.connect(self.stop)
        layout.addWidget(s)

        layout.addWidget(self.l)
        layout.addWidget(b)

        w = QWidget()
        w.setLayout(layout)

        self.setCentralWidget(w)

        self.show()

        self.threadpool = QThreadPool()
        print("Multithreading with maximum %d threads" % self.threadpool.maxThreadCount())

        self.data = {}
        self.lock = RLock()
        self.num_points_uploaded = 0
        self.internetConnection = internetConnectionPresent()
        self.stopped = False
        if (self.internetConnection):
            cred = credentials.Certificate("firebase-key.json")
            firebase_admin.initialize_app(cred)
            self.db = firestore.client()
            self.db.collection("DataTest").document("ourDoc").set({"ThrustData":{}}, merge = True)
            print(f"Set internet connection to {self.internetConnection}")

        self.timer = QTimer()
        self.timer.setInterval(1000)
        self.timer.timeout.connect(self.recurring_timer)
        self.timer.start()


    def test_dbfuncs(self, progress_callback):
        with open('data_2.txt', 'r') as f:
            lines = f.readlines()
        for line in lines:
            temp = line.split(",")
            temp[1] = float(temp[1]) * -1
            temp[1] = str(temp[1])
            with self.lock:
                self.data[temp[0]] = temp[1]
                time.sleep(0.1)

    def start_test_dbfuncs(self):
        worker = Worker(self.test_dbfuncs)
        self.threadpool.start(worker)
    def stop(self):
        self.stopped = True
    def upload(self, progress_callback):
        while True: 
            if self.stopped:
                return
            with self.lock:
                if (self.data and self.num_points_uploaded < 200 and len(self.data) >= 2):
                    self.pushtodb(self.data)
                    self.data = {}
    
    def start_upload(self):
        worker = Worker(self.upload)
        self.threadpool.start(worker)

    def pushtodb(self, data: Dict[str,str]) -> bool:
        if(internetConnectionPresent() == False):
            return
        self.db.collection("DataTest").document("ourDoc").set({"ThrustData": data}, merge = True)


    def recurring_timer(self):
        self.counter +=1
        self.l.setText("Counter: %d" % self.counter)


app = QApplication([])
window = MainWindow()
app.exec_()