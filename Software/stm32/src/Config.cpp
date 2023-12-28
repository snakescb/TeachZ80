#include <Config.h>

/* Types and definitions ------------------------------------------------------------------------------- */  
#define CONFIG_CHECKSUM_START  	0xBB
#define CONFIG_BASE_ADDRESS		0x8004000

/* -----------------------------------------------------------------------
 Constructor
-------------------------------------------------------------------------*/
Config::Config() { }

/* -----------------------------------------------------------------------
 defaults
-------------------------------------------------------------------------*/
void Config::defaults(void) {
	
	configdata.clock.z80Clock = 10000000;
	configdata.clock.sioaClock = 7372800;
	configdata.clock.siobClock = 7372800;

	configdata.flash.dumpStart = 0x0000;
	configdata.flash.dumpEnd = 0x03FF;

	write();
}

/* -----------------------------------------------------------------------
 read
-------------------------------------------------------------------------*/
bool Config::read(void) {

	uint8_t* pDest = (uint8_t*) &configdata;
	uint8_t* pSrc  = (uint8_t*) CONFIG_BASE_ADDRESS;
	uint8_t  checksum = CONFIG_CHECKSUM_START;
	
	for (int i=1; i<(int)sizeof(configdata_t); i++) {
		pDest[i] = pSrc[i];
		checksum ^= pSrc[i];		
	}	
	configdata.checksum = pSrc[0];

	//Serial.printf("Configuration read complete. Checksum: %u.\r\n", configdata.checksum);
	//if (checksum == configdata.checksum) Serial.printf("Checksum CONFIRMED\r\n");
	//else Serial.printf("Checksum ERROR\r\n");

	//read content and check integrity
	if (checksum == configdata.checksum) return true;
	return false;
}

/* -----------------------------------------------------------------------
 write
-------------------------------------------------------------------------*/
void Config::write(void) {
			
	uint8_t* pSrcByte = (uint8_t*) &configdata;
	configdata.checksum  = CONFIG_CHECKSUM_START;

	for (int i=1; i<(int)sizeof(configdata_t); i++) configdata.checksum ^= pSrcByte[i];
	pSrcByte[0] = configdata.checksum;

	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.NbSectors = 1;
	EraseInitStruct.Sector = FLASH_SECTOR_1;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	uint32_t sectorError = 0;
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&EraseInitStruct, &sectorError);
	
	uint32_t address = CONFIG_BASE_ADDRESS;
	uint8_t* pSrc = (uint8_t*) &configdata;

	for (int i=0; i<sizeof(configdata_t); i++) HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address++, pSrc[i]);


	HAL_FLASH_Lock();
	//Serial.printf("Configuration write complete. Checksum: %u.\r\n", configdata.checksum);
}




