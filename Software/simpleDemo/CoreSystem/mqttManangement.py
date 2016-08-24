import paho.mqtt.client as mqtt
import threading
import Queue

import time
import logging
import datetime
import json
import sys

from simpleDemoConfig import simpleDemoConfig


class MqttMananagement:

    def __init__(self,GatewayConfigIns=None):
        self.MQTTclient = mqtt.Client()
        self.GatewayConfigIns = GatewayConfigIns
        self.InputQueue = Queue.Queue()
        self.comsumeQueueThread = threading.Thread(target=self.consumeQueue)
        self.LOG_FILENAME = 'mqttManagement_logfile.out'
        self.loggingIns = logging
        self.loggingIns.basicConfig(filename=self.LOG_FILENAME,level=logging.DEBUG)
        #self.loggingIns.debug("test")

    def setInstanceOfSerialProcess(self,serialProcessIns):
        self.serialProcessIns = serialProcessIns


    def on_connect(self,client, userdata, flags, rc):
        print("Connected with result code " + str(rc))
        # client.subscribe("/ZigBeeAtmel/toMQTT")
        client.subscribe(self.GatewayConfigIns.CORE_CONFIG_SYSTEM_MQTT_TO_GATEWAY)
        client.subscribe(self.GatewayConfigIns.CORE_CONFIG_TOPICMATCHING_MQTT_TO_GATEWAY)
        # client.subscribe(self.pathMqtt)

    def on_message(self,client, userdata, msg):

        #print msg.topic+"   "+msg.payload

        messageTemp = []

        if msg.topic == self.GatewayConfigIns.CORE_CONFIG_SYSTEM_MQTT_TO_GATEWAY:
            print msg.payload
        elif msg.topic == self.GatewayConfigIns.CORE_CONFIG_TOPICMATCHING_MQTT_TO_GATEWAY:
            try:
                messageTemp = json.loads(msg.payload)
            except Exception as inst:
                print "Can not convert msg to json "+msg.topic
                print inst

            print messageTemp
            #print messageTemp[0]['cmd']
            self.processMatchingTopic(messageTemp)

        elif len([d for d in self.GatewayConfigIns.COMMAMD_TOPIC_LIST if str(d['TOPICCMD']) == msg.topic]) > 0:
            record_temp = [d for d in self.GatewayConfigIns.COMMAMD_TOPIC_LIST if str(d['TOPICCMD']) == msg.topic][0]
            #print record_temp
            if record_temp['CMDNAME'] == "ONOFF":
                #print "ON"
                try:
                    sp_message = "ONOFF %d %d %d %d" % (record_temp['EP'],0,record_temp['SRCADDR'], 1 if msg.payload == 'ON' else 0  )
                    self.serialProcessIns.SendStringToHardwareGateway(sp_message)

                    sp_message = "READATTR %d %d %d %d %d" % (record_temp['EP'], 0, record_temp['SRCADDR'], 6, 0)
                    self.serialProcessIns.SendStringToHardwareGateway(sp_message)
                except:
                    print sys.exc_info()


                #send read attribute follow onoff to condfirm value was change.
        elif len([d for d in self.GatewayConfigIns.COMMAND_AND_RESPONSE_TOPIC_LIST if str(d['TOPICCMD']) == msg.topic]) > 0:
            print msg.topic + " " + msg.payload + " MATCHING TYPE : 0"
        #serialProcess.gg()



        '''
        print(msg.topic + " " + str(msg.payload))
        if msg.payload == "ON":
            cmd = "ONOFF 255 0 " + str(int("35846", 10)) + " 1"
            writeCommandToSerial(ser, cmd)
            # mock up response
            MQTTclient.publish(self.GatewayConfigIns.CORE_RESPONSE_FROM_GATEWAY_TO_MQTT, 'ON', 0, True)
        elif msg.payload == "OFF":
            cmd = "ONOFF 255 0 " + str(int("35846", 10)) + " 0"
            writeCommandToSerial(ser, cmd)
            # mock up response
            MQTTclient.publish(self.GatewayConfigIns.CORE_RESPONSE_FROM_GATEWAY_TO_MQTT, 'OFF', 0, True)
        '''

    def processMatchingTopic(self,json_temp):
        # clear subscribe
        for i in self.GatewayConfigIns.COMMAMD_TOPIC_LIST:
            self.MQTTclient.unsubscribe(i['TOPICCMD'].encode('ascii'))
            #self.MQTTclient.unsubscribe('WRAT/simpleDemo1/floor1/room101/power/CMD')
        for i in self.GatewayConfigIns.COMMAND_AND_RESPONSE_TOPIC_LIST:
            self.MQTTclient.unsubscribe(i['TOPICCMD'].encode('ascii'))

        self.GatewayConfigIns.COMMAMD_TOPIC_LIST = []
        self.GatewayConfigIns.ROUTINE_TOPIC_LIST = []
        self.GatewayConfigIns.COMMAND_AND_RESPONSE_TOPIC_LIST = []

        for tuple_temp in json_temp:
            if tuple_temp['CMDTYPE'] == 0:
                self.GatewayConfigIns.COMMAMD_TOPIC_LIST.append(tuple_temp)
                #print type(str(tuple_temp['TOPICCMD']))
                self.MQTTclient.subscribe(str(tuple_temp['TOPICCMD']),0)
            elif tuple_temp['CMDTYPE'] == 1:
                self.GatewayConfigIns.ROUTINE_TOPIC_LIST.append(tuple_temp)
            elif tuple_temp['CMDTYPE'] == 2:
                self.GatewayConfigIns.COMMAND_AND_RESPONSE_TOPIC_LIST.append(tuple_temp)
                self.MQTTclient.subscribe(str(tuple_temp['TOPICCMD']), 0)
        print self.GatewayConfigIns.COMMAMD_TOPIC_LIST
        print self.GatewayConfigIns.ROUTINE_TOPIC_LIST
        print self.GatewayConfigIns.COMMAND_AND_RESPONSE_TOPIC_LIST


    def consumeQueue(self):
        #handle string from serial port cc2530 here. Separate message type and send it to the right topic.
        while True:
            json_temp = self.InputQueue.get()
            print "consumeQ : " + str(json_temp)
            #print type(json_temp)


            if json_temp.has_key('CMD'):
                # handle read attr
                #print "consumeQueue : found key"
                if json_temp['CMD'] == 2:
                    #print "consumeQueue : found cmd 2"
                    if len( [ d for d in self.GatewayConfigIns.ROUTINE_TOPIC_LIST if str(d['CMDNAME']) == 'READATTR' \
                                    and d['EP'] == json_temp['EP'] and d['SRCADDR'] == json_temp['SRC_ADDR'] \
                                    and d['CLUSTERID'] == json_temp['CLUSTER_ID'] and d['ATTRIBUTEID'] == json_temp['ATTR_ID'] \
                                    and d['CLUSTERID'] == json_temp['CLUSTER_ID'] ] ) > 0:
                        #find tuple
                        tuple_temp = [ d for d in self.GatewayConfigIns.ROUTINE_TOPIC_LIST if str(d['CMDNAME']) == 'READATTR' \
                                    and d['EP'] == json_temp['EP'] and d['SRCADDR'] == json_temp['SRC_ADDR'] \
                                    and d['CLUSTERID'] == json_temp['CLUSTER_ID'] and d['ATTRIBUTEID'] == json_temp['ATTR_ID'] \
                                    and d['CLUSTERID'] == json_temp['CLUSTER_ID'] ]
                        print "consumeQueue : found" + str(tuple_temp)
                        #onoff
                        tuple_temp = tuple_temp[0]
                        if tuple_temp['CLUSTERID'] == 6:
                            if tuple_temp['ATTRIBUTEID'] == 0:
                                data_temp = ''
                                if json_temp['DATA'] == 1:
                                    data_temp = 'ON'
                                else:
                                    data_temp = 'OFF'
                                self.MQTTclient.publish(tuple_temp['TOPICRESP'].encode('ascii'),data_temp,0,False)


                else:
                    print "consumeQueue : no cmd match"
            else:
                print "consumeQueue : no cmd key"



    def putMessageToQueue(self, JSONtemp):
        self.InputQueue.put(JSONtemp)

    def startMQTTserver(self):
        self.MQTTclient.on_connect = self.on_connect
        self.MQTTclient.on_message = self.on_message
        #while True:
        #    try:
        self.MQTTclient.connect(self.GatewayConfigIns.MQTT_SERVER_IP, self.GatewayConfigIns.MQTT_SERVER_PORT, self.GatewayConfigIns.MQTT_SERVER_KEEPALIVE)
        #        self.loggingIns.debug(datetime.datetime.strftime(datetime.datetime.now(), '%Y-%m-%d %H:%M:%S ')+"MQTT connection is successful")
        #        break
        #    except Exception as inst:
        #        print "MQTT Can not connect to Internet : " + str(inst)
        #        self.loggingIns.debug(datetime.datetime.strftime(datetime.datetime.now(), '%Y-%m-%d %H:%M:%S ')+"MQTT Can not connect to Internet : " + str(inst))
        #        time.sleep(3)
        self.MQTTclient.loop_start()

        #self.comsumeQueueThread.setDaemon(True)
        self.comsumeQueueThread.start()

if __name__ == '__main__':
    GatewayConfig_temp = simpleDemoConfig.GatewayConfig('../CONFIG_GATEWAY.csv')

    GatewayConfig_temp.loadConfig()
    print GatewayConfig_temp.MQTT_SERVER_IP
    MqttMananagementIns = MqttMananagement(GatewayConfigIns=GatewayConfig_temp)
    MqttMananagementIns.startMQTTserver()
    print "test"
    GG = {'STATUS':1}
    TT = json.dumps(GG)
    MqttMananagementIns.putMessageToQueue(GG)
    MqttMananagementIns.putMessageToQueue(TT)
