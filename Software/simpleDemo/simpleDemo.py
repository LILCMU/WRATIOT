# Copyright (c) 2015 Supakorn Yukonthong

import serial
import threading
import json
import time
import paho.mqtt.client as mqtt
import logging
import struct

from simpleDemoConfig import simpleDemoConfig
from CoreSystem.byteCodeZigBee import ByteCodeZigBee

MQTTclient = mqtt.Client()

def is_hex(s):
    hex_digits = set(string.hexdigits)
    return s[0:2] == "0x" and all(c in hex_digits for c in s[2:])


def initSerial():
    sp = serial.Serial()
    sp.port = simpleDemoConfig.ZIGBEE_RS232_NAME
    sp.baudrate = 115200
    sp.parity = serial.PARITY_NONE
    sp.bytesize = serial.EIGHTBITS
    sp.stopbits = serial.STOPBITS_ONE
    sp.timeout = None
    sp.xonxoff = False
    sp.rtscts = False
    sp.dsrdtr = False
    sp.open()
    return sp

sp = initSerial()
print sp

ser = sp

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    # client.subscribe("/ZigBeeAtmel/toMQTT")
    client.subscribe(simpleDemoConfig.CORE_CMD_FROM_MQTT_TO_GATEWAY)
    # client.subscribe(self.pathMqtt)

def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))
    if msg.payload == "ON":
        cmd = "ONOFF 255 0 "+ str(int("35846",10)) +" 1"
        writeCommandToSerial(ser, cmd)
        #mock up response
        MQTTclient.publish(simpleDemoConfig.CORE_RESPONSE_FROM_GATEWAY_TO_MQTT, 'ON', 0, True)
    elif msg.payload == "OFF":
        cmd = "ONOFF 255 0 "+ str(int("35846",10)) +" 0"
        writeCommandToSerial(ser, cmd)
        #mock up response
        MQTTclient.publish(simpleDemoConfig.CORE_RESPONSE_FROM_GATEWAY_TO_MQTT, 'OFF', 0, True)



def startMQTTserver():
    MQTTclient.on_connect = on_connect
    MQTTclient.on_message = on_message
    MQTTclient.connect("188.166.233.211", 1883, 60)
    MQTTclient.loop_start()

# use quene list to get data from serial and add to quene list
# the function that have duty to use data to process should get from quene
# To prevent threading process data at the same time you should lock a quene

cmd_Map_Table = [{"CMD":chr(1),"ByteCount":11,"Detail":"DeviceAnnc"}]
ByteCodeInterpreter = ByteCodeZigBee(loggingLevel=logging.DEBUG)
def readInputSerial(ser):
    _temp = []
    counterPayloadByte = 0
    valueBefore = ''
    #_cmdList use to contain complete packet
    _cmdList = []
    while True:
        #print valueBefore
        if counterPayloadByte > 0:
            #print "In read payload :" + str(counterPayloadByte)
            value = ser.read(1)
            _temp.append(value)
            counterPayloadByte = counterPayloadByte - 1
            if counterPayloadByte <= 0:
                _cmdList.append(_temp)
                _temp = []
        elif valueBefore == chr(0x54) and counterPayloadByte <= 0:
            value = ser.read(1)
            if value == chr(0xfe):
                print "Found Header"
                _cmdByte1 = ser.read(1)
                _cmdByte2 = ser.read(1)
                _counterPayloadByte = ser.read(1)

                #print str(ord(_counterPayloadByte))

                _temp.append(chr(0x54))
                _temp.append(chr(0xfe))
                _temp.append(_cmdByte1)
                _temp.append(_cmdByte2)
                _temp.append(_counterPayloadByte)

                counterPayloadByte = ord(_counterPayloadByte)
            else:
                valueBefore = value
        else:
            #print "Header not found"
            #print ser.inWaiting()
            value = ser.read(1)
            valueBefore = value


        for i in _cmdList:
            '''
            try:
                i = json.loads(i)
            except ValueError as e:
                print i
            if type(i) is dict:
                if i.has_key('GG'):
            '''
            #print i
            ByteCodeInterpreter.interpretByteCodeToPacket(i)
        _cmdList = []


def writeCommandToSerial(ser, cmd):
    cmd = cmd + "\n"
    cmd = cmd.encode('ascii')
    print "Write to Serial : " + cmd
    ser.write(cmd)
    time.sleep(0.01)


if __name__ == '__main__':

    #sp = initSerial()
    #print sp


    t1 = threading.Thread(target=readInputSerial, args=(sp,))
    t1.start()

    startMQTTserver()

    while True:
        n = raw_input("Type Command : ")
        writeCommandToSerial(sp, n)


    """
    print "hello"
    print json.dumps(['foo',{'key':'test'}])
    strTest = json.loads('["foo",{"key":"test"}]')
    print strTest[0]
    """
