// BlockSchedule.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "instr.h"
#include "Segment.h"
#include "GraphNode.h"

const static int MAX_INST_NUM		= 32;
const static int REG_NUM			= 32;
const static int MAX_SEG_NUM		= 32;

bool conflict_matrix[MAX_INST_NUM][MAX_INST_NUM];
int pre_referred_dict[REG_NUM];

vector<Instr> source;

vector<Segment> segments;

ofstream output_file;

void init();
void load_file();									//载入程序
void make_conflict_matrix();						//计算相关矩阵
void split_segment();								//对基本块划分段
void optimization();								//对每个段进行优化
void build_file(const vector<GraphNode> &graph);	//将优化后的代码写入文件
void clean();

int main()
{
	init();
	load_file();
	optimization();
	clean();
    return 0;
}

void init()
{
	memset(conflict_matrix, (int)false, sizeof(conflict_matrix));
	for (int index = 1; index <= 30; ++index)
	{
		pre_referred_dict[index] = -1;
	}

	output_file.open("after_schedule.s", ios_base::ate);

}

void make_conflict_matrix()
{
	for (int instr_index = source.size() - 1; instr_index >= 0; --instr_index)
	{
		Instr instr = source[instr_index];

		if (instr.has_target_reg && 
			pre_referred_dict[instr.target_reg.reg_num] != -1 &&
			instr_index - pre_referred_dict[instr.target_reg.reg_num] < 2)
		{
			conflict_matrix[instr_index][pre_referred_dict[instr.target_reg.reg_num]] = true;
		}

		while (instr.has_next_reg())
		{
			Operand op = instr.next_reg();
			if (op.reg_num == 0 || op.reg_num == 31)
				continue;
			pre_referred_dict[op.reg_num] = instr_index;
		}
	}
}

void split_segment()
{
	int i = 0;
	while (i * SEG_INSTR_NUM + SEG_INSTR_NUM < source.size())
	{
		Segment new_seg;
		new_seg.start_index = i * SEG_INSTR_NUM;
		new_seg.end_index = i * SEG_INSTR_NUM + SEG_INSTR_NUM - 1;
		new_seg.size = SEG_INSTR_NUM;
		i++;
		segments.push_back(new_seg);
	}
	if (i * SEG_INSTR_NUM < source.size() - 1)
	{
		Segment new_seg;
		new_seg.start_index = i * SEG_INSTR_NUM;
		new_seg.end_index = source.size() - 2;
		new_seg.size = source.size() - i * SEG_INSTR_NUM - 1;
		segments.push_back(new_seg);
	}
}

void optimization()
{
	make_conflict_matrix();

	split_segment();

	for (int seg_index = 0; seg_index < segments.size(); ++seg_index)
	{
		bool dependency_matrix[SEG_INSTR_NUM][SEG_INSTR_NUM] = { false };
		
		Segment curr_segment = segments.at(seg_index);

		//	计算dependency_matrix
		for (int curr_instr_index = 0; curr_instr_index < curr_segment.size; ++curr_instr_index)
		{
			Instr curr_instr = source.at(curr_instr_index + curr_segment.start_index);
			
			if (!curr_instr.has_target_reg || curr_instr.target_reg.reg_num == 0 || curr_instr.target_reg.reg_num == 31)
				continue;

			for (int later_instr_index = curr_instr_index + 1; later_instr_index < curr_segment.size; ++later_instr_index)
			{
				Instr later_instr = source.at(later_instr_index + curr_segment.start_index);
				if (later_instr.has_dependency(curr_instr))
				{
					dependency_matrix[curr_instr_index][later_instr_index] = true;
				}
			}
		}

		//	----BFS
		int degree[SEG_INSTR_NUM] = { 0 };
		bool is_used[SEG_INSTR_NUM] = { false };
		vector<int> zero_in_degree;
		vector<GraphNode> graph;
		vector<int> iso_ist;

		//caculate indegree
		for (int instr_index = 0; instr_index < curr_segment.size; ++instr_index)
		{
			for (int index = 0; index < SEG_INSTR_NUM; ++index)
				if (dependency_matrix[index][instr_index] == true)
					degree[instr_index]++;

			if (degree[instr_index] == 0)
			{
				zero_in_degree.push_back(instr_index);
				is_used[instr_index] = true;
			}

		}

		//topologic sort and group
		while (!zero_in_degree.empty())
		{
			GraphNode node;
			while (!zero_in_degree.empty())
			{
				source.at(zero_in_degree.front() + curr_segment.start_index).node_id = graph.size();
				Instr instr = source[zero_in_degree.front() + curr_segment.start_index];
				node.instrs.push_back(instr);
				zero_in_degree.erase(zero_in_degree.begin());
			}

			for (int i = 0; i < node.instrs.size(); ++i)
			{
				for (int j = i + 1; j < curr_segment.size; ++j)
				{
					if (dependency_matrix[node.instrs.at(i).index - curr_segment.start_index][j] == true)
						degree[j]--;
					if (degree[j] == 0 && !is_used[j])
					{
						zero_in_degree.push_back(j);
						is_used[j] = true;
					}	
				}
			}
			graph.push_back(node);
		}

		//iso_ist
		for (int i = 0; i < curr_segment.size; ++i)
			if (!is_used[i])
				iso_ist.push_back(i);

		//	----schedule
		for (int i = 0; i < curr_segment.size; ++i)
		{
			for (int j = 0; j < curr_segment.size; ++j)
			{
				if (conflict_matrix[i + curr_segment.start_index][j + curr_segment.start_index] == false)
					continue;
				
				Instr Ia = source[i + curr_segment.start_index];
				Instr Ib = source[j + curr_segment.start_index];
				GraphNode Va = graph.at(Ia.node_id);
				GraphNode Vb = graph.at(Ib.node_id);

				Va.swap_to_front(Ia);
				if (!iso_ist.empty())
				{
					Va.instrs.push_back(source[iso_ist.front() + curr_segment.start_index]);	
				}
				Vb.swap_to_back(Ib);
			}
		}

		build_file(graph);
	}
	output_file << source.back().build_str();
}

void build_file(const vector<GraphNode>& graph)
{
	for (auto &node: graph)
		for (auto &instr: node.instrs)	
			output_file << instr.build_str();
}

void clean()
{
	output_file.close();
}

void load_file()
{
	Instr instr;

	instr = Instr("ADDIU", 0);
	instr.set_target_reg(1);
	instr.op_list.push_back(Operand::make_reg(0));
	instr.op_list.emplace_back(Operand::make_var("A"));
	source.push_back(instr);

	instr = Instr("LW", 1);
	instr.set_target_reg(2);
	instr.op_list.push_back(Operand::make_reg_imm(1, 0));
	source.push_back(instr);

	instr = Instr("ADD", 2);
	instr.set_target_reg(4);
	instr.op_list.push_back(Operand::make_reg(0));
	instr.op_list.push_back(Operand::make_reg(2));
	source.push_back(instr);

	instr = Instr("SW", 3);
	instr.op_list.push_back(Operand::make_reg(4));
	instr.op_list.push_back(Operand::make_reg_imm(1, 0));
	source.push_back(instr);

	instr = Instr("LW", 4);
	instr.set_target_reg(6);
	instr.op_list.push_back(Operand::make_reg_imm(1, 4));
	source.push_back(instr);

	instr = Instr("ADD", 5);
	instr.set_target_reg(8);
	instr.op_list.push_back(Operand::make_reg(6));
	instr.op_list.push_back(Operand::make_reg(1));
	source.push_back(instr);

	instr = Instr("MUL", 6);
	instr.set_target_reg(12);
	instr.op_list.push_back(Operand::make_reg(10));
	instr.op_list.push_back(Operand::make_reg(1));
	source.push_back(instr);

	instr = Instr("ADD", 7);
	instr.set_target_reg(16);
	instr.op_list.push_back(Operand::make_reg(12));
	instr.op_list.push_back(Operand::make_reg(1));
	source.push_back(instr);

	instr = Instr("ADD", 8);
	instr.set_target_reg(18);
	instr.op_list.push_back(Operand::make_reg(16));
	instr.op_list.push_back(Operand::make_reg(1));
	source.push_back(instr);

	instr = Instr("SW", 9);
	instr.op_list.push_back(Operand::make_reg(18));
	instr.op_list.push_back(Operand::make_reg_imm(1, 16));
	source.push_back(instr);

	instr = Instr("LW", 10);
	instr.set_target_reg(20);
	instr.op_list.push_back(Operand::make_reg_imm(1, 8));
	source.push_back(instr);

	instr = Instr("MUL", 11);
	instr.set_target_reg(22);
	instr.op_list.push_back(Operand::make_reg(20));
	instr.op_list.push_back(Operand::make_reg(14));
	source.push_back(instr);

	instr = Instr("MUL", 12);
	instr.set_target_reg(24);
	instr.op_list.push_back(Operand::make_reg(26));
	instr.op_list.push_back(Operand::make_reg(14));
	source.push_back(instr);

	instr = Instr("TEQ", 13);
	instr.op_list.push_back(Operand::make_reg(0));
	instr.op_list.push_back(Operand::make_reg(0));
	source.push_back(instr);
}