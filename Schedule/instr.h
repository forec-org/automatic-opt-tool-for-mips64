#pragma once

#include "stdafx.h"

enum OpType						//操作数类型
{
	REG,
	IMM,
	UIMM,
	VAR,
	REG_IMM,
};

struct Operand
{
	OpType type;
	
	int reg_num;			//寄存器号
	string ident;			//变量名
	int imm;				//立即数
	unsigned int uimm;		//无符号立即数

	static Operand make_reg(int reg_num)
	{
		Operand op;
		op.type = REG;
		op.reg_num = reg_num;
		return op;
	}

	static Operand make_var(const string &var_name)
	{
		Operand op;
		op.type = VAR;
		op.ident = var_name;
		return op;
	}

	static Operand make_imm(int imm)
	{
		Operand op;
		op.type = IMM;
		op.imm = imm;
		return op;
	}

	static Operand make_uimm(unsigned int uimm)
	{
		Operand op;
		op.type = UIMM;
		op.uimm = uimm;
		return op;
	}

	static Operand make_reg_imm(int reg, int imm)
	{
		Operand op;
		op.type = REG_IMM;
		op.reg_num = reg;
		op.imm = imm;
		return op;
	}

	bool isReg()
	{
		return type == OpType::REG || type == OpType::REG_IMM;
	}

	string build() const
	{
		stringstream op_str;
		switch (type)
		{
		case REG:
			op_str << "$r" << reg_num;
			break;
		case IMM:
			op_str << imm;
			break;
		case UIMM:
			op_str << uimm;
			break;
		case VAR:
			op_str << ident;
			break;
		case REG_IMM:
			op_str << imm << "($r" << reg_num << ")";
			break;
		}
		return op_str.str();
	}
};

struct Instr					//指令结构体
{
	int index;

	Operand target_reg;			//目标寄存器
	bool has_target_reg;	

	string mnemonic;			//指令助记符
	vector<Operand> op_list;	//操作数

	int node_id;				//所在的节点号

	int curr_op_index = -1;

	Instr() {}

	Instr(const string &mnemonic, int index)
	{
		this->mnemonic = mnemonic;
		this->index = index;
		has_target_reg = false;
		node_id = -1;
	}

	void set_target_reg(int num)
	{
		target_reg = Operand::make_reg(num);
		has_target_reg = true;
	}

	string build_str() const
	{
		stringstream instr_str;
		instr_str << setfill(' ') << setw(10) << setiosflags(ios::left) << mnemonic;

		if (has_target_reg)
			instr_str << target_reg.build() << ",";

		instr_str << op_list.front().build();
		for (auto op = op_list.begin() + 1; op != op_list.end(); ++op)
		{
			instr_str << "," << op->build();
		}
		instr_str << setfill(' ') << setw(40 - instr_str.tellp()) << setiosflags(ios::right) << "#";
		instr_str << index << "\n";
		return instr_str.str();
	}

	bool has_next_reg()
	{
		for (int i = curr_op_index + 1; i < op_list.size(); ++i)
			if (op_list.at(i).type == REG || op_list.at(i).type == REG_IMM)
			{
				curr_op_index = i;
				return true;
			}				
		return false;
	}

	Operand next_reg()
	{
		return op_list.at(curr_op_index);
	}

	bool has_dependency(const Instr& instr)
	{
		for (int op_index = 0; op_index < op_list.size(); ++op_index)
			if (instr.target_reg.reg_num == op_list.at(op_index).reg_num)
				return true;
		return false;
	}
};
