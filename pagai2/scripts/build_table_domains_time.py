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
    print r"\begin{tabular}{|l|rrrrrrr|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{tabular}"

def print_line_header():
    print r"Benchmark"\
    + ' & ' +r"BOX"\
    + ' & ' +r"PKEQ"\
    + ' & ' +r"PPL"\
    + ' & ' +r"PPL\_B"\
    + ' & ' +r"PKGRID"\
    + ' & ' +r"PK"\
    + ' & ' +r"OCT"\
    + r"\\ \hline"

def get_str_time(benchmark_name,technique,domain):
    try:
        r = "%.1f" % json_dict[benchmark_name]["domain"][technique]["time"][domain]
    except:
        r = "-"
    return r

def get_str_time_total(technique,domain):
    global json_dict
    timeout = ""
    try:
        r = 0.
        for benchmark_name in json_dict:
            t = get_str_time(benchmark_name,technique,domain)
            if "-" in t:
                timeout = ">"
            else:
                r = r + float(t)
        return timeout + ("%.1f" % r)
    except:
        return "-"

begin_tabular()
print_line_header()

t = sys.argv[1] # technique

for benchmark_name in json_dict:
    print escape_latex(benchmark_name) \
    + ' & ' + get_str_time(benchmark_name,t,"BOX")\
    + ' & ' + get_str_time(benchmark_name,t,"PKEQ")\
    + ' & ' + get_str_time(benchmark_name,t,"PPL_POLY")\
    + ' & ' + get_str_time(benchmark_name,t,"PPL_POLY_BAGNARA")\
    + ' & ' + get_str_time(benchmark_name,t,"PKGRID")\
    + ' & ' + get_str_time(benchmark_name,t,"PK")\
    + ' & ' + get_str_time(benchmark_name,t,"OCT")\
    + r"\\"

print "\hline \\textbf{TOTAL}" \
+ ' & ' + get_str_time_total(t,"BOX")\
+ ' & ' + get_str_time_total(t,"PKEQ")\
+ ' & ' + get_str_time_total(t,"PPL_POLY")\
+ ' & ' + get_str_time_total(t,"PPL_POLY_BAGNARA")\
+ ' & ' + get_str_time_total(t,"PKGRID")\
+ ' & ' + get_str_time_total(t,"PK")\
+ ' & ' + get_str_time_total(t,"OCT")\
+ r"\\"

end_tabular()
