#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "Frame.h"
#include "math_helper.cpp"

#define SUCCESS 1
#define FAILURE 0
uint64_t min (uint64_t a, uint64_t b)
{
  if (a < b)
  {
    return a;
  }
  return b;
}
uint64_t abs (int a)
{
  if (a < 0)
  {
    return -a;
  }
  return a;
}

word_t search_empty_frame (word_t frame, int level, word_t *max_frame_index)
{
  if (level == TABLES_DEPTH)
  {
    return 0;
  }

  bool is_empty = true;
  word_t value;
  for (int i = 0; i < PAGE_SIZE; ++i)
  {
    PMread ((frame * PAGE_SIZE) + i, &value);
    if (value > *max_frame_index)
    {
      *max_frame_index = value;
    }
    if (value != 0)
    {
      is_empty = false;
      if (value != frame)
      {
        word_t k = search_empty_frame (value, level + 1, max_frame_index);
        if (k != 0)
        {
          if (k == value)
          {
            PMwrite ((frame * PAGE_SIZE) + i, 0);
          }
          return k;
        }
      }
    }
  }
  if (is_empty)
  {
    return frame;
  }
  else
  {
    return 0;
  }
}
//word_t searchEmptyFrame (word_t *max_frame_index)
//{
//  word_t value;
//  for (word_t frame = 0; frame < NUM_FRAMES; ++frame)
//  {
//    bool is_empty = true;
//    for (int i = 0; i < PAGE_SIZE; ++i)
//    {
//      PMread ((frame * PAGE_SIZE) + i, &value);
//      if (value > *max_frame_index)
//      {
//        *max_frame_index = value;
//      }
//      if (value != 0)
//      {
//        is_empty = false;
//        if (value == frame)
//        {
//          break;
//        }
//      }
//    }
//    if (is_empty)
//    {
//      return frame;
//    }
//  }
//  return 0;
//}

Frame swap_out_frame (word_t frame, int level, uint64_t page_swapped_in,
                      uint64_t cur_page, uint64_t parent_address)
{
  if (level == TABLES_DEPTH)
  {
    cur_page = cur_page / PAGE_SIZE;
    uint64_t cyclical_distance = min (abs (((int) page_swapped_in) -
                                           (int) cur_page),
                                      NUM_PAGES
                                      - abs ((int) page_swapped_in -
                                             (int)
                                                 cur_page));
    return *new Frame (frame, cur_page, cyclical_distance, parent_address);
  }
  Frame info = Frame (0, 0, 0, 0);
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    word_t value;
    PMread ((frame * PAGE_SIZE) + i, &value);
    if (value != 0)
    {
      if (value != frame)
      {
        uint64_t new_page = cur_page | (i << (OFFSET_WIDTH * (TABLES_DEPTH -
                                                              level)));
        Frame new_info = swap_out_frame (value, level + 1,
                                         page_swapped_in, new_page,
                                         (frame * PAGE_SIZE) + i);
        if (new_info.get_frame () != 0
            && new_info.get_cyclical_distance () > info
                .get_cyclical_distance ())
        {
          info = new_info;
        }
      }
    }
  }
  return info;
}

void VMinitialize ()
{
  for (int i = 0; i < PAGE_SIZE; i++)
  {
    word_t value = 0;
    PMwrite (i, value);
  }
}

uint64_t getPhysicalAddress (uint64_t virtualAddress)
{
  uint64_t offset;

  // Extract the page number and offset from the virtual address

  offset = virtualAddress & ((1LL << OFFSET_WIDTH) - 1LL);

  uint64_t page_number = virtualAddress >> OFFSET_WIDTH;

  // Start with the root page table in frame 0
  word_t frame = 0;

  // Traverse the page tables
  for (int level = TABLES_DEPTH; level > 0; --level)
  {
    uint64_t row_number = (virtualAddress >> (OFFSET_WIDTH * level)) &
                          ((1LL << OFFSET_WIDTH) - 1LL);
    uint64_t entry_address = (frame * PAGE_SIZE) + row_number;
    word_t prev_frame = frame;
    PMread (entry_address, &frame);

    // If the frame index is 0, it means a page fault occurred
    if (frame == 0)
    {
      PMwrite (entry_address, prev_frame);
      word_t max_frame_index = 0;
      word_t empty_frame = search_empty_frame (0, 0, &max_frame_index);
      if (empty_frame == 0)
      {
        empty_frame = max_frame_index + 1;
      }
      if (empty_frame >= NUM_FRAMES)
      {
        Frame info = swap_out_frame (0, 0, page_number, 0, 0);
        PMevict (info.get_frame (), info.get_page ());
        PMwrite (info.get_parent_address (), 0);
        empty_frame = info.get_frame ();
      }
      PMwrite (entry_address, empty_frame);
      frame = empty_frame;
      if (level == 1)
      {
        PMrestore (frame, page_number);
      }
      else
      {
        for (int i = 0; i < PAGE_SIZE; i++)
        {
          PMwrite ((frame * PAGE_SIZE) + i, 0);
        }
      }
    }
  }
  return (frame * PAGE_SIZE) + offset;
}

int VMread (uint64_t virtualAddress, word_t *value)
{
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
  {
    return FAILURE;
  }
  uint64_t physicalAddress = getPhysicalAddress (virtualAddress);
  PMread (physicalAddress, value);
  return SUCCESS;
}

int VMwrite (uint64_t virtualAddress, word_t value)
{

  if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
  {
    return FAILURE;
  }
  uint64_t physicalAddress = getPhysicalAddress (virtualAddress);
  PMwrite (physicalAddress, value);
  return SUCCESS;
}


//
//#include "VirtualMemory.h"
//#include "PhysicalMemory.h"
//
///*
// * Initialize the virtual memory.
// */
//void VMinitialize ()
//{
//  for (int i = 0; i < PAGE_SIZE; i++)
//  {
//    word_t value = 0;
//    PMwrite (i, value);
//  }
//}
////1. A frame containing an empty table – where all rows are 0. We don’t have to evict it, but we do have
////    to remove the reference to this table from its parent.
////2. An unused frame – When traversing the tree, keep a variable with the maximal frame Index referenced from any table we visit. Since we fill frames in order, all used frames are contiguous in
////    the memory, and if max_frame_index+1 < NUM_FRAMES then we know that the frame in the index
////(max_frame_index + 1) is unused.
//word_t search_empty_frame (word_t frame, int level, word_t *max_frame_index)
//{
//  if (level == TABLES_DEPTH)
//  {
//    return 0;
//  }
//
//  bool is_empty = true;
//  word_t value;
//  for (int i = 0; i < PAGE_SIZE; ++i)
//  {
//    PMread ((frame * PAGE_SIZE) + i, &value);
//    if (value > *max_frame_index)
//    {
//      *max_frame_index = value;
//    }
//    if (value != 0)
//    {
//      is_empty = false;
//      if (value != frame)
//      {
//        word_t k = search_empty_frame (value, level + 1, max_frame_index);
//        if (k != 0)
//        {
//          if (k == value)
//          {
//            PMwrite ((frame * PAGE_SIZE) + i, 0);
//          }
//          return k;
//        }
//      }
//    }
//  }
//  if (is_empty)
//  {
//    return frame;
//  }
//  else
//  {
//    return 0;
//  }
//}
//
//struct frame_info
//{
//    word_t frame;
//    uint64_t page;
//    uint64_t cyclical_distance;
//    uint64_t parent_address;
//};
//uint64_t min (uint64_t a, uint64_t b)
//{
//  if (a < b)
//  {
//    return a;
//  }
//  return b;
//}
//uint64_t abs (uint64_t a)
//{
//  if (a < 0)
//  {
//    return -a;
//  }
//  return a;
//}
//
//
////If all frames are already used, then a page must be swapped out from some
//// frame in order to replace it with the relevant page (a table or actual page)
//// . The frame that will be chosen is the frame
////    containing a page p such that the cyclical distance: min{NUM_PAGES - |page_swapped_in - p|,
////|page_swapped_in - p|} is maximal (don’t worry about tie-breaking).
//// This page must be swapped
////    out before we use the frame, and we also have
////    to remove the reference to this page from its parent
////use this function:
////void PMevict(uint64_t frameIndex, uint64_t evictedPageIndex);
//
//frame_info swap_out_frame (word_t frame, int level, uint64_t page_swapped_in,
//                           uint64_t cur_page, uint64_t parent_address)
//{
//  if (level == TABLES_DEPTH)
//  {
//    cur_page = cur_page / PAGE_SIZE;
//    uint64_t cyclical_distance = min (abs (page_swapped_in - cur_page),
//                                      NUM_PAGES
//                                      - abs (page_swapped_in - cur_page));
//
//    return frame_info{frame, cur_page, cyclical_distance, parent_address};
//  }
//  frame_info info = {0, 0, 0, 0};
//  for (uint64_t i = 0; i < PAGE_SIZE; i++)
//  {
//    word_t value;
//    PMread ((frame * PAGE_SIZE) + i, &value);
//    if (value != 0)
//    {
//      if (value != frame)
//      {
//        uint64_t new_page = cur_page | (i << (OFFSET_WIDTH * (TABLES_DEPTH -
//                                                              level)));
//        frame_info new_info = swap_out_frame (value, level + 1,
//                                              page_swapped_in, new_page,
//                                              (frame * PAGE_SIZE) + i);
//        if (new_info.frame != 0 && new_info.cyclical_distance > info
//            .cyclical_distance)
//        {
//          info = new_info;
//        }
//      }
//    }
//  }
//  return info;
//}
//
///* Reads a word from the given virtual address
// * and puts its content in *value.
// *
// * returns 1 on success.
// * returns 0 on failure (if the address cannot be mapped to a physical
// * address for any reason)
// */
//int VMread (uint64_t virtualAddress, word_t *value)
//{
//  if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
//  {
//    return 0;
//  }
//  word_t frame;
//  uint64_t offset;
//
//  // Extract the page number and offset from the virtual address
//
//  offset = virtualAddress & ((1LL << OFFSET_WIDTH) - 1LL);
//  uint64_t page_number = virtualAddress >> OFFSET_WIDTH;
//
//  // Start with the root page table in frame 0
//  frame = 0;
//
//  // Traverse the page tables
//  for (int level = TABLES_DEPTH; level > 0; level--)
//  {
//    uint64_t row_number = (virtualAddress >> (OFFSET_WIDTH * level)) &
//                          ((1LL << OFFSET_WIDTH) - 1);
//    uint64_t entry_address = (frame * PAGE_SIZE) + row_number;
//    word_t prev_frame = frame;
//    PMread (entry_address, &frame);
//
//    // If the frame index is 0, it means a page fault occurred
//    if (frame == 0)
//    {
//      PMwrite (entry_address, prev_frame); //ASK CHEN
//      word_t max_frame_index = 0;
//      word_t empty_frame = search_empty_frame (0, 0,
//                                               &max_frame_index);
//      if (empty_frame == 0)
//      {
//        empty_frame = max_frame_index + 1;
//      }
//      if (empty_frame >= NUM_FRAMES)
//      {
//        frame_info info = swap_out_frame (0, 0, page_number, 0, 0);
//        PMevict (info.frame, info.page);
//        PMwrite (info.parent_address, 0);
//        empty_frame = info.frame;
//      }
//      PMwrite (entry_address, empty_frame);
//      frame = empty_frame;
//      if (level == 1)
//      {
//        PMrestore (frame, page_number);
//      }
//      else
//      {
//        for (int i = 0; i < PAGE_SIZE; i++)
//        {
//          PMwrite ((frame * PAGE_SIZE) + i, 0);
//        }
//      }
//    }
//  }
//  // Read the value from the physical memory
//  PMread ((frame * PAGE_SIZE) + offset, value);
//
//  return 1; // Return 1 to indicate success
//}
//
///* Writes a word to the given virtual address.
// *
// * returns 1 on success.
// * returns 0 on failure (if the address cannot be mapped to a physical
// * address for any reason)
// */
//int VMwrite (uint64_t virtualAddress, word_t value)
//{
//  if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
//  {
//    return 0;
//  }
//  word_t frame;
//  uint64_t offset;
//
//  // Extract the page number and offset from the virtual address
//
//  offset = virtualAddress & ((1LL << OFFSET_WIDTH) - 1LL);
//
//  uint64_t page_number = virtualAddress >> OFFSET_WIDTH;
//
//  // Start with the root page table in frame 0
//  frame = 0;
//
//  // Traverse the page tables
//  for (int level = TABLES_DEPTH; level > 0; --level)
//  {
//    uint64_t row_number = (virtualAddress >> (OFFSET_WIDTH * level)) &
//                          ((1LL << OFFSET_WIDTH) - 1LL);
//    uint64_t entry_address = (frame * PAGE_SIZE) + row_number;
//    word_t prev_frame = frame;
//    PMread (entry_address, &frame);
//
//    // If the frame index is 0, it means a page fault occurred
//    if (frame == 0)
//    {
//      PMwrite (entry_address, prev_frame);
//      word_t max_frame_index = 0;
//      word_t empty_frame = search_empty_frame (0, 0,
//                                               &max_frame_index);
//      if (empty_frame == 0)
//      {
//        empty_frame = max_frame_index + 1;
//      }
//      if (empty_frame >= NUM_FRAMES)
//      {
//        frame_info info = swap_out_frame (0, 0, page_number, 0, 0);
//        word_t check;
//        PMread (info.parent_address, &check);
//
//        PMevict (info.frame, info.page);
//        PMwrite (info.parent_address, 0);
//        empty_frame = info.frame;
//      }
//      PMwrite (entry_address, empty_frame);
//      frame = empty_frame;
//      if (level == 1)
//      {
//        PMrestore (frame, page_number);
//      }
//      else
//      {
//        for (int i = 0; i < PAGE_SIZE; i++)
//        {
//          PMwrite ((frame * PAGE_SIZE) + i, 0);
//        }
//      }
//    }
//  }
//
//  // Read the value from the physical memory
//  PMwrite ((frame * PAGE_SIZE) + offset, value);
//  return 1; // Return 1 to indicate success
//}
//
//
//
