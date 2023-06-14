//
// Created by Yair on 14-Jun-23.
//
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



uint64_t traverse(uint64_t virtualAddress)
{
  int[TABLES_DEPTH + 1] translated;
  translate(virtualAddress, &translated);
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
  uint64_t physicalAddress = traverse (virtualAddress);
  PMread (physicalAddress, value);
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
  uint64_t physicalAddress = traverse (virtualAddress);
  PMwrite (physicalAddress, value);
}


