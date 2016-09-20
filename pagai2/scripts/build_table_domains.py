#!/usr/bin/python

import sys
import json
import os

input = sys.argv[2]
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
    print r"\begin{tabular}{|l|rrrr|rrrr|rrrr|rrrr|rrrr|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{tabular}"

def print_line_header():
    print r"\multirow{2}{*}{Benchmark}"\
    + ' & ' +r"\multicolumn{4}{c|}{PK // OCT}"\
    + ' & ' +r"\multicolumn{4}{c|}{PK // BOX}"\
    + ' & ' +r"\multicolumn{4}{c|}{OCT // BOX}"\
    + ' & ' +r"\multicolumn{4}{c|}{PKGRID // PK}"\
    + ' & ' +r"\multicolumn{4}{c|}{PPL\_B // PPL}"\
    + r"\\ \cline{2-21}"
    print \
      ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
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
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + r"\\ \hline"

def get_str_comparison(benchmark_name,technique,comparison,result):
    try:
        r = str(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison][result])
    except:
        r = "-"
    return r

def get_str_comparison_total(technique,comparison,result):
    global json_dict
    timeout = ""
    try:
        r = 0
        for benchmark_name in json_dict:
            c = get_str_comparison(benchmark_name,technique,comparison,result)
            if "-" in c:
                timeout = "$\\geq$ "
            else:
                r = r + int(c)
        return str(r)
    except:
        return "-"

def get_str_comparison_percentage(benchmark_name,technique,comparison,result):
    try:
        res = float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison][result])

        total = float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["lt"]) + \
                float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["gt"]) +\
                float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["eq"]) +\
                float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["un"])
        r = str("%.0f" % round((res*100)/total,0))+"\%"
    except:
        r = "-"
    return r

def get_str_comparison_percentage_total(technique,comparison,result):
    global json_dict
    timeout = ""
    #try:
    res = 0.
    total = 0.
    for benchmark_name in json_dict:
        val = get_str_comparison(benchmark_name,technique,comparison,result)
        if "-" in val:
            continue
        else:
            res = res + float(val)

            total = total + float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["lt"]) + \
                    float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["gt"]) +\
                    float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["eq"]) +\
                    float(json_dict[benchmark_name]["domain"][technique]["comparison"][comparison]["un"])
    return str("%.0f" % round((res*100)/total,0))+"\%"
    #except:
    #   return "-"


begin_tabular()
print_line_header()

t = sys.argv[1] # technique


if False:
    for benchmark_name in json_dict:
        print escape_latex(benchmark_name) \
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // OCT", "lt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // OCT", "gt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // OCT", "eq")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // OCT", "un")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // BOX", "lt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // BOX", "gt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // BOX", "eq")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PK // BOX", "un")\
        + ' & ' + get_str_comparison(benchmark_name,t,"OCT // BOX", "lt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"OCT // BOX", "gt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"OCT // BOX", "eq")\
        + ' & ' + get_str_comparison(benchmark_name,t,"OCT // BOX", "un")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PKGRID // PK", "lt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PKGRID // PK", "gt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PKGRID // PK", "eq")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PKGRID // PK", "un")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "lt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "gt")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "eq")\
        + ' & ' + get_str_comparison(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "un")\
        + r"\\"

    print "\hline \\textbf{TOTAL}" \
    + ' & ' + get_str_comparison_total(t,"PK // OCT", "lt")\
    + ' & ' + get_str_comparison_total(t,"PK // OCT", "gt")\
    + ' & ' + get_str_comparison_total(t,"PK // OCT", "eq")\
    + ' & ' + get_str_comparison_total(t,"PK // OCT", "un")\
    + ' & ' + get_str_comparison_total(t,"PK // BOX", "lt")\
    + ' & ' + get_str_comparison_total(t,"PK // BOX", "gt")\
    + ' & ' + get_str_comparison_total(t,"PK // BOX", "eq")\
    + ' & ' + get_str_comparison_total(t,"PK // BOX", "un")\
    + ' & ' + get_str_comparison_total(t,"OCT // BOX", "lt")\
    + ' & ' + get_str_comparison_total(t,"OCT // BOX", "gt")\
    + ' & ' + get_str_comparison_total(t,"OCT // BOX", "eq")\
    + ' & ' + get_str_comparison_total(t,"OCT // BOX", "un")\
    + ' & ' + get_str_comparison_total(t,"PKGRID // PK", "lt")\
    + ' & ' + get_str_comparison_total(t,"PKGRID // PK", "gt")\
    + ' & ' + get_str_comparison_total(t,"PKGRID // PK", "eq")\
    + ' & ' + get_str_comparison_total(t,"PKGRID // PK", "un")\
    + ' & ' + get_str_comparison_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "lt")\
    + ' & ' + get_str_comparison_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "gt")\
    + ' & ' + get_str_comparison_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "eq")\
    + ' & ' + get_str_comparison_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "un")\
    + r"\\"
else:
    for benchmark_name in json_dict:
        print escape_latex(benchmark_name) \
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // OCT", "lt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // OCT", "gt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // OCT", "eq")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // OCT", "un")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // BOX", "lt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // BOX", "gt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // BOX", "eq")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PK // BOX", "un")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"OCT // BOX", "lt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"OCT // BOX", "gt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"OCT // BOX", "eq")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"OCT // BOX", "un")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PKGRID // PK", "lt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PKGRID // PK", "gt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PKGRID // PK", "eq")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PKGRID // PK", "un")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "lt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "gt")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "eq")\
        + ' & ' + get_str_comparison_percentage(benchmark_name,t,"PPL_POLY_BAGNARA // PPL_POLY", "un")\
        + r"\\"

    print "\hline \\textbf{TOTAL}" \
    + ' & ' + get_str_comparison_percentage_total(t,"PK // OCT", "lt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // OCT", "gt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // OCT", "eq")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // OCT", "un")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // BOX", "lt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // BOX", "gt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // BOX", "eq")\
    + ' & ' + get_str_comparison_percentage_total(t,"PK // BOX", "un")\
    + ' & ' + get_str_comparison_percentage_total(t,"OCT // BOX", "lt")\
    + ' & ' + get_str_comparison_percentage_total(t,"OCT // BOX", "gt")\
    + ' & ' + get_str_comparison_percentage_total(t,"OCT // BOX", "eq")\
    + ' & ' + get_str_comparison_percentage_total(t,"OCT // BOX", "un")\
    + ' & ' + get_str_comparison_percentage_total(t,"PKGRID // PK", "lt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PKGRID // PK", "gt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PKGRID // PK", "eq")\
    + ' & ' + get_str_comparison_percentage_total(t,"PKGRID // PK", "un")\
    + ' & ' + get_str_comparison_percentage_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "lt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "gt")\
    + ' & ' + get_str_comparison_percentage_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "eq")\
    + ' & ' + get_str_comparison_percentage_total(t,"PPL_POLY_BAGNARA // PPL_POLY", "un")\
    + r"\\"
end_tabular()
