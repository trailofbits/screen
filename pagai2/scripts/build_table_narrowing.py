#!/usr/bin/python

import sys
import json
import os

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
        #index = escaped_name.find('_')
        index = 15
        escaped_name = escaped_name[0:index]
    return escaped_name


def begin_tabular():
    print r"\begin{tabular}{|l|rr|SS|rrrr|SS|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{tabular}"

def print_line_header():
    print r"\multirow{2}{*}{Benchmark}"\
    + ' & ' +r"\multicolumn{2}{c|}{Comparison}"\
    + ' & ' +r"\multicolumn{2}{c|}{Time}"\
    + ' & ' +r"\multicolumn{4}{c|}{Functions}"\
    + ' & ' +r"\multicolumn{2}{c|}{Time eq}"\
    + r"\\ \cline{2-11}"
    print \
    "& $\sqsubset$"\
    + ' & ' +"$=$"\
    + ' & ' +"\multicolumn{1}{c}{N}"\
    + ' & ' +"\multicolumn{1}{c|}{S}"\
    + ' & ' +"\#total"\
    + ' & ' +"\#seeds"\
    + ' & ' +"$\sqsubset$"\
    + ' & ' +"$=$"\
    + ' & ' +"N"\
    + ' & ' +"S"\
    + r"\\ \hline"


begin_tabular()
print_line_header()
for benchmark_name in json_dict:
    if "comparison" not in json_dict[benchmark_name]:
        continue

    print escape_latex(benchmark_name) \
    + ' & ' + str(json_dict[benchmark_name]["comparison"]["N / S"]["lt"])\
    + ' & ' + str(json_dict[benchmark_name]["comparison"]["N / S"]["eq"])\
    + ' & ' + "%.2f" %(json_dict[benchmark_name]["time"]["N"])\
    + ' & ' + "%.2f" %(json_dict[benchmark_name]["time"]["S"])\
    + ' & ' + str(json_dict[benchmark_name]["functions"])\
    + ' & ' + str(json_dict[benchmark_name]["seeds"]["functions with seeds"])\
    + ' & ' + str(json_dict[benchmark_name]["seeds"]["functions improved"])\
    + ' & ' + str(json_dict[benchmark_name]["seeds"]["functions equal"])\
    + ' & ' + "%.2f" %(json_dict[benchmark_name]["seeds"]["time eq"]["N"])\
    + ' & ' + "%.2f" %(json_dict[benchmark_name]["seeds"]["time eq"]["S"])\
    + r"\\"
end_tabular()
