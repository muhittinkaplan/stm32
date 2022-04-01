#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
ZetCode PyQt5 tutorial

This example shows a tooltip on
a window and a button.

author: Jan Bodnar
website: zetcode.com
last edited: January 2015
"""

import sys
from PyQt5.QtCore import*
from PyQt5.QtWidgets import*
from PyQt5.QtGui import QFont

import usb.core


class USBForm(QWidget):
    def __init__(self):
        super().__init__()

        self.initUI()

    def __init__(self, parent=None):
        super(USBForm, self).__init__(parent)
        self.setWindowTitle("Demo")

        layout = QVBoxLayout()
        self.setLayout(layout)

        self.pbConnect = QPushButton("Bağlan")
        self.pbConnect.clicked.connect(self.con)
        self.isConnected = False

        self.pbLED = QPushButton("LED Blinking")
        self.pbLED.setCheckable(True)
        self.pbLED.clicked.connect(self.changeLED)
        self.pbLED.setEnabled(False)

        layout.addWidget(self.pbConnect)
        layout.addWidget(self.pbLED)

        self.swStatus = QLineEdit()
        self.swStatus.setReadOnly(True)
        layout.addWidget(self.swStatus)

        self.butTimer = QTimer(self)
        self.butTimer.timeout.connect(self.pollButton)
    def con(self):
        if self.isConnected == False:
            # Do USB Connect Here
            self.dev = usb.core.find(idVendor=0x0483, idProduct=0x5751)
            self.dev.set_configuration()

            self.cfg = self.dev.get_active_configuration()
            self.intf = self.cfg[(0, 0)]

            self.ep = usb.util.find_descriptor(
                self.intf,
                # match the first OUT endpoint
                custom_match= \
                    lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_OUT)


            # Sync changeLED
            self.changeLED()




            self.isConnected = True
            self.pbConnect.setText("Bağlantıyı kes")
            self.pbLED.setEnabled(True)
            self.butTimer.start(100)
        else:
            self.isConnected = False
            self.pbConnect.setText("Bağlan")
            self.pbLED.setEnabled(False)
            self.butTimer.stop()

    def changeLED(self):
        if self.pbLED.isChecked():
            # Send command to make LED on
            self.dev.write(self.ep.bEndpointAddress,[0x01, 0x01, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01], 1000)
            self.pbLED.setText("LED On")
        else:
            # Send command to make LED blink
            self.dev.write(self.ep.bEndpointAddress, [0x0, 0x00, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01], 1000)
            self.pbLED.setText("LED Blinking")

    def pollButton(self):

        data = self.dev.read(0x81, 8, timeout=1000)
        if data[0]:
            self.swStatus.setText("Button Pressed")
        else:
            self.swStatus.setText("Button Released")


if __name__ == '__main__':

    app = QApplication(sys.argv)
    form = USBForm()
    form.show()
    sys.exit(app.exec_())


