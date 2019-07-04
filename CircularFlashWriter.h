/** @file CircularFlashWriter.h
 *  @brief Circular log for SPIFlash
 *  
 *  Implementation of a circular log using the SPIFlash library.
 *
 *	Data is stored in BLOCKS. Each BLOCK contains a number of PAGES.
 *
 *  By default the BLOCK size is 4096 bytes and each page is 256 bytes.
 *  Therefore each BLOCK contains 4096/256 = 16 pages.
 *
 *  Items are added onto the end of the log using putAnything. Items can
 *  be removed from the beginning of the log using getAnything. 
 *
 *	The code is designed to make best use of flash memory, in particular
 *  to avoid any rewriting to the same address. 
 *
 *  Note that structures cannot be written that are > page size
 *
 *  @author Chris Knowles
 *  @bug No known bugs.
 */
 
#ifndef CircularFlashWriter_h
#define CircularFlashWriter_h
#endif

#include "Arduino.h"
#include<SPIMemory.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
// Required for Serial on Zero based boards
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#if defined (SIMBLEE)
#define BAUD_RATE 250000
#define RANDPIN 1
#else
#define BAUD_RATE 115200
#define RANDPIN A0
#endif

#define BLOCK_SIZE 4096 //4K
#define PAGE_SIZE 256 //
#define PAGE_WRITING 0x7F //01111111
#define PAGE_WRITTEN 0x3F //00111111
#define PAGE_SENT 0x37    //00110111

class CircularFlashWriter
{
  public:
    SPIFlash* flash;
	uint32_t next;
	uint32_t first;
	uint32_t capacity;
	CircularFlashWriter();
	
	void begin(uint8_t csPin);
	void flashToggle(bool yes);
	void putPacket(byte* data_buffer, int buffer_size);

	uint32_t getNextAddress();
	uint32_t getFirstAddress();
	uint32_t search(uint32_t from,uint32_t to, uint32_t  ss);
	uint32_t size();
	
	
	template <class T> void CircularFlashWriter::putAnything( const T& value) {
	  //if we are at the start of a block then 
	  //erase the first page of this block 
	  //and the next block
	  if (next%BLOCK_SIZE==0) {
		#ifdef CFW_DEBUG
		 Serial.print("ERASE:");
		 Serial.println(next);
		#endif

		flash->writeByte(next, 0xFF); //flag as started
		flash->eraseSector( (next)%capacity);

		flash->writeByte(next + BLOCK_SIZE, 0xFF); //flag as started
		flash->eraseSector( (next + BLOCK_SIZE)%capacity);
	  }
	   flash->writeByte(next, 0x7F); //flag as started
	   byte* p = (byte*)(void*)&value;
	   flash->writeByteArray(next+1, p, sizeof(value));
	   flash->writeByte(next, 0x3F); //flag as written
	   next = (next + PAGE_SIZE) % capacity;  
	}

	template <class T> void CircularFlashWriter::getAnything(const T& value, bool remove) {  
		byte* p = (byte*)(void*)&value;
	  
		flash->readByteArray(first+1, p, sizeof(value));
		if (remove) {
			flash->writeByte(first, 0xFF); //flag as empty
			first = (first + PAGE_SIZE) % capacity;  
		}
		//what happens if first == next??
	}
};

