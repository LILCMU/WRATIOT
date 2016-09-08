import serial
import threading
import Queue
import logging
import time
import ast
from CoreSystem.byteCodeZigBee import ByteCodeZigBee
from simpleDemoConfig import simpleDemoConfig

from CoreSystem.mqttManangement import MqttMananagement

class SerialProcess:
    def __init__(self,loggingLevel=None,GatewayConfigIns=None):
        self.GatewayConfigIns = GatewayConfigIns
        #self.ByteCodeZigBee_logging = logging
        #self.ByteCodeZigBee_logging.basicConfig(level=loggingLevel)
        #self.ByteCodeZigBee_logging.debug("DEBUG MODE ON")
        self.sp = serial.Serial()
        self.StringToHardwareGatewayQueue = Queue.Queue()
        self.StringToHardwareGateway_event = threading.Event()
        self.StringToHardwareGateway_event.set()
        self.ByteCodeInterpreter = ByteCodeZigBee(loggingLevel=loggingLevel)

    def initSerialPort(self):
        self.sp.port = self.GatewayConfigIns.ZIGBEE_RS232_NAME
        self.sp.baudrate = 115200
        self.sp.parity = serial.PARITY_NONE
        self.sp.bytesize = serial.EIGHTBITS
        self.sp.stopbits = serial.STOPBITS_ONE
        self.sp.timeout = None
        self.sp.xonxoff = False
        self.sp.rtscts = False
        self.sp.dsrdtr = False
        self.sp.open()

    def init(self):
        self.initSerialPort()

    def getSerialInstance(self):
        return self.sp

    def closeSerialPort(self):
        self.sp.close()

    def setInstanceOfMqttManagement(self,mqttManagementIns):
        self.mqttManagementIns = mqttManagementIns

    def readInputSerial(self,ser):
        _temp = []
        counterPayloadByte = 0
        valueBefore = ''
        # _cmdList use to contain complete packet
        _cmdList = []
        while True:
            # print valueBefore
            if counterPayloadByte > 0:
                # print "In read payload :" + str(counterPayloadByte)
                value = ser.read(1)
                _temp.append(value)
                counterPayloadByte = counterPayloadByte - 1
                if counterPayloadByte <= 0:
                    _cmdList.append(_temp)
                    _temp = []
                    # reset header
                    value = ''
                    valueBefore = ''
            elif valueBefore == chr(0x54) and counterPayloadByte <= 0:
                value = ser.read(1)
                if value == chr(0xfe):
                    print "Found Header"
                    _cmdByte1 = ser.read(1)
                    _cmdByte2 = ser.read(1)
                    _counterPayloadByte = ser.read(1)

                    # print str(ord(_counterPayloadByte))

                    _temp.append(chr(0x54))
                    _temp.append(chr(0xfe))
                    _temp.append(_cmdByte1)
                    _temp.append(_cmdByte2)
                    _temp.append(_counterPayloadByte)

                    counterPayloadByte = ord(_counterPayloadByte)
                else:
                    valueBefore = value
            else:
                # print "Header not found"
                # print ser.inWaiting()
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
                packet_fromGateway = self.ByteCodeInterpreter.interpretByteCodeToPacket(i)
                if packet_fromGateway['CMD'] == 8:
                    print "set event"
                    time.sleep(0.1)
                    self.StringToHardwareGateway_event.set()
                    print "event set get cmd 8"
                else:
                    self.mqttManagementIns.putMessageToQueue(packet_fromGateway)
                    #print "get another response : " + str(packet_fromGateway)


            _cmdList = []

    def writeStringToSerial(self,cmd):
        cmd = cmd + "\n"
        #cmd = cmd.encode('ascii')
        print "Write to Serial : " + cmd
        self.sp.write(cmd)
        #time.sleep(0.01)

    def SendStringToHardwareGateway(self,string_temp):
        #On hardware, we use space to separate parameter.
        #So we only support 10 parameters.
        if string_temp.count(' ') <= 10:
            #use for SENDCMD payload
            #Not complete (In Beta)
            if len(string_temp.split()) == 8:
                string_split_temp = string_temp.split()
                if string_split_temp[0] == 'SENDCMD':
                    print "ok"
                    print ast.literal_eval(string_split_temp[7])
                    payloadTemp = ast.literal_eval(string_split_temp[7])
                    payloadTemp_convertToByteCode = string_split_temp[0] +' '+ string_split_temp[1] +' '+ \
                                                    string_split_temp[2] +' '+ string_split_temp[3] +' '+ \
                                                    string_split_temp[4] +' '+ string_split_temp[5] +' '+ \
                                                    string_split_temp[6] +' '

                    for i in payloadTemp:
                        payloadTemp_convertToByteCode += chr(i)
                    print len(payloadTemp_convertToByteCode)
                    string_temp = payloadTemp_convertToByteCode

            self.StringToHardwareGatewayQueue.put(string_temp)
        else:
            print "TOO MUCH PARAMETERS"


    def processSendStringToHardwareGateway(self):
        while True:
            if self.StringToHardwareGateway_event.wait(5.0):
                #print "5555"
                stringFromQ = self.StringToHardwareGatewayQueue.get()
                #print "6666"
                self.writeStringToSerial(stringFromQ)
                self.StringToHardwareGateway_event.clear()
                #print "Exit with set"
            else:
                #report to serial report topic
                self.mqttManagementIns.putMessageToQueue({'CMD':8,'STATUS':2})

                #prevent when serial message is larger than cc2530 buffer.
                #But cc2530 is still working. We give cc2530 an opportunity to have chance to process
                #the next string in serial but we still continouly to report serial is not reponse in time.
                self.StringToHardwareGateway_event.set()
                print "Exit without set from cc2530"


    def loop_start(self):

        t2 = threading.Thread(target=self.processSendStringToHardwareGateway)
        t2.start()

        t1 = threading.Thread(target=self.readInputSerial, args=(self.sp,))
        t1.start()


#test
'''
def gg():
    print "From GG : " + str(simpleDemoConfig.COMMAMD_TOPIC_LIST)
    simpleDemoConfig.loadConfig()
'''

if __name__ == "__main__":
    simpleDemoConfig_temp = simpleDemoConfig.GatewayConfig('../CONFIG_GATEWAY.csv')
    simpleDemoConfig_temp.loadConfig()
    #print simpleDemoConfig_temp.MQTT_PROJECT_ID

    MqttMananagementIns = MqttMananagement(loggingLevel=None,GatewayConfigIns=simpleDemoConfig_temp)

    #loggingLevel=logging.DEBUG
    SerialProcess_temp = SerialProcess(loggingLevel=None,GatewayConfigIns=simpleDemoConfig_temp)
    SerialProcess_temp.init()

    SerialProcess_temp.setInstanceOfMqttManagement(MqttMananagementIns)
    MqttMananagementIns.setInstanceOfSerialProcess(SerialProcess_temp)

    MqttMananagementIns.startMQTTserver()
    SerialProcess_temp.loop_start()

    #print "GG"
    #print simpleDemoConfig_temp.COMMAMD_TOPIC_LIST
    #print SerialProcess_temp.sp

    '''
    for i in range(0, 100):
        print "round : " + str(i)
        SerialProcess_temp.SendStringToHardwareGateway("gggggg")


    toggle_on_off = 1
    for i in range(0,200):
        print "round : " + str(i)
        #SerialProcess_temp.SendStringToHardwareGateway("READATTR 8 0 46831 6 0")
        if toggle_on_off == 1:
            toggle_on_off = 0
        else:
            toggle_on_off = 1
        SerialProcess_temp.SendStringToHardwareGateway("SENDCMD 8 0 18492 6 "+str(toggle_on_off)+" 0 0")
        time.sleep(0.05)
    '''

    while True:
        n = raw_input("Type Command : ")
        SerialProcess_temp.SendStringToHardwareGateway(n)