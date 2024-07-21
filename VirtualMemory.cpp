#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "Frame.h"
#include "math_helper.cpp"

#define SUCCESS 1
#define FAILURE 0
#define FRAME_INDEX(frame, i) ((frame) * PAGE_SIZE + (i))
#define NO_FRAME_VALUE 0

Frame search_empty_frame (word_t frame, int level, word_t *max_frame_index)
{
  if (level == TABLES_DEPTH)
  {
    return Frame (); // return a default Frame instance
  }

  bool is_empty = true;
  word_t value;
  for (int i = 0; i < PAGE_SIZE; ++i)
  {
    PMread (FRAME_INDEX(frame, i), &value);
    if (value > *max_frame_index)
    {
      *max_frame_index = value;
    }
    if (value != 0)
    {
      is_empty = false;
      if (value != frame)
      {
        Frame k = search_empty_frame (value, level + 1, max_frame_index);
        if (k.get_frame () != 0)
        {
          if (k.get_frame () == value)
          {
            PMwrite (FRAME_INDEX(frame, i), 0);
          }
          return k;
        }
      }
    }
  }
  if (is_empty)
  {
    return Frame (frame, 0, 0, 0); // return a Frame instance with the found frame
  }
  else
  {
    return Frame (); // return a default Frame instance
  }
}

Frame swap_out_frame (word_t frame, int level, uint64_t page_swapped_in,
                      uint64_t cur_page, uint64_t parent_address)
{
  if (level == TABLES_DEPTH)
  {
    cur_page = cur_page / PAGE_SIZE;
    uint64_t diff = absolute_diff (page_swapped_in, cur_page);
    uint64_t cyclical_distance = min (diff, NUM_PAGES - diff);
    return *new Frame (frame, cur_page, cyclical_distance, parent_address);
  }
  Frame info = Frame (0, 0, 0, 0);
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    word_t value;
    PMread (FRAME_INDEX(frame, i), &value);
    if (value != 0)
    {
      if (value != frame)
      {
        uint64_t new_page = setBitsAtLevel (cur_page, i, level);
        Frame new_info = swap_out_frame (value, level + 1,
                                         page_swapped_in, new_page,
                                         FRAME_INDEX(frame, i));
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

  uint64_t offset = getOffset (virtualAddress);
  uint64_t page_number = getPageNum (virtualAddress);

  // Start with the root page table in frame 0
//  word_t frame = 0;

  Frame frame = Frame ();

  // Traverse the page tables
  for (int level = TABLES_DEPTH; level > 0; --level)
  {
    uint64_t row_number = extractBitsAtLevel (virtualAddress, level);
    uint64_t entry_address = FRAME_INDEX(frame.get_frame (), row_number);
    word_t previousFrame = frame.get_frame ();
    word_t curr;
    PMread (entry_address, &curr);
    frame.set_frame (curr);
    if (frame.get_frame () == NO_FRAME_VALUE)
    {
      PMwrite (entry_address, previousFrame);
      word_t max_frame_index = 0;

//      word_t empty_frame = search_empty_frame (0, 0, &max_frame_index);
      Frame empty_frame = search_empty_frame (0, 0, &max_frame_index);
      if (empty_frame.get_frame () == 0)
      {
        empty_frame.set_frame (max_frame_index + 1);
      }
      if (empty_frame.get_frame () >= NUM_FRAMES)
      {
        Frame info = swap_out_frame (0, 0, page_number, 0, 0);
        PMevict (info.get_frame (), info.get_page ());
        PMwrite (info.get_parent_address (), 0);
        empty_frame.set_frame (info.get_frame ());
      }
      PMwrite (entry_address, empty_frame.get_frame ());
      frame = empty_frame;
      if (level == 1)
      {
        PMrestore (frame.get_frame (), page_number);
      }
      else
      {
        for (int i = 0; i < PAGE_SIZE; i++)
        {
          PMwrite (FRAME_INDEX(frame.get_frame (), i), 0);
        }
      }
    }
  }
  return FRAME_INDEX(frame.get_frame (), offset);
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

