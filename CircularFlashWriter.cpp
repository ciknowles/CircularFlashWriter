#include "Arduino.h"
#include "CircularFlashWriter.h"

CircularFlashWriter::CircularFlashWriter() {
}

void CircularFlashWriter::begin(uint8_t csPin) {
	 flash = new SPIFlash(csPin);
	 	 
	 flash->begin();
	 capacity = flash->getCapacity(); 
	 next = getNextAddress();
	 first = getFirstAddress();
	 
	 #ifdef CFW_DEBUG
	 Serial.print("PIN: ");
	 Serial.println(flash->csPin);
	 Serial.print("CAPACITY:");
	 Serial.println(capacity);
	 Serial.print("Next:");
	 Serial.println(next);
	 Serial.print("First:");
	 Serial.println(first);
	 #endif
}

/**
  Toggle the flash on/off

  @param yes true is on, false is off
*/
void CircularFlashWriter::flashToggle(bool yes) {
  if (yes) {
    flash->powerUp();
  }
  else 
    flash->powerDown();
}

uint32_t CircularFlashWriter::size() {
	return (next-first)/PAGE_SIZE;
}

/**
  Writes a variable length packet of no more than 255
  bytes to the rolling log

  @param data_buffer[] the bytes to write
  @param bufferSize the size of data_buffer
*/
void CircularFlashWriter::putPacket(byte* data_buffer, int buffer_size) {
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
  //flash->writeByte(next+1, buffer_size);
  flash->writeByteArray(next+1, data_buffer, buffer_size);
  flash->writeByte(next, 0x3F); //flag as written
  #ifdef CFW_DEBUG
  Serial.print("WRITTEN:");
  Serial.println(next);
  #endif CFW_DEBUG
  next = (next + PAGE_SIZE) % capacity;  
}

/**
  Examines the SerialFlash finds the next empty page. Is called
  once on startup. A reference is then kept to this 
  address. 

  @return the next available address
*/
uint32_t CircularFlashWriter::getNextAddress() {
	//search blocks to find last that is written
    uint32_t ad = search(0,capacity, BLOCK_SIZE);
	
	//now seach pages to find next free page
    return (search(ad, ad+BLOCK_SIZE, PAGE_SIZE)+ PAGE_SIZE)%capacity;
}

/**
  Examines the SerialFlash finds the first non-empty page. Must 
  be called after getNextAddress. It should be called once
  once on startup. A reference is then kept to this 
  address. 

  @return the next available address
*/
uint32_t CircularFlashWriter::getFirstAddress() {
	//search blocks to find last that is written
    uint32_t ad = search((next - PAGE_SIZE)%capacity,0, -BLOCK_SIZE);
	
	//now seach pages to find first that was written
    return (search(ad, 0, -PAGE_SIZE))%capacity;
}

/**
  Examines the first byte in a chunk of flash to determine if 
  page has been written.

  @param from the from address
  @param to the to address
  @param ss the step size, normally BLOCK or PAGE
  @param headermask mask to apply to first byte of PAGE
  @return the last written address
*/
uint32_t CircularFlashWriter::search(uint32_t from,uint32_t to, uint32_t ss) {
  byte lb = 255;//isnt written
  uint32_t pt;
  
	if (ss>0) {
	  for ( pt = from;pt<=to;pt=pt+ss) {
		byte b = flash->readByte(pt);   		
		if ((b==255) && (lb!=255)) {
		  return pt-ss;
		}
		lb = b;
	  }
	  return pt;
	}
    
	for ( pt = from;pt>=to;pt=pt-ss) {
		byte b = flash->readByte(pt);   		
		if ((b==255) && (lb!=255)) {
		  return pt-ss;
		}
		lb = b;
	}
	return pt;
}