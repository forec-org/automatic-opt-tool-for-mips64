"""
created by forec
2017.5.14
An optimizor for MIPS64 codes
"""

import re
import os
import sys
import codecs
from optimizer import *

def rename_register(source, register_used):
    """
    rename registers to avoid WAW/WAR conflictions
    """
    registers_refer = [-1 for i in range(32)]
    register_map = {}
    available_registers = list(set(range(1, 30)).difference(register_used))
    available_registers.sort()
    for i in range(len(source)-1, -1, -1):
        (op_code, opns, opn_list, _) = source[i]
        # print source[i]
        for wd_reg, target_register in enumerate(opn_list):
            if target_register == '$r0' or target_register == '$r31':
                continue
            if len(available_registers) == 0:
                continue
            pre_refer_line = registers_refer[int(target_register[2:])]
            if wd_reg == 0 and op_code.upper() in LD_OPS:
                registers_refer[int(target_register[2:])] = i
            if pre_refer_line == -1:
                continue
            if pre_refer_line == -1 or pre_refer_line - i > 3:
                continue
            cycle_sub = LD_CYCN[op_code.upper()] - LD_CYCN[source[pre_refer_line][0].upper()]
            if cycle_sub != 0 and cycle_sub <= pre_refer_line - i:
                can_rename = True
                pre_opn_list = source[pre_refer_line][2]
                for opnr in pre_opn_list[1:]:
                    if opnr == target_register:
                        can_rename = False
                        break
                if not can_rename:
                    continue
                available_register = available_registers[0]
                available_registers = available_registers[1:]
                register_used.add(available_register)
                if register_map.get(target_register) is None:
                    register_map[target_register] = []
                register_map[target_register].append('$r' + str(available_register))
                for j in range(pre_refer_line, len(source)):
                    (op_rep, opns_rep, opn_list_rep, dup) = source[j]
                    for k, _ in enumerate(opns_rep):
                        if opns_rep[k] == target_register or opns_rep[k][-(1+ len(target_register)):-1] == target_register:
                            opns_rep[k] = opns_rep[k].replace(target_register, '$r' + str(available_register))
                    for k, _ in enumerate(opn_list_rep):
                        if opn_list_rep[k] == target_register:
                            opn_list_rep[k] = '$r' + str(available_register)
                    source[j] = (op_rep, opns_rep, opn_list_rep, dup)
    return source, register_used, register_map


if __name__ == '__main__':
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print "Usage: python2 optimizor.py <source.s> [target.s]"
        exit(0)
    source_path_str = str(sys.argv[1])
    target_path_str = str(os.path.splitext(source_path_str)[0]) + "_opt.s"
    if len(sys.argv) == 3:
        target_path_str = str(sys.argv[2])
    source, other_code, register_used, duplicate_segment, rl = parse_source(source_path_str)
    source, _, register_map = rename_register(source, register_used)
    # source = static_dispatcher(source, duplicate_segment, rl)
    print_source(source, other_code, duplicate_segment, register_map, target_path_str)