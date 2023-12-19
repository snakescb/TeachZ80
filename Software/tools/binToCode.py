# --------------------------------------------------------------------------------------
# Teach Z80 
#
# Simple helper to convert bin files to C array
#
# Expected arguments: <binfile.bin>
# Example usage: python binToCode.py blink-flash.bin
#
# Author: Christian Luethi
# Version: 1.0 - December 12 2023
# --------------------------------------------------------------------------------------


# --------------------------------------------------------------------------------------
# Imports and variables
# --------------------------------------------------------------------------------------
import sys, os.path, math

# --------------------------------------------------------------------------------------
# Configuration
# --------------------------------------------------------------------------------------
arrayName = "z80TestProgram"
lengthName = "Z80_TEST_PROGRAM_LENGTH"
bytesPerLine = 16 
versionString = "1.0"

# **************************************************************************************
# Classes
# **************************************************************************************   

# **************************************************************************************
# Functions
# **************************************************************************************
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
# Check arguments provided
if (len(sys.argv) != 2): printAndExit("Invalid usage. Try 'python binToCode.py <inputfile.bin>'")

# Check if the input file is readable, if not exit
filename = str(sys.argv[1])
if (os.path.isfile(filename) == False): printAndExit(f"Invalid input file '{filename}'")

# Open the bin file and read the data
bin = open(filename, mode="rb")
bindata = bin.read()
bin.close


# All checks done, proceed with parsing and putting to screen
print()
print(f"BinToCode Script Version {versionString}")
print()
print(f"#define {lengthName}     {len(bindata)}")
print()
print("const uint8_t ", end="")
print(arrayName, end="")
print("[] {")
recordlist = []
for i in range(math.ceil(len(bindata) / bytesPerLine)):
    data = bindata[i*bytesPerLine:(i+1)*bytesPerLine]  
    print("     ", end="")
    for j in range(len(data)):
        print (f"0x{data[j]:0>2X}",end="")
        if (i*bytesPerLine + j < len(bindata) - 1):  print(", ",end="")
    print()
print("};")
print()
print()   

    