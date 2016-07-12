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

        else:
            self.ByteCodeZigBee_logging.debug("BAD HEADER")
        self.ByteCodeZigBee_logging.debug("Packet : " + str(packet_temp))
        return packet_temp

if __name__ == "__main__":
    print "Testing In Class : " + __file__
    test = ByteCodeZigBee(loggingLevel=logging.DEBUG)
    aa = [chr(0x54),chr(0xfe)     ,chr(0),chr(1)   ,chr(11)    ,chr(0x0),chr(0x12),chr(0x4b),chr(0x0),chr(0x7),chr(0x1a),chr(0x6e),chr(0x8b), chr(0x35),chr(0xf6) ,chr(0x8e)]
    test.interpretByteCodeToPacket(aa)
