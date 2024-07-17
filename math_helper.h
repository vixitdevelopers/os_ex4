//
// Created by dani on 7/17/24.
//

#ifndef _MATH_HELPER_H_
#define _MATH_HELPER_H_
#include "PhysicalMemory.h"

uint64_t min (uint64_t a, uint64_t b);
uint64_t abs (int a);
uint64_t getOffset(uint64_t address);
uint64_t getPageNum (uint64_t address);
#endif //_MATH_HELPER_H_
