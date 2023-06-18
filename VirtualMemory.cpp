//
// Created by Yair on 14-Jun-23.
//
#include <exception>
#include <cassert>
#include "VirtualMemory.h"

/**
 * Translates a virtual memory address into an array of page indexes.
 * @param virtualAddress Virtual memory address.
 * @param arr Array of size TABLES_DEPTH + 1 in which the indexes will be stored.
 */
void translate(uint64_t virtualAddress, unsigned int* arr)
{
  uint64_t exponent = TABLES_DEPTH * OFFSET_WIDTH;
  uint64_t divider = 1LL << exponent;
  for (int i = 0; i < TABLES_DEPTH + 1; ++i)
  {
    uint64_t remainder = virtualAddress % divider;
    arr[i] = virtualAddress / divider;
    virtualAddress = remainder;
    divider = divider >> OFFSET_WIDTH;
  }
}

bool is_frame_empty(int frameAddr) {
  int value;
  for (int i=0; i < TABLES_DEPTH; i++) {
	  PMread(frameAddr + i, &value);
	  if (value) {
		  return false;
	  }
	}
  return true;
}

void empty_frame(int frameAddr) {
  int value;
  for (int i=0; i < TABLES_DEPTH; i++) {
	  PMwrite(frameAddr + i, 0);
	}
}

void find_empty_frame(int *newAddr, bool cleanFrame) {
  for (int i = 0; i < ) {

  }

}

int empty_table_handler(int *addr,  int prevAddr, bool cleanFrame){
  int newAddr;
  find_empty_frame(&newAddr, cleanFrame);
  if (!newAddr) {
	  find_frame_to_switch(&newAddr);
	  switch_frames(&newAddr);
  }

  PMwrite (prevAddr, newAddr);
  *addr = newAddr;
}




/**
 * function extract from the page Table tree the physical address of the given
 * virtual Address.
 * @param virtualAddress
 * @param pageAddress
 * @return 0 if fails, 1 else
 */
int get_physical_address(uint64_t virtualAddress, uint64_t * physicalAddress) {

  unsigned int translated[TABLES_DEPTH + 1];
  translate(virtualAddress, translated);
  int prevAddr;
  int addr = 0;
  for (int i = 0 ; i < TABLES_DEPTH; i++) {
	  prevAddr = addr * PAGE_SIZE + translated[i];
	  PMread(addr * PAGE_SIZE + translated[i], &addr);
	  bool cleanFrame = i < TABLES_DEPTH - 1;
	  if (!empty_table_handler(&addr, prevAddr, cleanFrame)) {
		  return 0;
		}
	}

  if (!empty_table_handler(&addr, prevAddr, false)) {
	  return 0;
	}

  *physicalAddress = addr * PAGE_SIZE  + translated[TABLES_DEPTH];
  return 1;
}

/**
 * Initialize the virtual memory.
 */
void VMinitialize()
{
  for (long long int i = 0; i < PAGE_SIZE; ++i)
  {
    PMwrite (i, 0);
  }
}


/**
 * Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value)
{
  try {
	  uint64_t physicalAddress;
	  assert(get_physical_address(virtualAddress, &physicalAddress) );
	  PMread (physicalAddress, value);
	} catch (std::exception &exception) {
	  return 0;
  }
  return 1;

}

/**
 * Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
  try {
	  uint64_t physicalAddress;
	  assert(get_physical_address(virtualAddress, &physicalAddress) );
	  PMwrite (physicalAddress, value);
	} catch (std::exception &exception) {
	  return 0;
	}
  return 1;
}

