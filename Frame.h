//
// Created by dani on 7/17/24.
//

#ifndef _FRAME_H_
#define _FRAME_H_

#include "PhysicalMemory.h"

class Frame
{
  word_t index;
  uint64_t pageNumber;
  uint64_t dist_from_other_cyc;
  uint64_t parent;

 public:
  Frame (word_t myframe, uint64_t myPage, uint64_t my_cyclical_distance, uint64_t my_parent_address)
      : index (myframe), pageNumber (myPage), dist_from_other_cyc (my_cyclical_distance), parent (my_parent_address)
  {}
  Frame ()
      : index (0), pageNumber (0), dist_from_other_cyc (0), parent (0)
  {}
  word_t get_index () const
  { return index; }
  void set_index (word_t myframe)
  { index = myframe; }
  uint64_t get_page_num () const
  { return pageNumber; }
  void set_page_num (uint64_t myPage)
  { pageNumber = myPage; }
  uint64_t get_dist_from_other_cyc () const
  {
    return dist_from_other_cyc;
  }
  void set_dist_from_other_cyc (uint64_t my_cyclical_distance)
  { dist_from_other_cyc = my_cyclical_distance; }
  uint64_t get_parent_address () const
  {
    return parent;
  }
  void set_parent_address (uint64_t my_parent_address)
  {
    parent = my_parent_address;
  }
  bool isValid ()
  {
    return index != 0;
  }
  uint64_t getEntryAdress (uint64_t row) const
  {
    return index * PAGE_SIZE + row;
  }
  void setFrameByPhysical (uint64_t entry_address)
  {
    word_t curr;
    PMread (entry_address, &curr);
    set_index (curr);
  }
};

#endif //_FRAME_H_
