//
// Created by Yair on 14-Jun-23.
//
#include <exception>
#include <cassert>
#include <algorithm>
#include <iostream>
#include "VirtualMemory.h"

typedef uint64_t addr_t;

enum rw_t
{
    READ,
    WRITE
};

/**
 * Translates a virtual memory address into an array of page indexes.
 * @param virtualAddress Virtual memory address.
 * @param arr Array of size TABLES_DEPTH + 1 in which the indexes will be stored.
 */
void translate (addr_t virtualAddress, unsigned int *arr)
{
  addr_t exponent = TABLES_DEPTH * OFFSET_WIDTH;
  addr_t divider = 1LL << exponent;
  for (int i = 0; i < TABLES_DEPTH + 1; ++i)
  {
    addr_t remainder = virtualAddress % divider;
    arr[i] = virtualAddress / divider;
    virtualAddress = remainder;
    divider = divider >> OFFSET_WIDTH;
  }
}

/**
 * Checks if the given frame is an empty table.
 * @param frameIndex Index of desired frame.
 * @return true if the frame is an empty table (all 0), false otherwise.
 */
bool is_frame_empty (int frameIndex)
{
  int value;
  addr_t frameAddr = frameIndex * PAGE_SIZE;
  for (int i = 0; i < PAGE_SIZE; i++)
  {
    PMread (frameAddr + i, &value);
    if (value)
    {
      return false;
    }
  }
  return true;
}

void empty_frame (int frameIndex)
{
  addr_t frameAddr = frameIndex * PAGE_SIZE;
  for (int i = 0; i < PAGE_SIZE; i++)
  {
    PMwrite (frameAddr + i, 0);
  }
}

/**
 * Finds the earliest possible empty frame, and writes it into newFrame.
 * Returns a value based on the type of frame found.
 * Solved with recursion.
 * If an empty table is found, it is removed from its parent table.
 * If no empty table is found, newFrame will be equal to 1 + max frame found.
 * @param callFrame Frame calling this function.
 * @param startFrame Starting frame of search. Should be 0 outside of recursion.
 * @param layer Current viewed layer in the hierarchy. Should be 0 outside of recursion.
 * @param newFrame Integer into which the index of the new frame is written.
 * @return Empty table = -1, new frame >= 0.
 */
int find_empty_frame (int callFrame, int startFrame, int layer, int *newFrame)
{
  int maxFrame = startFrame;
  int nextFrame, res;
  bool empty = true;
  addr_t frameAddr = startFrame * PAGE_SIZE;
  // Check entire frame
  for (int i = 0; i < PAGE_SIZE; ++i)
  {
    PMread (frameAddr + i, &nextFrame);
    if (nextFrame == 0)
    {
      continue;
    }
    empty = false;
    if (layer < TABLES_DEPTH - 1) // all frames are pointing to tables
    {
      res = find_empty_frame (callFrame, nextFrame, layer + 1, newFrame);
      if (res == -1)
      {
        PMwrite (frameAddr + i, 0);
        return -1;
      }
      maxFrame = std::max (maxFrame, res); // res >= nextFrame
    }
    else // all frames are pointing to non-tables
    {
      maxFrame = std::max (maxFrame, nextFrame);
    }
  }
  if (empty && startFrame != callFrame)
  {
    *newFrame = startFrame;
    return -1;
  }
  *newFrame = maxFrame + 1;
  return maxFrame;
}

int empty_table_handler (int *addr, addr_t pAddr, bool cleanFrame)
{
  int newFrame;
  int res = find_empty_frame ((int) (pAddr / PAGE_SIZE), 0, 0, &newFrame);
  if (newFrame == NUM_FRAMES)
  {
    //find_frame_to_switch (&newFrame);
    //switch_frames (&newFrame);
    return 0;
  }
  else if (res != -1 && cleanFrame)
  {
    empty_frame (newFrame);
  }
  assert(newFrame < 64);

  PMwrite (pAddr, newFrame);
  *addr = newFrame;
  return 1;
}

/**
 * function extract from the page Table tree the physical address of the given
 * virtual Address.
 * @note includes test prints
 * @param virtualAddress
 * @param pageAddress
 * @return 0 if fails, 1 else
 */
int get_physical_address (addr_t virtualAddress, addr_t *physicalAddress, rw_t mode)
{
  unsigned int translated[TABLES_DEPTH + 1];
  translate (virtualAddress, translated);
  for (auto i: translated)
  {
    std::cout << i << " ";
  }
  std::cout << std::endl;
  addr_t pAddr;
  int frameIndex = 0;
  for (int i = 0; i < TABLES_DEPTH; i++)
  {
    pAddr = frameIndex * PAGE_SIZE + translated[i];
    PMread (pAddr, &frameIndex);
    if (frameIndex == 0)
    {
      bool cleanFrame = i < TABLES_DEPTH - 1;
      if (empty_table_handler (&frameIndex, pAddr, cleanFrame) == 0)
      {
        return 0;
      }
    }
    std::cout << frameIndex << " ";
  }
  std::cout << std::endl;
  *physicalAddress = frameIndex * PAGE_SIZE + translated[TABLES_DEPTH];
  return 1;
}

/**
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  empty_frame (0);
}

/**
 * Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread (addr_t virtualAddress, word_t *value)
{
  try
  {
    addr_t physicalAddress;
    assert(get_physical_address (virtualAddress, &physicalAddress, READ));
    PMread (physicalAddress, value);
  }
  catch (std::exception &exception)
  {
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
int VMwrite (addr_t virtualAddress, word_t value)
{
  try
  {
    addr_t physicalAddress;
    assert(get_physical_address (virtualAddress, &physicalAddress, WRITE));
    PMwrite (physicalAddress, value);
  }
  catch (std::exception &exception)
  {
    return 0;
  }
  return 1;
}


