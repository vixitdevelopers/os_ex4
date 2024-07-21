#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "Frame.h"
#include "math_helper.cpp"

#define SUCCESS 1
#define FAILURE 0
#define FRAME_INDEX(frame, i) ((frame) * PAGE_SIZE + (i))
#define NO_FRAME_VALUE 0

Frame search_empty_frame (word_t frameNum, int depth, word_t *maxFrameNumber)
{
  if (depth == TABLES_DEPTH)
  {
    return {}; // return a default Frame instance
  }
  word_t value;
  bool found = false;
  for (int i = 0; i < PAGE_SIZE; ++i)
  {
    PMread (FRAME_INDEX(frameNum, i), &value);
    if (value > *maxFrameNumber)
    {
      *maxFrameNumber = value;
    }
    if (value != 0)
    {
      found = true;
      if (value != frameNum)
      {
        Frame k = search_empty_frame (value, depth + 1, maxFrameNumber);
        if (k.isValid ())
        {
          if (k.get_index () == value)
          {
            PMwrite (FRAME_INDEX(frameNum, i), 0);
          }
          return k;
        }
      }
    }
  }
  if (!found)
  {
    return {frameNum, 0, 0, 0}; // return a Frame instance with the found word
  }
  else
  {
    return {}; // return a default Frame instance
  }
}

Frame evictFrameToSwap (word_t frame, int level, uint64_t page_swapped_in,
                        uint64_t cur_page, uint64_t parent)
{
  if (level == TABLES_DEPTH)
  {
    cur_page = cur_page / PAGE_SIZE;
    uint64_t diff = absolute_diff (page_swapped_in, cur_page);
    uint64_t dist_from_other_cyc = min (diff, NUM_PAGES - diff);
    return *new Frame (frame, cur_page, dist_from_other_cyc, parent);
  }
  Frame next = Frame (0, 0, 0, 0);
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    word_t value;
    PMread (FRAME_INDEX(frame, i), &value);
    if (value != 0)
    {
      if (value != frame)
      {
        uint64_t new_page = setBitsAtLevel (cur_page, i, level);
        Frame new_info = evictFrameToSwap (value, level + 1,
                                           page_swapped_in, new_page,
                                           FRAME_INDEX(frame, i));
        if (new_info.isValid ()
            && new_info.get_dist_from_other_cyc () > next
                .get_dist_from_other_cyc ())
        {
          next = new_info;
        }
      }
    }
  }
  return next;
}

void VMinitialize ()
{
  for (int i = 0; i < PAGE_SIZE; i++)
  {
    word_t value = 0;
    PMwrite (i, value);
  }
}
Frame getEmptyFrameOrMax ()
{
  word_t max_frame_index;
  Frame empty_frame = search_empty_frame (0, 0, &max_frame_index);
  if (!empty_frame.isValid ())
  {
    empty_frame.set_index (max_frame_index + 1);
  }
  return empty_frame;
}
void swapFrame (Frame &empty_frame, uint64_t pageNum)
{
  Frame frame = evictFrameToSwap (0, 0, pageNum, 0, 0);
  PMevict (frame.get_index (), frame.get_page_num ());
  PMwrite (frame.get_parent_address (), 0);
  empty_frame.set_index (frame.get_index ());
}

uint64_t getPhysicalAddress (uint64_t virtualAddress)
{

  uint64_t offset = getOffset (virtualAddress);
  uint64_t page_number = getPageNum (virtualAddress);

  Frame frame = Frame ();

  for (int d = TABLES_DEPTH; d > 0; --d)
  {
    uint64_t row_number = extractBitsAtLevel (virtualAddress, d);
    uint64_t entry_address = frame.getEntryAdress (row_number);
    word_t previousFrame = frame.get_index ();
    frame.setFrameByPhysical (entry_address);
    if (frame.get_index () == NO_FRAME_VALUE)
    {
      PMwrite (entry_address, previousFrame);
      Frame frameToUse = getEmptyFrameOrMax ();
      if (frameToUse.get_index () >= NUM_FRAMES)
      {
        swapFrame (frameToUse, page_number);
      }
      PMwrite (entry_address, frameToUse.get_index ());
      frame = frameToUse;
      if (d == 1)
      {
        PMrestore (frame.get_index (), page_number);
      }
      else
      {
        for (int i = 0; i < PAGE_SIZE; i++)
        {
          PMwrite (frame.getEntryAdress (i), 0);
        }
      }
    }
  }
  return frame.getEntryAdress (offset);
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

