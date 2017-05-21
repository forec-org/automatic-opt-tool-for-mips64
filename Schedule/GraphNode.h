#pragma once

#include "stdafx.h"
#include "instr.h"

struct GraphNode
{
	vector<Instr> instrs;
	int first = 0;

	void swap_to_front(const Instr &instr)
	{
		for (int i = 0; i < instrs.size(); ++i)
		{
			if (instrs.at(i).index == instr.index)
			{
				Instr to_insert = instrs.at(i);
				instrs.erase(instrs.begin() + i);
				instrs.insert(instrs.begin() + first++, to_insert);
				if (first == instrs.size())
					first--;
				return;
			}			
		}
	}

	void swap_to_back(const Instr &instr)
	{
		for (int i = 0; i < instrs.size(); ++i)
		{
			if (instrs.at(i).index == instr.index)
			{
				Instr to_insert = instrs.at(i);
				instrs.erase(instrs.begin() + i);
				instrs.push_back(to_insert);
				return;
			}
		}
	}
};