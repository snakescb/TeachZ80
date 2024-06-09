#include <CPMFileSystem.h>

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
CPMFileSystem::CPMFileSystem(diskGeometry geometry, Z80SDCard z80sdcard, Z80Bus z80bus) : sdcard(z80sdcard), bus(z80bus) {
	
	//for now assume all disks have the same geometry
	diskdef = diskDefs[geometry];
}

/*--------------------------------------------------------------------------------------------------------
 List all the files on a disk
---------------------------------------------------------------------------------------------------------*/
bool CPMFileSystem::listFiles(uint8_t diskIndex, bool forceRead) {

	//intialize the the disk
	if (readDisk(diskIndex, forceRead)) { 
		Serial.println("");
		file_t* next = disks[diskIndex].files;
		if (next == nullptr) Serial.println("No File Fount");
		else {
			//print header				
			Serial.println(" Recs  Bytes  Ext  Acc");
			while (next != nullptr) {
				Serial.printf(" %4u  %4uk  %3u  ", next->records, next->blocks*diskdef.blocksize >> 10, next->extends);
				Serial.print("R/W  ");
				Serial.write(diskIndex + 'A');
				Serial.print(": ");
				for (int j=0; j<FILE_NAME_MAX_LEN; j++) if (next->name[j] != ' ') Serial.write(next->name[j]);
				Serial.write('.');
				for (int j=0; j<FILE_TYPE_MAX_LEN; j++) if (next->type[j] != ' ') Serial.write(next->type[j]);
				Serial.println("");
				next = next->nextfile;
			}
			//print capacity
			Serial.print(" Bytes Remaining On ");
			Serial.write(diskIndex + 'A');
			Serial.printf(": %uk\r\n", (disks[diskIndex].capacity - disks[diskIndex].usedblocks*diskdef.blocksize) >> 10);
		}
		return true;
	}
	Serial.println("ERROR: Cannot read the Disk");
	return false;
}

/*--------------------------------------------------------------------------------------------------------
 Helper function to recursively delete the linked list of files
---------------------------------------------------------------------------------------------------------*/
void CPMFileSystem::deleteFileTree(file_t* next) {
	if (next == nullptr) return;
	if (next->nextfile != nullptr) deleteFileTree(next->nextfile);	
	delete next;
}

/*--------------------------------------------------------------------------------------------------------
 Called when a disk is loaded
 Does general disk calculations, creates the inital allocation bitmap, counts the number of directory extends
 and builds the file list
 Returns directly when disk is already initalized
---------------------------------------------------------------------------------------------------------*/
bool CPMFileSystem::readDisk(uint8_t diskIndex, bool forceRead) {

	if (disks[diskIndex].initialized && !forceRead) return true; //no refresh required

	//initialize the sd card
	bus.write_controlBit(bus.reset, true);
	result = sdcard.accessCard(true);
	if (result == sdcard.ok) {
		//intiailize some variables
		disks[diskIndex].initialized = false;
		disks[diskIndex].files = nullptr;
		disks[diskIndex].usedblocks = 0;
		disks[diskIndex].usedExtends = 0;
		for (int i=0; i<ALLOCATION_VECTOR_LENGTH; i++) disks[diskIndex].alv[i] = 0;
		deleteFileTree(disks[diskIndex].files);
		disks[diskIndex].files = nullptr;

		//if mbr is not valid yet, read mbr
		//if the amount of partitions fount is smaller than the requested partition return with error (sdPartition 0 == pysical sd partition 1)
		if (mbr.partitions == 0) mbr = sdcard.readMBR();
		if (mbr.partitions < sdPartition + 1) return false;

		//calculate basic disk / filesystem information
		//size of directory in tracks (equal sd blocks). CEILING!
		uint32_t directorySize = diskdef.maxdir*32;
		uint32_t trackSize = diskdef.seclen*diskdef.sectrk; //must be 512!
		disks[diskIndex].directoryTracks = (directorySize + trackSize - 1) / trackSize;
		//available data tracks on the disk
		disks[diskIndex].capacity = (diskdef.tracks - diskdef.boottrk - disks[diskIndex].directoryTracks)*trackSize;		
		//disk start sector on sd card, assumes track size == sd block size
		disks[diskIndex].sdoffset = diskIndex * diskdef.tracks + mbr.partitiontable[sdPartition].block;
		//disk extend mask and shift calculation
		//assumes 16bit block addresses / amount of blocks on disk (DSM) > 255
		//RC can be max 128, and therefore addresses max 16k (128*128) (as designed in cpm 1.4). 
		//One extend can hold blocksize*8 bytes, divide by max RC (128*128), then - 1 -> The max EX value than can appear in one extend
		disks[diskIndex].EXM = (( diskdef.blocksize * 8 ) / ( diskdef.seclen * 128 )) - 1;  
		//trackbuffer casted to array of extends
		directoryExtend_t* extend = (directoryExtend_t*) trackBuffer;
		//the current file entry worked on
		file_t* currentfile = nullptr;

		//read the whole directory space, build the allocation vector and the list of files
		for (uint16_t directorytrack = 0; directorytrack < disks[diskIndex].directoryTracks; directorytrack++) {
			//read the next directory track from the sd card
			uint32_t blockNumber = disks[diskIndex].sdoffset + diskdef.boottrk + directorytrack;
			result = sdcard.readBlock(blockNumber, trackBuffer);
			if (result == sdcard.ok) {	
				//1 track = 1 sd block always holds 16 extends (512 / 32 bytes)
				for (int i=0; i<16; i++) {			
					if (extend[i].status != 0xE5) {			
						disks[diskIndex].usedExtends++;

						//Extend calculation
						uint16_t entrynumber = (32*extend[i].EG + extend[i].EX) / (disks[diskIndex].EXM + 1);
						uint16_t recs = (extend[i].EX & disks[diskIndex].EXM) * 128 + extend[i].RC;
						uint16_t blocks = (recs*diskdef.seclen + diskdef.blocksize - 1) / diskdef.blocksize; 
						disks[diskIndex].usedblocks += blocks;
						

						//DEBUG
						/*
						Serial.printf("Extend %2u, Entry %u, Recs %3u, Blocks %u - ", disks[diskIndex].usedExtends, entrynumber, recs, blocks);
						for (int j=0; j<FILE_NAME_MAX_LEN;j++) Serial.write(extend[i].name[j]);
						Serial.write('.');
						for (int j=0; j<FILE_TYPE_MAX_LEN;j++) Serial.write(extend[i].type[j]);
						Serial.print(" -");
						for (int j=0; j<8; j++) Serial.printf(" %2u", extend[i].allocation[j]);
						Serial.println("");
						*/

						//if entry number is zero, create a new file
						if (entrynumber == 0) {

							file_t* next = new file_t;
							if (currentfile == nullptr) disks[diskIndex].files = next;
							else currentfile->nextfile = next;
							currentfile = next;

							//fill file information
							currentfile->blocks = blocks;
							currentfile->records = recs;
							//copy name and type as is
							memcpy(currentfile->name, extend[i].name, FILE_NAME_MAX_LEN);
							memcpy(currentfile->type, extend[i].type, FILE_TYPE_MAX_LEN);
							//check file flags
							if (extend[i].type[0] & 0x80) currentfile->readonly = true;
							if (extend[i].type[1] & 0x80) currentfile->sysfile = true;
						}
						//if not, there must be an already created file with the same name, add to this file
						else {
							file_t* root = disks[diskIndex].files;
							while (root != nullptr) {
								//compare name
								bool match = true;
								if (memcmp(root->name, extend[i].name, FILE_NAME_MAX_LEN) != 0) match = false;
								if (memcmp(root->type, extend[i].type, FILE_TYPE_MAX_LEN) != 0) match = false;
								if (match) {
									root->extends++;
									root->records += recs;
									root->blocks += blocks;
									break;
								}												
								root = root->nextfile;
							}
						}

						//Build allocation vector
						//max 8 blocks allocated per extend (assumes >255 blocks on disk)								
						for (int j=0; j<8; j++) {
							//Serial.printf("%2u - ", extend[i].allocation[j]);
							if (extend[i].allocation[j] > 0) {
								uint16_t byte = extend[i].allocation[j] / 8;
								uint8_t  bit  = extend[i].allocation[j] % 8;
								disks[diskIndex].alv[byte] |= 1 << bit;				
							}
						}
					}
				}
			}
			else {
				Serial.print("ERROR: Error while reading directory track from SD card");
				sdcard.accessCard(false);
				bus.write_controlBit(bus.reset, false);
				return false;
			}
		}
		//done
		disks[diskIndex].initialized = true;
		sdcard.accessCard(false);
		bus.write_controlBit(bus.reset, false);
		return true;
	}
	Serial.print("ERROR: Cannot Access SD Card");
	sdcard.accessCard(false);
	bus.write_controlBit(bus.reset, false);
	return false;
} 
