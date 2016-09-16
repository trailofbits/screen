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
    print r"\begin{tabular}{|l|l|rrrr|rrrr|rrrr|rrrr|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{tabular}"

def print_line_header():
    print r"\multirow{2}{*}{Benchmark}"\
    + ' & ' +r"\multirow{2}{*}{\#Func}"\
    + ' & ' +r"\multicolumn{4}{c|}{C/S}"\
    + ' & ' +r"\multicolumn{4}{c|}{C/PF}"\
    + ' & ' +r"\multicolumn{4}{c|}{C/G}"\
    + ' & ' +r"\multicolumn{4}{c|}{PF/G}"\
    + r"\\ \cline{3-18}"
    print \
      ' & & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + r"\\ \hline"

def get_str_comparison(benchmark_name,comparison,result):
    try:
        r = str(json_dict[benchmark_name]["comparison"][comparison][result])
    except:
        r = "-"
    return r

def get_num_functions(benchmark_name):
    try:
        r = str(json_dict[benchmark_name]["functions"])
    except:
        r = "-"
    return r

begin_tabular()
print_line_header()


for benchmark_name in json_dict:
    if '-' in get_str_comparison(benchmark_name,"C / S", "lt"):
        continue
    print escape_latex(benchmark_name) \
    + ' & ' + get_num_functions(benchmark_name)\
    + ' & ' + get_str_comparison(benchmark_name,"C / S", "lt")\
    + ' & ' + get_str_comparison(benchmark_name,"C / S", "gt")\
    + ' & ' + get_str_comparison(benchmark_name,"C / S", "eq")\
    + ' & ' + get_str_comparison(benchmark_name,"C / S", "un")\
    + ' & ' + get_str_comparison(benchmark_name,"C / PF", "lt")\
    + ' & ' + get_str_comparison(benchmark_name,"C / PF", "gt")\
    + ' & ' + get_str_comparison(benchmark_name,"C / PF", "eq")\
    + ' & ' + get_str_comparison(benchmark_name,"C / PF", "un")\
    + ' & ' + get_str_comparison(benchmark_name,"C / G", "lt")\
    + ' & ' + get_str_comparison(benchmark_name,"C / G", "gt")\
    + ' & ' + get_str_comparison(benchmark_name,"C / G", "eq")\
    + ' & ' + get_str_comparison(benchmark_name,"C / G", "un")\
    + ' & ' + get_str_comparison(benchmark_name,"PF / G", "lt")\
    + ' & ' + get_str_comparison(benchmark_name,"PF / G", "gt")\
    + ' & ' + get_str_comparison(benchmark_name,"PF / G", "eq")\
    + ' & ' + get_str_comparison(benchmark_name,"PF / G", "un")\
    + r"\\"
end_tabular()
