"""
created by forec
2017.5.14
An optimizor for MIPS64 codes
"""

import re
import os
import sys
import codecs

LD_OPS = ["LW", "ADDIU", "DSUB", "ADDI"]
LD_OPSN = {
    "LW": 2,
    "ADDIU": 3,
    "DSUB": 3,
    "ADDI": 3
}
LD_CYCN = {
    "LW": 4,
    "ADDIU": 5,
    "DSUB": 5,
    "ADDI": 5  
}
JP_OPS = ["B", "BEQZ", "BNEZ", "BEQ", "BNE", "BGEZ", "BGTZ", "BLEZ", "BLTZ", "BGEZAL", "BLTZAL"]
JP_OPSN = {
    "B": 1, 
    "BEQZ": 3, 
    "BNEZ": 3, 
    "BEQ": 3, 
    "BNE": 3, 
    "BGEZ": 2, 
    "BGTZ": 2, 
    "BLEZ": 2, 
    "BLTZ": 2, 
    "BGEZAL": 2, 
    "BLTZAL": 2
}

def parse_source(path):
    """
    read .s code from source file, split into lines
    """
    source = []
    register_used = set([])
    register_re = re.compile(r'\$r\d+')
    other_code = {}
    labels = {}
    duplicate_segment = []
    real_source_length = 0
    if os.path.isfile(path) and os.path.splitext(path)[1] == ".s":
        with codecs.open(path, "r", "utf8") as code:
            for line_num, line in enumerate(code.readlines()):
                line = line.strip("\n").strip("\r")
                line = line.strip()
                if len(line) == 0 or line[0] == '#':
                    continue
                if line[0] == "." or line[-1] == ':':
                    other_code[line_num] = line
                    if line[-1] == ':':
                        if labels.get(line[:-1]) is None:
                            labels[line[:-1]] = (line_num, len(source))
                        else:
                            print ("Syntax error in source file: \n"
                            "duplicate label declaration in line %d (%s), pre-defined in line %d" % 
                            (line_num, line, labels[line[:-1]][0]))
                    continue
                op_code = line.split(' ')[0]
                opnstr = line[len(op_code):].strip()
                opns = [x.strip() for x in opnstr.split(',')]
                opn_list = []
                for opnes in [register_re.findall(opn) for opn in opns]:
                    opnes = [opn for opn in opnes if len(opn) > 2]
                    opn_list.extend(opnes)
                register_used |= set([int(opn[2:]) for opn in opn_list if len(opn) > 2])
                source.append((op_code, opns, opn_list, -1))
                real_source_length += 1

                # validate op command
                if op_code.upper() in LD_OPS:
                    if LD_OPSN[op_code.upper()] != len(opns):
                        print "Invalid syntax: " + op_code + " " + opnstr, \
                              "Expect %d parameters but provide %d." % (LD_OPSN[op_code.upper()], len(opns))
                        exit(0)
                if op_code.upper() in JP_OPS:
                    if JP_OPSN[op_code.upper()] != len(opns):
                        print "Invalid syntax: " + op_code + " " + opnstr, \
                              "Expect %d parameters but provide %d." % (LD_OPSN[op_code.upper()], len(opns))
                        exit(0)
                    # deal with loop
                    label = opns[-1].strip()
                    if label in labels:
                        pre_defined = labels[label][1]
                        duplicate_length = len(source) - pre_defined
                        duplicate_segment.append((len(source), len(source) + duplicate_length))
                        for i in range(duplicate_length):
                            (op_code, opns, opn_list, dup) = source[pre_defined + i]
                            source.append((op_code, opns, opn_list, pre_defined + i))
    else:
        print "Path is not exist or may not end with '.s'"
        exit(0)
    return source, other_code, register_used, duplicate_segment, real_source_length


def static_dispatcher(source, duplicate_segment, real_source_length):
    raw_conflicts = []
    pre_3_wregs = [-1, -1, -1]
    conflict_matrix = [[-1 for i in range(real_source_length)] for j in range(real_source_length)]
    for source_num, (op_code, opns, opn_list, dup) in enumrate(source):
        pre_3_wregs = pre_3_wregs[1:]
        this_round_wreg = -1 if op_code.upper() not in LD_OPS else int(opn_list[0][2:])
        if op_code.upper() in LD_OPS:
            opn_list = opn_list[1:]
        for opn in opn_list:
            register_num = int(opn[2:])
            if register_num in pre_3_wregs:
                index = [x for x in range(3) if pre_3_wregs[x] == register_num][0]
                raw_conflicts.append([source_num - (3-x), source_num, register_num])
                depended_sl = source_num - (3-x) if source[source_num-(3-x)][3] == -1 else source[source_num-(3-x)][3]
                depending_sl = source_num if dup == -1 else dup
                conflict_matrix[depending_sl][depended_sl] = 1
        pre_3_wregs.append(this_round_wreg)


def print_source(source, other_code, duplicate_segment, register_map = None, path='console'):
    """
    print source code into file (on path or console) from structure 'source'
    """
    line_num = 0
    max_op_length = max([len(op_code) for (op_code, _, _, _) in source])
    max_opn_length = max([max([len(opn) for opn in opns]) for (_, opns, _, _) in source]) + 1
    duplicate_index = 0
    if path != 'console':
        with codecs.open(path, "w", "utf8") as code:
            next_line_num_in_source = 0
            while next_line_num_in_source < len(source):
                if duplicate_index < len(duplicate_segment):
                    if duplicate_segment[duplicate_index][0] == next_line_num_in_source:
                        next_line_num_in_source = duplicate_segment[duplicate_index][1]
                        duplicate_index += 1
                if other_code.get(line_num) is not None:
                    code.write(other_code[line_num] + "\n")
                else:
                    (op_code, opns, _, _) = source[next_line_num_in_source]
                    opns_str = ','.join([("%-"+ str(max_opn_length) +"s") % opn for opn in opns])
                    line = (("%-" + str(max_op_length) + "s ") % op_code) + opns_str + "\n"
                    code.write(line)
                    next_line_num_in_source += 1
                line_num += 1
    else:
        next_line_num_in_source = 0
        while next_line_num_in_source < len(source):
            if duplicate_index < len(duplicate_segment):
                if duplicate_segment[duplicate_index][0] == next_line_num_in_source:
                    next_line_num_in_source = duplicate_segment[duplicate_index][1]
                    duplicate_index += 1
            if other_code.get(line_num) is not None:
                print other_code[line_num]
            else:
                (op_code, opns, _) = source[next_line_num_in_source]
                opns_str = ','.join([("%-"+ str(max_opn_length) +"s") % opn for opn in opns])
                print ("%-" + str(max_op_length) + "s ") % op_code, opns_str
                next_line_num_in_source += 1
            line_num += 1
    if register_map:
        print "New source code has the following mappings:"
        register_map_list = sorted(register_map.iteritems(), key=lambda x: x[0], reverse=False)
        for origin, now in register_map_list:
            if len(now) > 1:
                for k, rep_register in enumerate(now):
                    print origin + "(" + str(k) + ")", "-->", rep_register
            else:
                print origin, "-->", now[0]

