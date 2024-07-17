//
// Created by dani on 7/17/24.
//

#ifndef _FRAME_H_
#define _FRAME_H_

#include "PhysicalMemory.h"

class Frame
{
  word_t frame;
  uint64_t page;
  uint64_t cyclical_distance;
  uint64_t parent_address;

 public:
  Frame (word_t myframe, uint64_t myPage, uint64_t my_cyclical_distance, uint64_t my_parent_address)
      : frame (myframe), page (myPage), cyclical_distance (my_cyclical_distance), parent_address (my_parent_address)
  {}
  word_t get_frame () const
  { return frame; }
  void set_frame (word_t myframe)
  { frame = myframe; }
  uint64_t get_page () const
  { return page; }
  void set_page (uint64_t myPage)
  { page = myPage; }
  uint64_t get_cyclical_distance () const
  {
    return cyclical_distance;
  }
  void set_cyclical_distance (uint64_t my_cyclical_distance)
  { cyclical_distance = my_cyclical_distance; }
  uint64_t get_parent_address () const
  {
    return parent_address;
  }
  void set_parent_address (uint64_t my_parent_address)
  {
    parent_address = my_parent_address;
  }
};

#endif //_FRAME_H_
