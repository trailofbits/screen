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
        #index = escaped_name.find('\_true')
        escaped_name = escaped_name.replace('\_true-unreach-label','')
        escaped_name = escaped_name.replace('\_cilled','')
        escaped_name = escaped_name.replace('32\_1\_ok\_nondet\_linux-3.4-32\_1-drivers--','')
        escaped_name = escaped_name.replace('32\_7a\_linux-3.8-rc1-32\_7a-drivers--','')
        escaped_name = escaped_name.replace('32\_7a\_linux-3.8-rc1-32\_7a-','')
        escaped_name = escaped_name.replace('32\_7a\_linux-3.8-rc1-drivers--','')

        #index = 25
        index = escaped_name.find('\_sequence')
        escaped_name = escaped_name[0:index]
    return escaped_name


def begin_tabular():
    print r"\begin{longtable}{|l|r|S|r|S|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{longtable}"

def print_line_header():
    print \
    "Benchmark & result & time & result & time"\
    + r"\\ \hline"

def remove_color(str):
    ansi_escape = re.compile(r'\x1b[^m]*m')
    return ansi_escape.sub('', str)

def print_result(str):
    return str.replace('UNKNOWN (ERROR)','error')
    return str

begin_tabular()
print_line_header()
for benchmark_name in json_dict:
    if "box" not in json_dict[benchmark_name]:
        continue
    if "pk" not in json_dict[benchmark_name]:
        continue
    if "result" not in json_dict[benchmark_name]["box"]:
        continue
    if "result" not in json_dict[benchmark_name]["pk"]:
        continue
    if "FALSE" in json_dict[benchmark_name]["box"]["expected"]:
        continue
    print escape_latex(benchmark_name) \
    + ' & ' + print_result(str(json_dict[benchmark_name]["pk"]["result"]))\
    + ' & ' + remove_color(str(json_dict[benchmark_name]["pk"]["time"]))\
    + ' & ' + print_result(str(json_dict[benchmark_name]["box"]["result"]))\
    + ' & ' + remove_color(str(json_dict[benchmark_name]["box"]["time"]))\
    + r"\\"
end_tabular()
