# --------------------------------------------------------------------------------------
# STM32 UART FlashLoader
#
# This tool downloads a given bin file to and stm32 processor using the built-in 
# flashloader. It sends a magic word to the stm32, which (if supported) reboots
# the CPU to system memory.
#
# Expected arguments: <binfile.bin>
# Example usage: python3 stm32loader.py firmware.bin
#
# Author: Christian Luethi
# Version: 1.0 - January 03 2024
# --------------------------------------------------------------------------------------

# --------------------------------------------------------------------------------------
# Imports
# --------------------------------------------------------------------------------------
import sys, os.path, serial, serial.tools.list_ports, time, math

# --------------------------------------------------------------------------------------
# Version
# --------------------------------------------------------------------------------------
versionString = "1.0"

# --------------------------------------------------------------------------------------
# Supported Devices 
#
# Add devices programmed with this code here. Each device either needs a fixed
# page size (in bytes), or a sector array with flash sectors sizes (in bytes)
#
# Device ID's can be fount in the device specific reference manuals, search for "DEV_ID"
# --------------------------------------------------------------------------------------
deviceTable = [    
    {"id": 0x435, "name": "STM32L43xxx / STM32L44xxx", "page": 2048},
    {"id": 0x462, "name": "STM32L45xxx / STM32L46xxx", "page": 2048},
    {"id": 0x464, "name": "STM32L41xxx / STM32L42xxx", "page": 2048},
    {"id": 0x452, "name": "STM32F72xxx / STM32F73xxx", "sectors": [16384, 16384, 16384, 16384, 65536, 131072, 131072, 131072]}
]

# --------------------------------------------------------------------------------------
# Constants and Variables
# --------------------------------------------------------------------------------------
serialPort      = None
commandResponse = None
bytesPerCommand = 256                        # bytes per command, max 256, must be dividable by 8
loadStartAddr   = 0x08000000                 # default stm32 flash base address
baudRate        = 115200                     # Serial baudrate default setting

# **************************************************************************************
# Functions
# **************************************************************************************
# --------------------------------------------------------------------------------------
# GET command
# --------------------------------------------------------------------------------------
def commandGet():
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x00, 0xFF]))
    if (waitAck()):
        if(readBootloaderData()):
            if (waitAck()):
                response["protocol"] = f"{(commandResponse[0] & 0xF0) >> 4}.{commandResponse[0] & 0x0F}"
                response["erasetype"] = "Unknown"
                for i in range(len(commandResponse)):
                    if (commandResponse[i] == 0x43): response["erasetype"] = "Standard"
                    if (commandResponse[i] == 0x44): response["erasetype"] = "Extended"
                response["ok"] = True   
    return response

# --------------------------------------------------------------------------------------
# GET ID command
# --------------------------------------------------------------------------------------
def commandGetID():
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x02, 0xFD]))
    if (waitAck()):
        if(readBootloaderData()):
            if (waitAck()):
               response["pid"] = (commandResponse[0] << 8) + commandResponse[1]
               response["ok"] = True               
    return response

# --------------------------------------------------------------------------------------
# GO command
# --------------------------------------------------------------------------------------
def commandGo(address):
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x21, 0xDE]))
    if (waitAck()):
        sendAddress(address)
        if (waitAck()): response["ok"] = True            
    return response 

# --------------------------------------------------------------------------------------
# ERASE command
# --------------------------------------------------------------------------------------
def commandErase(erasepages, timeout=10):
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x43, 0xBC]))
    if (waitAck()):
        if (erasepages == 0xFFFF): serialPort.write(bytearray([0xFF, 0x00])) 
        else:                
            serialPort.write(bytearray([erasepages-1]))
            checksum = erasepages-1
            for i in range(erasepages): 
                serialPort.write(bytearray([i]))        
                checksum = checksum ^ i
            serialPort.write(bytearray([checksum & 0xFF]))        
        if (waitAck(timeout)): response["ok"] = True
    return response 

# --------------------------------------------------------------------------------------
# EXTENDED ERASE command
# --------------------------------------------------------------------------------------
def commandExtendedErase(erasepages, timeout=10):
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x44, 0xBB]))
    if (waitAck()):
        if (erasepages == 0xFFFF): serialPort.write(bytearray([0xFF, 0xFF, 0x00])) 
        else:                                
            serialPort.write(bytearray([((erasepages-1) >> 8) & 0xFF]))
            checksum = (erasepages-1) >> 8                
            serialPort.write(bytearray([(erasepages-1) & 0xFF]))
            checksum = checksum ^ (erasepages-1)
            for i in range(erasepages):                                         
                serialPort.write(bytearray([(i >> 8) & 0xFF]))        
                checksum = checksum ^ (i >> 8)
                serialPort.write(bytearray([i & 0xFF]))
                checksum = checksum ^ i                     
            serialPort.write(bytearray([checksum & 0xFF]))        
        if (waitAck(timeout)): response["ok"] = True            
    return response 

# --------------------------------------------------------------------------------------
# WRITE command
# --------------------------------------------------------------------------------------
def commandWrite(address, data, timeout=1):
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x31, 0xCE]))
    if (waitAck()):
        sendAddress(address)
        if (waitAck()):
            serialPort.write(bytearray([len(data) - 1]))
            checksum = len(data) - 1            
            for d in data: checksum = checksum ^ d
            serialPort.write(bytearray(data))
            serialPort.write(bytearray([checksum & 0xFF]))
            if (waitAck(timeout)): response["ok"] = True
    return response

# --------------------------------------------------------------------------------------
# READ command
# --------------------------------------------------------------------------------------
def commandRead(address, numbytes, timeout=1):
    global serialPort
    response = {}
    response["ok"] = False
    serialPort.write(bytearray([0x11, 0xEE]))
    if (waitAck()):
        sendAddress(address)
        if (waitAck()):
            serialPort.write(bytearray([numbytes - 1]))
            serialPort.write(bytearray([(0xFF ^ (numbytes - 1)) & 0xFF]))
            if (waitAck()):
                 if (readBootloaderData(numbytes, timeout)): response["ok"] = True                       
    return response

# --------------------------------------------------------------------------------------
# Function sending a 32bit address plus CRC to the bootloader
# --------------------------------------------------------------------------------------
def sendAddress(address):
    crc = (address >> 24) ^ (address >> 16) ^ (address >> 8) ^ (address)       
    serialPort.write(bytearray([address >> 24 & 0xFF, address >> 16 & 0xFF, address >> 8 & 0xFF, address & 0xFF, crc & 0xFF]))

# --------------------------------------------------------------------------------------
# Function waiting for stm32 bootloader ack
# This function expects the next byte received being a ack, else it is an error
# --------------------------------------------------------------------------------------
def waitAck(timeout=0.2):
    global serialPort
    loopCount = timeout*1000
    while (loopCount):
        loopCount -= 1
        if (serialPort.in_waiting > 0):
            rx = int.from_bytes(serialPort.read(1))
            if (rx == 0x79): return True
            else: return False  #something else received
        time.sleep(0.001)
    return False                #timeout occured

# --------------------------------------------------------------------------------------
# Function waiting for stm32 bootloader data
# if numbytes == 0, the next byte received by the bootloader is interpreted as the
# number of bytes to follow - 1. If numbytes > 0, this specific amount of bytes is received
# Received bytes are stored in te global commandResponse variable
# --------------------------------------------------------------------------------------
def readBootloaderData(numbytes=0, timeout=1):
    global serialPort
    global commandResponse
    loopCount = timeout*1000
    waitState = 0  
    bytesreceived = 0
    if (numbytes > 0):
        waitState = 1
        commandResponse = [0]*(numbytes)        
    while (loopCount):
        loopCount -= 1
        while (serialPort.in_waiting > 0):
            rx = int.from_bytes(serialPort.read(1))
            if (waitState == 0):               
                commandResponse = [0]*(rx+1)
                waitState = 1
            elif (waitState == 1):
                commandResponse[bytesreceived] = rx
                bytesreceived += 1
                if (bytesreceived == len(commandResponse)): return True     
        time.sleep(0.001)          
    return False    #timeout

# --------------------------------------------------------------------------------------
# Check on each comport if a stm32 in DFU mode is reachable. 
# after this, if successful, the global serialPort object is open and connected to the stm32
# --------------------------------------------------------------------------------------
def autoConnectBootloader():    
    # Collect comport information
    # Check on any port if it can connect to a bootloader      
    global serialPort
    for listport in serial.tools.list_ports.comports():
        response = conntectBootloader(listport.device)
        if (response == "connected"): return listport.name    
    return "none"

# --------------------------------------------------------------------------------------
# Connect to stm32 bootloader on a specific port
# tries to connect with DFU protocol directly. If this fails, it sends thee magic word,
# using "non-DFU" serial settings, and then tries again.
# after this, if successful, the global serialPort object is open and connected to the stm32
# --------------------------------------------------------------------------------------
def conntectBootloader(port):
    global serialPort
    try:
        # send bootloader start command        
        # When bootloader sends an ack, it is successfully connected. If not, change the serial mod
        serialPort = serial.Serial(port, baudrate=baudRate, bytesize=serial.EIGHTBITS, parity=serial.PARITY_EVEN, stopbits=serial.STOPBITS_ONE, timeout=1)  # open serial port            
        serialPort.write(bytearray([0x7F]))   
        if (waitAck()): return "connected"
        serialPort.close()
        
        # Change serial settings to standard 115200 8N1, and send the magic sentence        
        serialPort = serial.Serial(port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)  # open serial port             
        serialPort.write("heySTM32StartYourDFUMode".encode())
        time.sleep(0.2)            
        serialPort.close()        
        
        # change back to DFU serial settings, and try to reach the processor again  
        # if no response, give up
        serialPort = serial.Serial(port, baudrate=baudRate, bytesize=serial.EIGHTBITS, parity=serial.PARITY_EVEN, stopbits=serial.STOPBITS_ONE, timeout=1)  # open serial port    
        serialPort.write(bytearray([0x7F]))   
        if (waitAck()): return "connected"            
        serialPort.close()
        return "noresponse"
                 
    #exception happened, just move to the next port 
    except Exception as e:
        return "porterror"
         
# --------------------------------------------------------------------------------------
# Prints the progress of the download to the screen
# --------------------------------------------------------------------------------------
def printProgress(recordnumber, numrecords):
    percentage = recordnumber*100 / numrecords
    print(f" {recordnumber: >4} of {numrecords: >4} -{percentage: 4.0f}% - ", end="")
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
#--------------------------------------------------------------------------------------
# Process commandline arguments
#--------------------------------------------------------------------------------------
print("")
binfile = "None"
fullChipErase = False
verify = False         
program = True            
printhelp = False
port = "auto"
args = len(sys.argv) - 1
pos = 1
while (args >= pos):
    arg = sys.argv[pos].upper()
    if (pos == 1): binfile = sys.argv[pos]
    if ((arg == "-H") or (arg == "--HELP")): printhelp = True
    if ((arg == "-E") or (arg == "--ERASE")): fullChipErase = True
    if ((arg == "-V") or (arg == "--VERIFY")): verify = True
    if ((arg == "-P") or (arg == "--PORT")): 
        if (pos + 1 < len(sys.argv)): port = sys.argv[pos+1]
        else: 
            print("Invalid port\r\n")
            printhelp = True
            break
    if ((arg == "-B") or (arg == "--BAUDRATE")): 
        if (pos + 1 < len(sys.argv)): baudRate = sys.argv[pos+1]
        else: 
            print("Invalid baudrate\r\n")
            printhelp = True
            break
    pos += 1
    
#--------------------------------------------------------------------------------------
# Check if binary file is valid
#--------------------------------------------------------------------------------------
if ((os.path.isfile(binfile) == False) and (printhelp == False)): 
    print(f"Invalid input file '{binfile}'\r\n")
    printhelp = True

#--------------------------------------------------------------------------------------
# Print command help, then exit
#--------------------------------------------------------------------------------------            
if (printhelp):
    print("usage: python3 stmLoader.py <inputfile.bin> [option]")
    print("Options and arguments:")
    print("-h, --help                : Display help")
    print("-p, --port <port>         : Port to use. Default is auto-detect (eg. COM28 on Windows, /dev/ttyUSB0 on Linux distros)")
    print("-b, --baudrate <baudrate> : Serial baudrate. Default is 115200")
    print("-e, --erase               : Full Chip Erase. Default is erase required pages/sectors only")
    print("-v, --verify              : Verify downloaded code. Default is not do verify downloaded bytes")
    printAndExit("")    

#--------------------------------------------------------------------------------------    
# Arguments ok, print welcome message
#--------------------------------------------------------------------------------------
print(f"STM32 Loader Script Version {versionString}")
print("")
    
#--------------------------------------------------------------------------------------    
# Detect and connect to stm32 DFU. If not connected, exit
#--------------------------------------------------------------------------------------
if (port == "auto"): 
    autoconnectResult = autoConnectBootloader()
    if (autoconnectResult == "none"): printAndExit("Cannot find STM32 device in DFU mode on any available port.")
    print(f"STM32 device auto-detected on {autoconnectResult}")
else:   
    connectResult = conntectBootloader(port)    
    if (connectResult == "noresponse"): printAndExit(f"Cannot connected to STM32 device on '{port}' - No response received")    
    if (connectResult == "porterror"): printAndExit(f"Cannot connected to STM32 device on '{port}' - Serial port exception") 
    print(f"Connected to STM32 device detected on '{port}'")

#--------------------------------------------------------------------------------------
# GET / GETID COMMAND execution. On error, abort
#--------------------------------------------------------------------------------------
getResult = commandGet()
getIdResult = commandGetID()
if ((getResult["ok"] == False) or (getIdResult["ok"] == False)): printAndExit("Error: Get / GetID Command Failed")

#--------------------------------------------------------------------------------------
# Check if this is a known stm32 device
#--------------------------------------------------------------------------------------
devType = None
for dev in deviceTable:
    if (dev["id"] == getIdResult["pid"]):
        devType = dev
        break
    
#--------------------------------------------------------------------------------------
# Print device information
#--------------------------------------------------------------------------------------
print(f"Device type: 0x{getIdResult['pid']:0>4X} - ", end="")
if (devType == None): print("Unsupported device")
else: print (f"{dev['name']} - {getResult['erasetype']} Erase")   
print(f"Bootloader protocol: {getResult['protocol']}")

#--------------------------------------------------------------------------------------
# If the device is unknow, abort, but send a GO command first, just in case 
#--------------------------------------------------------------------------------------
if (devType == None): 
    commandGo(loadStartAddr)
    printAndExit("Unsupported device. Please add device to the device Table.")

#--------------------------------------------------------------------------------------  
# All checks done, proceed with parsing and download
#--------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------
# Open the bin file and read the data
#--------------------------------------------------------------------------------------
bin = open(binfile, mode="rb")
bindata = bin.read()
bin.close()

#--------------------------------------------------------------------------------------
# Create a list of records to download
#--------------------------------------------------------------------------------------
recordlist = []
address = loadStartAddr
for i in range(math.ceil(len(bindata) / bytesPerCommand)):
    data = bindata[i*bytesPerCommand:(i+1)*bytesPerCommand]   
    recordlist.append({"address": address, "data": data})
    address = address + bytesPerCommand

#--------------------------------------------------------------------------------------
# Calculate the pages/sectors that need to be erased. 0xFFFF for full chip erase
#--------------------------------------------------------------------------------------
erasePages = 0xFFFF
if (fullChipErase == False):
    erasePages = 0
    eraseTopAddress = loadStartAddr - 1
    while (eraseTopAddress < loadStartAddr + len(bindata) - 1):
        if ("page" in devType): eraseTopAddress += devType["page"]
        else: eraseTopAddress += devType["sectors"][erasePages]
        erasePages += 1
        
#--------------------------------------------------------------------------------------
# Final information output before erasing / programming
#--------------------------------------------------------------------------------------
print(f"{len(bindata)} data bytes available in '{binfile}'")
print(f"{len(recordlist)} download records created")
    
#--------------------------------------------------------------------------------------
# Erase flash
#--------------------------------------------------------------------------------------
if (fullChipErase): 
    print("Full chip erase... ")
else:
    if ("page" in devType): print(f"Erasing {erasePages} flash-pages (0x{loadStartAddr:0>8X} - 0x{eraseTopAddress:0>8X})... ")
    else: print(f"Erasing {erasePages} flash-sectors (0x{loadStartAddr:0>8X} - 0x{eraseTopAddress:0>8X})... ")

eraseResult = None
if (getResult["erasetype"] == "Extended"): eraseResult = commandExtendedErase(erasePages)
else: eraseResult = commandErase(erasePages)
if (not eraseResult["ok"]): printAndExit("Error while erasing flash. Aborting")
    
#--------------------------------------------------------------------------------------
# Data Download
#--------------------------------------------------------------------------------------
reccount = 0
for rec in recordlist:
    reccount += 1
    print("Downloading ", end="")
    printProgress(reccount, len(recordlist))
    writeResult = commandWrite(rec["address"], rec["data"])
    if (writeResult["ok"]): print("OK")
    else: 
        print("Error")
        printAndExit(f"Error while writing record {reccount} of {len(recordlist)}. Aborting")
        
#--------------------------------------------------------------------------------------
# Verify Data
#--------------------------------------------------------------------------------------
if (verify):
    reccount = 0
    for rec in recordlist:
        reccount += 1
        print("Verifying ", end="")
        printProgress(reccount, len(recordlist))
        readResult = commandRead(rec["address"], len(rec["data"]))
        if (readResult["ok"]):
            for i in range(len(rec["data"])):
                if(commandResponse[i] != rec["data"][i]):
                    print("Error")
                    printAndExit("Error while verifying data record. Aborting")
            print("OK")                
        else: 
            print("Error")
            printAndExit("Error while reading data record. Aborting")

#--------------------------------------------------------------------------------------
# Complete - Start the loaded code, then exit
#--------------------------------------------------------------------------------------    
goResult = commandGo(loadStartAddr)
if (not goResult["ok"]): printAndExit("Error: Go Command Failed")
print("")
printAndExit("DOWNLOAD COMPLETED SUCCESSFULLY")
   

    