#!/usr/bin/python

import sys
import json
from os import system
import os

def getFileBetween(filename,begin,end):
    try:
        f = open(filename,"r")
        res = ""
        start = False
        for line in f:
            if end in line :
                f.close()
                return res
            if start :
                res += line
            if begin in line :
                start = True
    except IOError:
        return ""

def get_techniques(filename):
    res = set()
    string = getFileBetween(filename,"TIME:","TIME_END")
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 2 )
        res.add(elements[2])
    return res


def to_technique(string):
    res = ""
    ok = False
    for c in string:
        if c != '\t' and c != ' ' and c != '/' :
            ok = True
        if ok :
            res += c
    res = res.replace("PATH FOCUSING","PF")
    res = res.replace("GUIDED","G")
    res = res.replace("CLASSIC","S")
    res = res.replace("NEWNARROWING","N")
    res = res.replace("COMBINED","C")
    res = res.replace("DISJUNCTIVE","DIS")
    res = res.replace("LOOKAHEAD WIDENING","LW")
    return res

def process_time_eq(filename,json_dict):
    string = getFileBetween(filename,"TIME_EQ:","TIME_EQ_END")
    filename = json_name(filename)
    if not string :
        return
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 4 )
        t = to_technique(elements[-1]) # last element of the list
        cat = "seeds"
        if cat not in json_dict[filename]:
            json_dict[filename][cat] = dict()
        if "time eq" not in json_dict[filename][cat]:
            json_dict[filename][cat]["time eq"] = dict()
        json_dict[filename][cat]["time eq"][t] = float(elements[0] + "." + elements[1])

def process_time(filename,time_s_array,time_ms_array,time_SMT_s_array,time_SMT_ms_array,json_dict):
    string = getFileBetween(filename,"TIME:","TIME_END")
    if not string :
        return
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 4 )
        t = to_technique(elements[-1]) # last element of the list
        if t in time_s_array:
            time_s_array[t] += int(elements[0])
        else:
            time_s_array[t] = int(elements[0])
        if t in time_ms_array:
            time_ms_array[t] += int(elements[1])
        else:
            time_ms_array[t] = int(elements[1])
        if time_ms_array[t] >= 1000000 :
            time_ms_array[t] -= 1000000
            time_s_array[t] += 1
        #same for time_SMT arrays
        if len(elements) > 3 and elements[2] != "//":
            if t in time_SMT_s_array:
                time_SMT_s_array[t] += int(elements[2])
            else:
                time_SMT_s_array[t] = int(elements[2])
            if t in time_SMT_ms_array:
                time_SMT_ms_array[t] += int(elements[3])
            else:
                time_SMT_ms_array[t] = int(elements[3])
            if time_SMT_ms_array[t] >= 1000000 :
                time_SMT_ms_array[t] -= 1000000
                time_SMT_s_array[t] += 1
        else:
            time_SMT_s_array[t] = 0
            time_SMT_ms_array[t] = 0

        filename = json_name(filename)
        cat = "time"
        if cat not in json_dict[filename]:
            json_dict[filename][cat] = dict()
        json_dict[filename][cat][t] = float(elements[0] + "." + elements[1])
        cat = "time SMT"
        if cat not in json_dict[filename]:
            json_dict[filename][cat] = dict()
        if len(elements) > 3 and elements[2] != "//":
            json_dict[filename][cat][t] = float(elements[2] + "." + elements[3])
        else:
            json_dict[filename][cat][t] = float("0.0")

def process_warnings(filename,warnings_array,safe_array,json_dict):
    string = getFileBetween(filename,"WARNINGS:","WARNINGS_END")
    if not string :
        return
    jsonfilename = json_name(filename)
    cat = "warnings"
    if cat not in json_dict[jsonfilename]:
        json_dict[jsonfilename][cat] = dict()

    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 1 )
        t = to_technique(elements[1])
        if t in warnings_array:
            warnings_array[t] += int(elements[0])
        else:
            warnings_array[t] = int(elements[0])

        if t not in json_dict[jsonfilename][cat]:
            json_dict[jsonfilename][cat][t] = dict()
        json_dict[jsonfilename][cat][t]["ko"] = int(elements[0])

    string = getFileBetween(filename,"SAFE_PROPERTIES:","SAFE_PROPERTIES_END")
    if not string :
        return
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 1 )
        t = to_technique(elements[1])
        if t in safe_array:
            safe_array[t] += int(elements[0])
        else:
            safe_array[t] = int(elements[0])

        if t not in json_dict[jsonfilename][cat]:
            json_dict[jsonfilename][cat][t] = dict()
        json_dict[jsonfilename][cat][t]["ok"] = int(elements[0])


def process_count_functions(filename,json_dict):
    string = getFileBetween(filename,"FUNCTIONS:","FUNCTIONS_END")
    filename = json_name(filename)
    if not string :
        return 0
    n = 0
    for lines in string.rstrip().split('\n') :
        n += int(lines)
    json_dict[filename]["functions"] = n
    return n

def process_function_seeds(filename,json_dict):
    string = getFileBetween(filename,"FUNCTIONS_SEEDS:","FUNCTIONS_SEEDS_END")
    filename = json_name(filename)
    if not string :
        return 0
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 4 )
        cat = "seeds"
        if cat not in json_dict[filename]:
            json_dict[filename][cat] = dict()
        json_dict[filename][cat]["functions equal"] = int(elements[0])
        json_dict[filename][cat]["functions improved"] = int(elements[1])
        json_dict[filename][cat]["functions with seeds"] = int(elements[2])
        json_dict[filename][cat]["functions with no seeds"] = int(elements[3])


def process_count_functions_skipped(filename,json_dict):
    string = getFileBetween(filename,"IGNORED:","IGNORED_END")
    if not string :
        return 0
    n = 0
    for lines in string.rstrip().split('\n') :
        n += int(lines)
    filename = json_name(filename)
    json_dict[filename]["functions_skipped"] = n
    return n

def process_skipped(filename,skipped_array,json_dict):
    string = getFileBetween(filename,"SKIPPED:","SKIPPED_END")
    filename = json_name(filename)
    if not string :
        return
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 1 )
        t = to_technique(elements[1])
        if t in skipped_array:
            skipped_array[t] += int(elements[0])
        else:
            skipped_array[t] = int(elements[0])

        cat = "skipped"
        if cat not in json_dict[filename]:
            json_dict[filename][cat] = dict()
        json_dict[filename][cat][t] = elements[0]


def process_matrix(filename,matrix,json_dict):
    string = getFileBetween(filename,"MATRIX:","MATRIX_END")
    filename = json_name(filename)
    if not string :
        return
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 4 )
        t = to_technique(elements[4])
        if t not in matrix :
            matrix[t] = dict()
            matrix[t][0] = int(elements[0])
            matrix[t][1] = int(elements[1])
            matrix[t][2] = int(elements[2])
            matrix[t][3] = int(elements[3])
        else:
            matrix[t][0] += int(elements[0])
            matrix[t][1] += int(elements[1])
            matrix[t][2] += int(elements[2])
            matrix[t][3] += int(elements[3])
        if "comparison" not in json_dict[filename]:
            json_dict[filename]["comparison"] = dict()
        if t not in json_dict[filename]["comparison"]:
            json_dict[filename]["comparison"][t] = dict()
        json_dict[filename]["comparison"][t]["eq"] = int(elements[0])
        json_dict[filename]["comparison"][t]["lt"] = int(elements[1])
        json_dict[filename]["comparison"][t]["gt"] = int(elements[2])
        json_dict[filename]["comparison"][t]["un"] = int(elements[3])

def compute_bb_number(matrix):
    n = 0
    for t in matrix :
        for val in matrix[t] :
            n += matrix[t][val]
        return n

def compute_matrix_average(matrix):
    matrix_average = dict()
    n_blocks = compute_bb_number(matrix)
    #compute the averages
    for t in matrix :
        if t not in matrix_average :
            matrix_average[t] = dict()
        for val in matrix[t] :
            matrix_average[t][val] = float(matrix[t][val]) * 100. / float(n_blocks)
    return matrix_average

def generate_gnuplot_from_matrix(matrix,root_dir,bench,graph_name):
    f = open('/tmp/data_gnuplot', 'w')
    for t in matrix :
        f.write('"'+t+'" ')
        for val in matrix[t] :
            f.write(str(matrix[t][val])+' ')
        f.write("\n")
    f.close()
    system('gnuplot '+root_dir+'/techniques.gnuplot')
    system('mv techniques.svg '+graph_name+'.techniques.svg')

def generate_gnuplot_from_time_array(time_s_array,time_ms_array,time_SMT_s_array,time_SMT_ms_array,root_dir,bench,graph_name):
    f = open('/tmp/data_time_gnuplot', 'w')
    for t in time_s_array :
        ms = str(time_ms_array[t])
        if t not in time_SMT_ms_array:
            time_SMT_ms_array[t] = 0
        if t not in time_SMT_s_array:
            time_SMT_s_array[t] = 0
        ms_SMT = str(time_SMT_ms_array[t])
        while len(ms) != 6 :
            ms = "0"+ms
        while len(ms_SMT) != 6 :
            ms_SMT = "0"+ms_SMT
        f.write('"'+t+'" '+str(time_s_array[t])+"."+ms+" "+str(time_SMT_s_array[t])+"."+ms_SMT+"\n")
    f.close()
    system('gnuplot '+root_dir+'/techniques_time.gnuplot')
    system('mv techniques_time.svg '+graph_name+'.time.svg')

def generate_gnuplot_from_warnings_array(warning_array,safe_array,root_dir,bench,graph_name):
    f = open('/tmp/data_warning_gnuplot', 'w')
    for t in warning_array :
        ms = str(warning_array[t])
        f.write('"'+t+'" '+str(warning_array[t])+' '+str(safe_array[t])+"\n")
    f.close()
    system('gnuplot '+root_dir+'/techniques_warnings.gnuplot')
    system('mv techniques_warnings.svg '+graph_name+'.warnings.svg')

def generate_gnuplot_from_skipped_array(skipped_array,root_dir,bench,graph_name):
    f = open('/tmp/data_skipped_gnuplot', 'w')
    for t in skipped_array :
        ms = str(skipped_array[t])
        f.write('"'+t+'" '+str(skipped_array[t])+"\n")
    f.close()
    system('gnuplot '+root_dir+'/techniques_skipped.gnuplot')
    system('mv techniques_skipped.svg '+graph_name+'.skipped.svg')

def json_name(name):
    res = os.path.basename(name)
    res = res.replace(".all.res","")
    res = res.replace(".narrowing.res","")
    return res

def process_input_files():
    time_s_array = dict()
    time_ms_array = dict()
    time_SMT_s_array = dict()
    time_SMT_ms_array = dict()
    warnings_array = dict()
    safe_array = dict()
    skipped_array = dict()
    matrix = dict()
    n_func = 0
    n_skipped = 0
    root_dir = sys.argv[1]
    bench = sys.argv[2]
    graph_name = sys.argv[3]

    if os.path.isfile('results.json'):
        json_data=open('results.json')
        json_dict = json.load(json_data)
        json_data.close()
    else:
        json_dict = dict()

    for filename in sys.argv[4:]:
        if json_name(filename) not in json_dict:
            json_dict[json_name(filename)] = dict()
        process_time(filename,time_s_array,time_ms_array,time_SMT_s_array,time_SMT_ms_array,json_dict)
        process_warnings(filename,warnings_array,safe_array,json_dict)
        process_skipped(filename,skipped_array,json_dict)
        n_func += process_count_functions(filename,json_dict)
        n_skipped += process_count_functions_skipped(filename,json_dict)
        process_matrix(filename,matrix,json_dict)
        process_function_seeds(filename,json_dict)
        process_time_eq(filename,json_dict)

    matrix_average = compute_matrix_average(matrix)

    generate_gnuplot_from_matrix(matrix_average,root_dir,bench,graph_name)
    generate_gnuplot_from_time_array(time_s_array,time_ms_array,time_SMT_s_array,time_SMT_ms_array,root_dir,bench,graph_name)
    generate_gnuplot_from_warnings_array(warnings_array,safe_array,root_dir,bench,graph_name)
    generate_gnuplot_from_skipped_array(skipped_array,root_dir,bench,graph_name)
    jsonfile = open("results.json", "w")
    jsonfile.write(json.dumps(json_dict))
    jsonfile.close()

process_input_files()

