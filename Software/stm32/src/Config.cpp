#include <Config.h>

/* Types and definitions ------------------------------------------------------------------------------- */  
#define CONFIG_CHECKSUM_START  0xAA

/* -----------------------------------------------------------------------
 Constructor
-------------------------------------------------------------------------*/
Config::Config(uint32_t flashPage) {
	this->flashPage = flashPage;
	flashAddress = FLASH_BASE + flashPage*FLASH_PAGE_SIZE;
 }

/* -----------------------------------------------------------------------
 defaults
-------------------------------------------------------------------------*/
void Config::defaults(void) {
	
	configdata.clock.z80Clock = 10000000;
	configdata.clock.sioaClock = 1843200;
	configdata.clock.siobClock = 1843200;

	configdata.flash.dumpStart = 0x0000;
	configdata.flash.dumpEnd = 0x03FF;

	write();
}

/* -----------------------------------------------------------------------
 read
-------------------------------------------------------------------------*/
bool Config::read(void) {

	uint8_t* pDest = (uint8_t*) &configdata;
	uint8_t* pSrc  = (uint8_t*) flashAddress;
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
	EraseInitStruct.Banks = FLASH_BANK_1;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.NbPages = 1;
	EraseInitStruct.Page = flashPage;

	uint32_t pageError = 0;
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&EraseInitStruct, &pageError);
	
	int progcount = sizeof(configdata_t) >> 3;
	if (progcount*8 < sizeof(configdata_t)) progcount++;
	uint32_t destAddress = flashAddress;
	uint64_t* pSrcDoubleWord = (uint64_t*) &configdata;

	for (int i=0; i<progcount; i++) {
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destAddress, pSrcDoubleWord[i]);
		destAddress += 8;
	}

	HAL_FLASH_Lock();
	//Serial.printf("Configuration write complete. Checksum: %u.\r\n", configdata.checksum);
}



