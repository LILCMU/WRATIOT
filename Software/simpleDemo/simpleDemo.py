# Copyright (c) 2015 Supakorn Yukonthong

import serial
import threading
import json
import time
import paho.mqtt.client as mqtt

from simpleDemoConfig import simpleDemoConfig

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
    sp.timeout = 0.5
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
        cmd = "ONOFF 255 0 "+ str(int("0xa6e3",16)) +" 1"
        writeCommandToSerial(ser, cmd)
        #mock up response
        MQTTclient.publish(simpleDemoConfig.CORE_RESPONSE_FROM_GATEWAY_TO_MQTT, 'ON', 0, True)
    elif msg.payload == "OFF":
        cmd = "ONOFF 255 0 "+ str(int("0xa6e3",16)) +" 0"
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


def readInputSerial(ser):
    _temp = ''
    while True:
        time.sleep(0.01)
        data_left = ser.inWaiting()
        value = ''
        _cmdList = []
        #header with CMD
        #slice with \r\n to list
        if data_left > 0:
            value = ser.read(data_left)
            #print raw data before slice
            print "Raw Incoming Serial, Lenght : " + str(len(value)) + ", Data :" + value
            j = 0;
            _status = False
            for i in value:
                if i == '\r' and value[j + 1] == '\n':
                    _status = False
                    _temp = _temp[3:]
                    _cmdList.append(_temp)
                    _temp = ''
                if i == 'C' and value[j + 1] == 'M' and value[j + 2] == 'D' and not _status:
                    _status = True
                if _status:
                    _temp += i

                j += 1;
            for i in _cmdList:
                print
                print "CMDList Lenght : "+str(len(i))+" CMD : "+i
            #clear _cmdList
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
