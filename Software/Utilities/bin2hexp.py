#!/usr/bin/python3

'''
Converts a binary file into an intel hex file
Usage: Bin2Hex.py [-A base_address] [-n] [-F yes|no] -b <binary_file> -o <hex_to_be_created>

pavel_a@fastmail.fm 05-aug-2022
Original: https://community.silabs.com/s/article/converting-binary-firmware-image-files-to-intel-hex-files-bin-to-hex-x
'''

import argparse
import sys

sDESC="Converts binary file to Intel Hex"
sEPILOG="Long series of FF bytes will not be copied to the output file unless -F yes flag is given"

HEX_LINE_LEN = 32

def ConstructRecord(recordType, address, data) -> bytes:

    assert 0 <= address < 0x10000
    assert 0 <= recordType <= 255
    record = []

    recordType = recordType & 0xFF
    address = (address >> 8) & 0xFF, address & 0xFF
    numBytes = len(data) & 0xFF
    
    record.append(numBytes)
    record += address
    record.append(recordType)
    record += data
    
    checksum = 0
    for x in record:
        checksum += x
    checksum &= 0xFF
    
    # Two's complement
    checksum = ~checksum + 1
    checksum &= 0xFF
    
    record.append(checksum)

    recordStr = ':'
    for byte in record:
        recordStr += '{:02X}'.format(byte)
    recordStr += '\n'
    
    return recordStr.encode('ascii', errors='strict')

def convertBinaryToHex(binaryPath, hexPath, start_addr = 0, noEndRecord = False, ignoreErasedRecords = True):
    check_sum = 0
    address = start_addr & 0xFFFF  # initial offset, low part
    addr_high = (start_addr >> 16) & 0xFFFF

    hexFile = open(hexPath, 'wb')
    
    version_string = "{\"VERSION\":" + args.version + "}\n"
    byte_version = version_string.encode('utf-8')
    hexFile.write(byte_version)

    if address != 0 :
        hexFile.write(ConstructRecord(0x04, 0x0000, addr_high.to_bytes(2, 'big')))

    with open(binaryPath, 'rb') as binaryFile:
        byte = binaryFile.read(1)

        data = []
        while byte != b'':
            if address == 0 :
                hexFile.write(ConstructRecord(0x04, 0x0000, addr_high.to_bytes(2, 'big')))

            byte = int.from_bytes(byte, byteorder='big') & 0xFF
            check_sum = (check_sum + byte) % 0x10000
            data.append(byte)
            address += 1
            
            if len(data) >= HEX_LINE_LEN:
                for val in data:
                    if val != 0xFF or not ignoreErasedRecords:
                        hexFile.write(ConstructRecord(0x00, address - len(data), data))
                        break
                data = []
                if address > 0xFFFF:
                    addr_high += 1
                    address   -= 0x10000
                    assert address == 0, "start addr must be aligned on HEX_LINE_LEN else revise!"
                
            byte = binaryFile.read(1)

        # last incomplete row
        if len(data) :
            for val in data:
                if val != 0xFF or not ignoreErasedRecords:
                    hexFile.write(ConstructRecord(0x00, address - len(data), data))
                    break
        
    if not noEndRecord:
        hexFile.write(ConstructRecord(0x01, 0x0000, []))

    checksum_string = "{\"CHECKSUM\":" + f"{check_sum}" + "}\n"
    byte_version = checksum_string.encode('utf-8')
    hexFile.write(byte_version)
    hexFile.close()

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description=sDESC, prog="bin2hexp", epilog=sEPILOG)
    parser.add_argument("-o", "--hexPath", required=True,
                                        help="Path to the hex file to be generated")
    parser.add_argument("-b", "--binaryPath", required=True,
                                        help="Path to the source binary file")
    parser.add_argument("-A", "--baseAddress", default = 0,
                        type= lambda arg: int(arg, 16), 
                        help= 'Base address for the hex file (32-bit)')
    parser.add_argument("-n", "--noEndRecord", action='store_true', default=False,
                        help="Do not end the output file with 'end' record. Use for merging several hex files.")
    #Note: type=bool args do not work! so use choice:
    parser.add_argument("-F", "--copyFF", choices=['yes','no'], default='no',
                        help="Copy long sequences of FF bytes to output ('erased flash' areas). default: no")
    parser.add_argument("-V", "--version", required=True,
                                        help="Firemware version included in the header")
    args = parser.parse_args()

    if not (0 <= args.baseAddress < 0xFFFFFFFF) :
        print("Base address is longer than 32 bits!")
        sys.exit(1)

    if args.baseAddress & (HEX_LINE_LEN -1) :
        print("Base address must be aligned on", HEX_LINE_LEN, "bytes!")
        sys.exit(1)

    convertBinaryToHex(args.binaryPath, args.hexPath, args.baseAddress, args.noEndRecord, args.copyFF == 'no')
    
    print("Done! Created hex file: {}".format(args.hexPath))
    sys.exit(0)
