# --------------------------------------------------------------------------------------
# Teach Z80 FlashLoader
#
# This tool downloads a given bin file to the TeachZ80 Flash 
# Stm32 support processor must be put to flash mode before executed
#
# Expected arguments: <binfile.bin>
# Example usage: python3 z80Loader.py blink-flash.bin
#
# Author: Christian Luethi
# Version: 1.0 - December 12 2023
# --------------------------------------------------------------------------------------

# --------------------------------------------------------------------------------------
# Imports and variables
# --------------------------------------------------------------------------------------
import sys, os.path, serial, serial.tools.list_ports, time, math

# --------------------------------------------------------------------------------------
# Configuration
# --------------------------------------------------------------------------------------
bytesPerRecord = 16 #amount of data bytes per hex record
versionString = "1.0"

# **************************************************************************************
# Classes
# **************************************************************************************   
# --------------------------------------------------------------------------------------
# Simple class to store and parse hex records
# --------------------------------------------------------------------------------------
class HexRecord:
    address = 0
    payload = []
    type = 0
    payloadLength = 0
    record = ""
    checksum = 0
    rxState = 0
    
    def resetReceiver(self): rxState = 0
    
    def receiverUpdate(self, character):
        if (self.rxState == 0):
            if (character == ':'): 
                self.rxState = 1
                self.record = ":"
                self.charCount = 0
        elif (self.rxState == 1):
            self.record = self.record + character
            if (len(self.record) == 3):
                self.payloadLength = int(self.record[1:3], 16)
                self.rxState = 2
        elif (self.rxState == 2):
            self.record = self.record + character
            if (len(self.record) == 7):
                self.address = int(self.record[3:7], 16)
                self.rxState = 3
        elif (self.rxState == 3):
            self.record = self.record + character
            if (len(self.record) == 9):
                self.type = int(self.record[7:9], 16)
                if (self.payloadLength == 0): self.rxState = 5
                else: self.rxState = 4
        elif (self.rxState == 4):
            self.record = self.record + character
            if (len(self.record) == 9+2*self.payloadLength):
                self.payload = [0]*self.payloadLength
                for i in range(self.payloadLength):
                    self.payload[i] = int(self.record[9+2*i:11+2*i], 16)
                self.rxState = 5
        elif (self.rxState == 5):      
            self.record = self.record + character
            if (len(self.record) == 11+2*self.payloadLength):
                chesumRx = int(self.record[9+2*self.payloadLength:11+2*self.payloadLength], 16)
                self.calcChecksum()
                self.rxState = 0
                if (chesumRx == self.checksum): return 1
                else: return 2        
        return 0
    
    def parseData(self, type, address, data):
        self.address = address
        self.payload = data
        self.payloadLength = len(data)
        self.type = type
        self.record = ":"
        self.record = self.record + f'{self.payloadLength:0>2X}'
        self.record = self.record + f'{self.address:0>4X}'
        self.record = self.record + f'{self.type:0>2X}'
        for value in self.payload:
            self.record = self.record + f'{value:0>2X}'
        self.calcChecksum()
        self.record = self.record + f'{self.checksum:0>2X}'        
        
    def calcChecksum(self):
        checksum = self.payloadLength;
        checksum += (self.address & 0xFF00) >> 8
        checksum += (self.address & 0x00FF)
        checksum += self.type
        for value in self.payload:
            checksum += value
        checksum = checksum & 0xFF
        self.checksum = self.twos8(checksum) 
        
    def twos8(self, number):
        binary = int("{0:08b}".format(number))
        flipped = ~binary
        flipped = flipped + 1
        str_twos = str(flipped)
        twos = int(str_twos, 2)
        return twos & 255

# **************************************************************************************
# Functions
# **************************************************************************************
# --------------------------------------------------------------------------------------
# Check on each comport if a TeachZ80 is reachable. 
# --------------------------------------------------------------------------------------
def findCommunicationPport():
    # Collect comport information
    # Check if port can be opened, and the Z80 board responds to the request (happens only when it is in boot mode)    
    for port in serial.tools.list_ports.comports():
        try:
            #Open the next port. Will raise an exception if not accessible
            com = serial.Serial(port.device, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)  # open serial port 
            
            # Send the magic sentence to board, which enables flash loader mode automatically
            com.write("..helloTeachZ80FlashLoader".encode())
            time.sleep(0.2) 

            # Send welcome record to the board
            com.reset_input_buffer()
            welcome = HexRecord()
            welcome.parseData(0xAA, 0x0000, [0xF0])
            com.write(welcome.record.encode())
            
            #receive data for a maximum 200 milliseconds
            response = HexRecord()
            loopCount = 0
            while (loopCount < 20):
                loopCount += 1
                time.sleep(0.01)
                for i in range(com.in_waiting):
                    character = com.read()
                    #print(character[0])
                    result = response.receiverUpdate(character.decode())
                    if (result == 1): 
                        if ((response.type == 0xAA) and (response.payload[0] == 0xA1)): return port.device
            
        #exception happened, just move to the next port 
        except Exception as e:
            pass
        
    #no port fount
    return 0
    
# --------------------------------------------------------------------------------------
# Prints the progress of the download to the screen
# --------------------------------------------------------------------------------------
def printProgress(recordnumber, numrecords):
    percentage = recordnumber*100 / numrecords;
    print(f"Download record {recordnumber: >4} of {numrecords: >4} -{percentage: 4.0f}% - ", end="")
    print("[",end="")
    dots = round(percentage / 5)
    for i in range(dots): print("=", end="")
    for i in range(dots,20): print(" ", end="")
    print("] - ", end="")
    
# --------------------------------------------------------------------------------------
# Prints exit code to screen and exits
# --------------------------------------------------------------------------------------
def printAndExit(exitmessage):
    print(exitmessage)
    print("")
    exit()

# **************************************************************************************
# Main Program
# **************************************************************************************
# Welcome message
print("")
print(f"FlashLoader Script Version {versionString}")

# Check arguments provided
if (len(sys.argv) != 2): printAndExit("Invalid usage. Try 'python3 z80Loader.py <inputfile.bin>'")

# Check if the input file is readable, if not exit
filename = str(sys.argv[1])
if (os.path.isfile(filename) == False): printAndExit(f"Invalid input file '{filename}'")

# Get comport on which teachZ80 is connected. If not fount, exit
comport = findCommunicationPport()
if (comport == 0): printAndExit("Cannot find TeachZ80 Board on any available port.\r\nMake sure the Board is connected, and Flash mode is activated.")

# Open the bin file and read the data
bin = open(filename, mode="rb")
bindata = bin.read()
bin.close

# limit amount of databytes to 64k
if ((len(bindata) == 0) or (len(bindata) > 65535)): printAndExit(f"Invalid amount of data read in file: {len(bindata)} bytes")

# All checks done, proceed with parsing and download
# create hex records
recordlist = []
for i in range(math.ceil(len(bindata) / bytesPerRecord)):
    hex = HexRecord()
    data = bindata[i*bytesPerRecord:(i+1)*bytesPerRecord]    
    hex.parseData(0x00, i*bytesPerRecord, data)
    recordlist.append(hex)
# create end of file record
hex = HexRecord()
hex.parseData(0x01, 0x0000, [])
recordlist.append(hex)

# Some output
print("")    
print(f"TeachZ80 fount on {comport}")
print(f"{len(bindata)} data bytes available in '{filename}'")
print(f"{len(recordlist)} HEX records created")
print("")

#download data
try:
    com = serial.Serial(comport, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)  # open serial port 
    com.flush()
    
    for i in range(len(recordlist)):
        printProgress(i+1, len(recordlist))
        
        #send next record to TeachZ80      
        com.write(recordlist[i].record.encode())
        print(recordlist[i].record, end="")
        print(" - ", end="")
        
        #try to receive a full response for 1 seconds max. If no response or and error message is received, abort                       
        response = HexRecord()
        recordConfirmed = False
        loopCount = 0
        while (loopCount < 100):
            loopCount += 1
            time.sleep(0.01)
            for i in range(com.in_waiting):
                character = com.read(1)
                result = response.receiverUpdate(character.decode())
                if (result == 1):
                    if ((response.type == 0xAA) and (response.payload[0] == 0xA1)): recordConfirmed = True
                    loopCount = 100

        if (recordConfirmed): 
            print("CONFIRMED")            
        else: 
            print("ERROR\r\n") 
            printAndExit("ERROR: VALIDATION failed")      
        
except Exception as e:
    printAndExit("ERROR: Unknown Error during Download: " + str(e))
    
#Completed
print("")
printAndExit("DOWNLOAD COMPLETED SUCCESSFULLY")
   

    