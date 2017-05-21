#pragma once

#include "stdafx.h"
#include "instr.h"

const static int SEG_INSTR_NUM = 16;

struct Segment
{
	int start_index;
	int end_index;
	int size;
};
