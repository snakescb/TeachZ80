/* -------------------------------------------------------------------------------------------------------
 Library CPM Filesystem

 This library implements a basic CPM 2.2 filesystem how it is used on the TeachZ80 CPM 
 It is used in collaboration with the SD card driver to initialize or browse CPM disks, upload
 or delete files to/from CPM disks, or update the boot area of disks

 Documentation: 
    https://hc-ddr.hucki.net/wiki/doku.php/cpm/systemdoku

 Simplifications:
    - This driver assumes that 1 cpm track equals 1 sd block (=512 bytes)
    - This driver assumes all disks share the same
    - Designed for CPM 2.2 only
    - Works only for filesystems with more than 255 Blocks cpacity, so the directory allocation vector uses 2 bytes per block
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef CPM_FILESYSTEM_H
#define CPM_FILESYSTEM_H

    #include <Arduino.h>
    #include <Z80SDCard.h>

    #define FILE_NAME_MAX_LEN            8
    #define FILE_TYPE_MAX_LEN            3
    #define TRACK_SIZE                 512
    #define MAX_DISKS                   16
    #define ALLOCATION_VECTOR_LENGTH  ( 0x10000 * 128 / ( 1024 * 8 )) //Max disk capacity = 0x10000*128, minimum block size 1024 > max 8192 bits / 1024 bytes for alloction bitmap per disk

    class CPMFileSystem  {

        struct cpm_diskdef_t {
            uint16_t seclen;            
            uint16_t tracks;
            uint16_t sectrk;
            uint16_t blocksize;                
            uint16_t maxdir;
            uint16_t skew;
            uint16_t boottrk;
        };

        const cpm_diskdef_t diskDefs[1] = {
            { 128, 16384, 4, 8192, 512, 1, 32 }     // Index 0, 8k blocks, 8MB disk, 512bytes per strack, 512 directories, 16k boot area
        };
              
        public:        
            enum diskGeometry : uint8_t { geometry_8k_8m_32_512 = 0 };                           
            CPMFileSystem(diskGeometry geometry, Z80SDCard z80sdcard, Z80Bus z80bus);  
            bool listFiles(uint8_t diskIndex, bool forceRead=false); 
            bool readDisk(uint8_t diskIndex, bool forceRead=false); 

        private:

            struct directoryExtend_t {
                uint8_t status;
                uint8_t name[8];
                uint8_t type[3];
                uint8_t EX;     // Highest extend number of the record, 0..31
                uint8_t S1;     // In CPM 2.2, not supported, always 0
                uint8_t EG;     // Highest Extendgroup of the record (also S2), 0..15
                uint8_t RC;     // In CPM 2.2, the sectors used by an extend is (EX & exm)*128 + RC
                uint16_t allocation[8];
            } __attribute__((packed));

            struct file_t {
                uint8_t   name[FILE_NAME_MAX_LEN];
                uint8_t   type[FILE_TYPE_MAX_LEN];
                uint16_t  extends = 1;
                uint32_t  records = 0;  
                uint16_t  blocks = 0;
                bool      readonly = false;
                bool      sysfile = false;
                file_t*   nextfile = nullptr;
            };

            struct disk_t {
                bool     initialized;                
                uint32_t sdoffset;                
                uint32_t capacity;      
                uint32_t directoryTracks;
                uint32_t usedblocks;
                uint16_t usedExtends;
                uint8_t  EXM;   //extend Mask                
                uint8_t  alv[ALLOCATION_VECTOR_LENGTH];
                file_t*  files;
            };

            void deleteFileTree(file_t* next);

            Z80SDCard sdcard; 
            Z80Bus bus;
            
            Z80SDCard::mbrResult mbr;
            Z80SDCard::sdResult result;
            
            uint8_t sdPartition = 0;
            cpm_diskdef_t diskdef;
            disk_t disks[MAX_DISKS];

            uint8_t trackBuffer[TRACK_SIZE];
        
    };

#endif