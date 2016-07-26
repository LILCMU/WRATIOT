import logging
#import json
import struct

class ByteCodeZigBee:
    def __init__(self,loggingLevel=None):
        self.ByteCodeZigBee_logging = logging
        self.ByteCodeZigBee_logging.basicConfig(level=loggingLevel)
        self.ByteCodeZigBee_logging.debug("DEBUG MODE ON")

    def interpretByteCodeToPacket(self,bc):
        self.ByteCodeZigBee_logging.debug("Input Byte code : " + str(bc))
        packet_temp = {}
        if ord(bc[0]) == 0x54 and ord(bc[1]) == 0xfe:
            cmd = bytearray([ 0, 0, ord( bc[2] ), ord( bc[3] ) ] )
            cmd_pack = struct.unpack('>I', cmd)[0]
            # device annouc
            # Byte Size for each
            # 0x54 0xfe [CMD:2] [byte count:1] [IEEE:8] [SRT:2] [CAP:1]
            if cmd_pack == 1:
                IEEE_ADDR = ''
                for i in range(5,13):
                    IEEE_ADDR += '%02X' % (ord(bc[i]))

                SRT_ADDR = bytearray([ 0, 0, ord( bc[13] ), ord( bc[14] ) ] )
                CAP = bytearray([ 0, 0, 0, ord( bc[15] ) ] )
                packet_temp['CMD'] = 1
                packet_temp['IEEE_ADDR'] = IEEE_ADDR
                packet_temp['SHORT_ADDR'] = struct.unpack('>I', SRT_ADDR)[0]
                packet_temp['CAP'] = struct.unpack('>I', CAP)[0]
            elif cmd_pack == 2:
                packet_temp['CMD'] = 2
                packet_temp['SRC_ADDR'] = struct.unpack('>I', bytearray([ 0, 0, ord( bc[5] ), ord( bc[6] ) ] ) )[0]
                packet_temp['EP'] = ord( bc[7] )
                packet_temp['CLUSTER_ID'] = struct.unpack('>I', bytearray([ 0, 0, ord( bc[8] ), ord( bc[9] ) ] ) )[0]
                packet_temp['ATTR_ID'] = struct.unpack('>I', bytearray([0, 0, ord(bc[10]), ord(bc[11])]) )[0]
                packet_temp['DATA_TYPE'] = ord(bc[12])
                #check Data type
                if packet_temp['DATA_TYPE'] == 0x10:
                    #ZCL_DATATYPE_BOOLEAN
                    packet_temp['DATA'] = ord(bc[13])

                else:
                    self.ByteCodeZigBee_logging.debug("NO DATA MATCHING")
            elif cmd_pack == 3:
                packet_temp['CMD'] = 3
                packet_temp['SRC_ADDR'] = struct.unpack('>I', bytearray([0, 0, ord(bc[5]), ord(bc[6])]))[0]
                packet_temp['TIMEOUT'] = struct.unpack('>I', bytearray([0, 0, ord(bc[7]), ord(bc[8])]))[0]
            elif cmd_pack == 4:
                CacheDeviceAmount = ( ord(bc[4]) - 4 ) / 2
                packet_temp['CMD'] = 4
                packet_temp['CacheDeviceInPacket'] = CacheDeviceAmount
                packet_temp['StartIndex'] = struct.unpack('>I', bytearray([0, 0, ord(bc[7]), ord(bc[8])]))[0]
                packet_temp['CacheDeviceAmount'] = struct.unpack('>I', bytearray([0, 0, ord(bc[5]), ord(bc[6])]))[0]
                CacheDeviceTbList = []
                for i in range(0,packet_temp['CacheDeviceInPacket']):
                    CacheDeviceTbList.append(struct.unpack('>I', bytearray([0, 0, ord(bc[9+(i*2)]), ord(bc[10+(i*2)])]))[0])
                packet_temp['CacheDeviceTable'] = CacheDeviceTbList
            elif cmd_pack == 5:
                packet_temp['CMD'] = 5
                packet_temp['STATUS'] = ord(bc[5])
            elif cmd_pack == 6:
                packet_temp['CMD'] = 6
                ActiveEPCount = ord(bc[7])
                ActiveEPList = []
                for i in range(0,ActiveEPCount):
                    ActiveEPList.append(ord(bc[8+i]))
                packet_temp['ACTIVEEPLIST'] = ActiveEPList
                packet_temp['ACTIVEEPLISTCOUNT'] = ActiveEPCount
                packet_temp['SRC_ADDR'] = struct.unpack('>I', bytearray([0, 0, ord(bc[5]), ord(bc[6])]))[0]

        else:
            self.ByteCodeZigBee_logging.debug("BAD HEADER")
        self.ByteCodeZigBee_logging.debug("Packet : " + str(packet_temp))
        return packet_temp

if __name__ == "__main__":
    print "Testing In Class : " + __file__
    test = ByteCodeZigBee(loggingLevel=logging.DEBUG)
    aa = [chr(0x54),chr(0xfe)     ,chr(0),chr(1)   ,chr(11)    ,chr(0x0),chr(0x12),chr(0x4b),chr(0x0),chr(0x7),chr(0x1a),chr(0x6e),chr(0x8b), chr(0x35),chr(0xf6) ,chr(0x8e)]
    test.interpretByteCodeToPacket(aa)
