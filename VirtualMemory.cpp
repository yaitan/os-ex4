//
// Created by Yair on 14-Jun-23.
//
#include <exception>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <cmath>
#include "VirtualMemory.h"

typedef uint64_t address;

typedef struct SwitchFrame {
	int frameInd;
	int pageInd = -1;
	address parent;
} SwitchFrame;

enum rw_t
{
    READ,
    WRITE
};

void print_ram(word_t *value)
{
  printf("your RAM is: \n");
  for (int i = 0; i < 16; ++i)
	{
	  printf("%d, ", int(value[i]));
	}
  printf("\n....\n");
}


void check_ram ()
{
  word_t value[16];
  for (int i = 0; i < 16; ++i)
	{
	  PMread (i, value + i);
	}
	print_ram(value);
}
/**
 * Translates a virtual memory address into an array of page indexes.
 * @param virtualAddress Virtual memory address.
 * @param arr Array of size TABLES_DEPTH + 1 in which the indexes will be stored.
 */
void translate (address virtualAddress, unsigned int *arr)
{
  address exponent = TABLES_DEPTH * OFFSET_WIDTH;
  address divider = 1LL << exponent;
  for (int i = 0; i < TABLES_DEPTH + 1; ++i)
  {
    address remainder = virtualAddress % divider;
    arr[i] = virtualAddress / divider;
    virtualAddress = remainder;
    divider = divider >> OFFSET_WIDTH;
  }
}

void empty_frame (word_t frameIndex)
{
  address frameAddr = frameIndex * PAGE_SIZE;
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
  address frameAddr = startFrame * PAGE_SIZE;
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
      if (res == -2) {
		  return -2;
      }
      if (res == -1)
      {
        PMwrite (frameAddr + i, 0);
        return -2;
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

int calculatePage(int page, int i, int level) {
  return page + i * pow(2, OFFSET_WIDTH * level);

}

void maxPageNumber(int level, int  page_swapped_in,int page, word_t
frame, address  parent,SwitchFrame *
frameToP) {
  if (level == TABLES_DEPTH) {
	  frameToP[frame].pageInd = page;
	  frameToP[frame].frameInd = frame;
	  frameToP[frame].parent = parent;
  } else {
    for (int i =0; i < PAGE_SIZE; i++) {
		word_t newFrame;
		PMread ( frame * PAGE_SIZE + i, &newFrame);
		if (newFrame != 0) {
			maxPageNumber( level +1, page_swapped_in, calculatePage(page, i,
														 level),
						   newFrame, frame * PAGE_SIZE + i ,frameToP);
		}
    }
  }
}


bool inPrevFrames(unsigned int
			   *frames, int frame) {
  for (int i =0; i < TABLES_DEPTH; i++) {
    if (frames[i] == frame) {
		return true;
    }
  }
  return false;
}
void find_frame_to_switch(SwitchFrame *newFrame, int page_swapped_in,
						 unsigned int* frames) {
  SwitchFrame frameToPage[NUM_FRAMES];
  maxPageNumber(0, page_swapped_in, 0, 0, 0, frameToPage);
  int maxFrame ;
  int maxPage = -1;
  for (int i = 1; i < NUM_FRAMES; i++) {
	  int p = frameToPage[i].pageInd;
	  bool isBigger = std::min ((long long) std::abs (p - page_swapped_in),NUM_PAGES -
																		   std::abs (p -
																					 page_swapped_in)) > maxPage;
	  if (isBigger && !inPrevFrames(frames, i)) {
		  maxFrame = i;
		  maxPage = p;
	  }

	}

  newFrame->frameInd = maxFrame;
  newFrame->pageInd = maxPage;
  newFrame->parent = frameToPage[maxFrame].parent;

}


void empty_table_handler (int *addr, address pAddr, bool cleanFrame, unsigned
int * frames, unsigned int page_swapped_in)
{
  int newFrame;
  int res = find_empty_frame ((int) (pAddr / PAGE_SIZE), 0, 0, &newFrame);
  if (newFrame == NUM_FRAMES)
  {
    SwitchFrame switch_frame;
    find_frame_to_switch (&switch_frame, page_swapped_in, frames);
	PMevict(switch_frame.frameInd, switch_frame.pageInd);
	PMwrite (switch_frame.parent, 0);
	if (!cleanFrame) {
	  PMrestore(switch_frame.frameInd, page_swapped_in);
	}
	newFrame = switch_frame.frameInd;
  }

  if (res != -1 && cleanFrame)
  {
    empty_frame (newFrame);
  }
  assert(newFrame < 64);

  PMwrite (pAddr, newFrame);
  *addr = newFrame;
}


/**
 * function extract from the page Table tree the physical address of the given
 * virtual Address.
 * @note includes test prints
 * @param virtualAddress
 * @param pageAddress
 * @return 0 if fails, 1 else
 */
int get_physical_address (address virtualAddress, address *physicalAddress, rw_t mode)
{
  unsigned int translated[TABLES_DEPTH + 1];
  unsigned int frames[TABLES_DEPTH];

  translate (virtualAddress, translated);
  for (auto i: translated)
  {
//    std::cout << i << " ";
  }
//  std::cout << std::endl;
  address pAddr;
  int frameIndex = 0;
  for (int i = 0; i < TABLES_DEPTH; i++)
  {
    pAddr = frameIndex * PAGE_SIZE + translated[i];
    PMread (pAddr, &frameIndex);

    if (frameIndex == 0)
    {
      bool cleanFrame = i < TABLES_DEPTH - 1;
      try {
		  empty_table_handler (&frameIndex, pAddr, cleanFrame, frames,
							   virtualAddress / PAGE_SIZE);
      } catch (std::exception &e) {
		  return 0;
      }
    }
	frames[i] = frameIndex;
	check_ram ();
  }
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
int VMread (address virtualAddress, word_t *value)
{
  try
  {
    assert(virtualAddress < VIRTUAL_MEMORY_SIZE );
    address physicalAddress;
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
int VMwrite (address virtualAddress, word_t value)
{
  try
  {
	assert(virtualAddress < VIRTUAL_MEMORY_SIZE );
	address physicalAddress;
    assert(get_physical_address (virtualAddress, &physicalAddress, WRITE));
    PMwrite (physicalAddress, value);
  }
  catch (std::exception &exception)
  {
    return 0;
  }
  return 1;
}


