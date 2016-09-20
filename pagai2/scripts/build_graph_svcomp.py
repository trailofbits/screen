#!/usr/bin/python

import sys
import json
import os
import re


input = sys.argv[1]
if os.path.isfile(input):
    json_data=open(input)
    json_dict = json.load(json_data)
    json_data.close()
else:
    json_dict = dict()

def escape_latex(name):
    CHARS = {
        '&':  r'\&',
        '%':  r'\%',
        '$':  r'\$',
        '#':  r'\#',
        '_':  r'\_',
        '{':  r'\}',
        '}':  r'\}',
        '~':  r'\~',
        '^':  r'\^',
        '\\': r'\backslash',
    }
    escaped_name="".join([CHARS.get(char, char) for char in name])
    if len(escaped_name) > 15:
        index = escaped_name.find('\_true')
        #index = 25
        escaped_name = escaped_name[0:index]
    return escaped_name

def remove_color(str):
    ansi_escape = re.compile(r'\x1b[^m]*m')
    return ansi_escape.sub('', str)

def sum_time(l):
    sum_l = []
    current = 0
    for elt in l:
        current = current+float(elt)
        sum_l.append(current)
    return sum_l

def get_time(benchmark_name,domain):
    try:
        return float(remove_color(json_dict[benchmark_name][domain]["time"]))
    except:
        return 1100.

def get_res(benchmark_name,domain):
    try:
        # do not consider the files analyzed with -O3
        if "O3" in domain:
            return "UNKNOWN"
        return str(remove_color(json_dict[benchmark_name][domain]["result"]))
    except:
        return "UNKNOWN"


li = []
points = 0
for benchmark_name in json_dict:
    if "FALSE" in json_dict[benchmark_name]["box"]["expected"]:
        # benchmark is FALSE
        res_pk   = get_res(benchmark_name,"pk")
        res_box  = get_res(benchmark_name,"box")
        res_pkO3   = get_res(benchmark_name,"pkO3")
        res_boxO3  = get_res(benchmark_name,"boxO3")
        if "TRUE" in res_pk or "TRUE" in res_box or "TRUE" in res_pkO3 or "TRUE" in res_boxO3:
            points = points - 8
        continue
    else:
        # benchmark is TRUE
        res_pk   = get_res(benchmark_name,"pk")
        res_box  = get_res(benchmark_name,"box")
        time_pk  = get_time(benchmark_name,"pk")
        time_box = get_time(benchmark_name,"box")

        res_pkO3   = get_res(benchmark_name,"pkO3")
        res_boxO3  = get_res(benchmark_name,"boxO3")
        time_pkO3  = get_time(benchmark_name,"pkO3")
        time_boxO3 = get_time(benchmark_name,"boxO3")

        if "TRUE" not in res_pk and "TRUE" not in res_box and "TRUE" not in res_pkO3 and "TRUE" not in res_boxO3:
            # not proved, 0 points
            continue

        min_time = 1000000.
        if "TRUE" in res_pk:
            min_time = min(min_time,time_pk)
        if "TRUE" in res_box:
            min_time = min(min_time,time_box)
        if "TRUE" in res_pkO3:
            min_time = min(min_time,time_pkO3)
        if "TRUE" in res_boxO3:
            min_time = min(min_time,time_boxO3)
        li.append(min_time)


li.sort()
#for elt in sum_time(li):
for elt in li:
    points = points + 2
    print str(points)+" "+str(elt)

