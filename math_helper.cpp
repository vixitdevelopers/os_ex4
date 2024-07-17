//
// Created by dani on 7/17/24.
//

#include "math_helper.h"
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

uint64_t absolute_diff (uint64_t a, uint64_t b)
{
  if (a > b)
  {
    return a - b;
  }
  return b - a;

}
uint64_t getOffset (uint64_t address)
{
  return address & ((1LL << OFFSET_WIDTH) - 1LL);
}
uint64_t getPageNum (uint64_t address)
{
  return address >> OFFSET_WIDTH;
}

uint64_t extractBitsAtLevel (uint64_t address, uint64_t level)
{
  return (address >> (OFFSET_WIDTH * level)) &
         ((1LL << OFFSET_WIDTH) - 1LL);
}
uint64_t setBitsAtLevel (uint64_t page, uint64_t row, uint64_t level)
{
  return page | (row << (OFFSET_WIDTH * (TABLES_DEPTH -
                                         level)));
}