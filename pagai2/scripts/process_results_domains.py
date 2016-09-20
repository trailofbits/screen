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

def get_technique(filename):
    string = getFileBetween(filename,"TECHNIQUE:","TECHNIQUE_END")
    if not string :
        return
    return to_technique(string.rstrip())

def process_time(filename,technique,time_s_array,time_ms_array,time_SMT_s_array,time_SMT_ms_array,json_dict):
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
        if len(elements) > 3:
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
        if cat not in json_dict[filename]["domain"][technique]:
            json_dict[filename]["domain"][technique][cat] = dict()
        json_dict[filename]["domain"][technique][cat][t] = float(elements[0] + "." + elements[1])
        cat = "time SMT"
        if cat not in json_dict[filename]["domain"][technique]:
            json_dict[filename]["domain"][technique][cat] = dict()
        if len(elements) > 3:
            json_dict[filename]["domain"][technique][cat][t] = float(elements[2] + "." + elements[3])
        else:
            json_dict[filename]["domain"][technique][cat][t] = float("0.0")

def process_warnings(filename,technique,warnings_array,safe_array,json_dict):
    string = getFileBetween(filename,"WARNINGS:","WARNINGS_END")
    if not string :
        return
    jsonfilename = json_name(filename)
    cat = "warnings"
    if cat not in json_dict[jsonfilename]["domain"][technique]:
        json_dict[jsonfilename]["domain"][technique][cat] = dict()

    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 1 )
        t = to_technique(elements[1])
        if t in warnings_array:
            warnings_array[t] += int(elements[0])
        else:
            warnings_array[t] = int(elements[0])

        if t not in json_dict[jsonfilename]["domain"][technique][cat]:
            json_dict[jsonfilename]["domain"][technique][cat][t] = dict()
        json_dict[jsonfilename]["domain"][technique][cat][t]["ko"] = int(elements[0])

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

        if t not in json_dict[jsonfilename]["domain"][technique][cat]:
            json_dict[jsonfilename]["domain"][technique][cat][t] = dict()
        json_dict[jsonfilename]["domain"][technique][cat][t]["ok"] = int(elements[0])


def process_matrix(filename,technique,matrix,json_dict):
    string = getFileBetween(filename,"MATRIX:","MATRIX_END")
    filename = json_name(filename)
    if not string :
        return
    for lines in string.rstrip().split('\n') :
        elements = lines.split(' ', 4 )
        d = elements[4]
        if d not in matrix :
            matrix[d] = dict()
            matrix[d][0] = int(elements[0])
            matrix[d][1] = int(elements[1])
            matrix[d][2] = int(elements[2])
            matrix[d][3] = int(elements[3])
        else:
            matrix[d][0] += int(elements[0])
            matrix[d][1] += int(elements[1])
            matrix[d][2] += int(elements[2])
            matrix[d][3] += int(elements[3])
        if "comparison" not in json_dict[filename]["domain"][technique]:
            json_dict[filename]["domain"][technique]["comparison"] = dict()
        if d not in json_dict[filename]["domain"][technique]["comparison"]:
            json_dict[filename]["domain"][technique]["comparison"][d] = dict()
        json_dict[filename]["domain"][technique]["comparison"][d]["eq"] = int(elements[0])
        json_dict[filename]["domain"][technique]["comparison"][d]["lt"] = int(elements[1])
        json_dict[filename]["domain"][technique]["comparison"][d]["gt"] = int(elements[2])
        json_dict[filename]["domain"][technique]["comparison"][d]["un"] = int(elements[3])


def json_name(name):
    res = os.path.basename(name)
    res = res.replace(".domain.s.pk.oct.res","")
    res = res.replace(".domain.s.pk.box.res","")
    res = res.replace(".domain.s.oct.box.res","")
    res = res.replace(".domain.s.pkgrid.pk.res","")
    res = res.replace(".domain.s.pkeq.pk.res","")
    res = res.replace(".domain.s.ppl_poly_bagnara.ppl_poly.res","")
    res = res.replace(".domain.pf.pk.oct.res","")
    res = res.replace(".domain.pf.pk.box.res","")
    res = res.replace(".domain.pf.oct.box.res","")
    res = res.replace(".domain.pf.pkgrid.pk.res","")
    res = res.replace(".domain.pf.pkeq.pk.res","")
    res = res.replace(".domain.pf.ppl_poly_bagnara.ppl_poly.res","")
    return res

def process_input_files():
    time_s_array = dict()
    time_ms_array = dict()
    time_SMT_s_array = dict()
    time_SMT_ms_array = dict()
    warnings_array = dict()
    safe_array = dict()
    matrix = dict()
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
        if "domain" not in json_dict[json_name(filename)]:
            json_dict[json_name(filename)]["domain"] = dict()
        technique = get_technique(filename)
        if technique not in json_dict[json_name(filename)]["domain"]:
            json_dict[json_name(filename)]["domain"][technique] = dict()

        process_time(filename,technique,time_s_array,time_ms_array,time_SMT_s_array,time_SMT_ms_array,json_dict)
        process_warnings(filename,technique,warnings_array,safe_array,json_dict)
        #process_skipped(filename,skipped_array,json_dict)
        #n_func += process_count_functions(filename,json_dict)
        #n_skipped += process_count_functions_skipped(filename,json_dict)
        process_matrix(filename,technique,matrix,json_dict)

    jsonfile = open("results.json", "w")
    jsonfile.write(json.dumps(json_dict))
    jsonfile.close()

process_input_files()

