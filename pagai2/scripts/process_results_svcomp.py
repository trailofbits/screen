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

def process_result(filename,json_dict):
    res = "UNKNOWN (ERROR)"
    if "_true" in filename:
        expected = "TRUE"
    elif "_false" in filename:
        expected = "FALSE"
    else:
        expected = "UNKNOWN"

    try:
        f = open(filename,"r")
        for line in f:
            if "TRUE" in line :
                res = "TRUE"
            if "UNKNOWN" in line :
                res = "UNKNOWN"
            if "TIME:" in line :
                time = line.replace("TIME: ","")
                time = time.replace("\n","")
        f.close()
        domain = get_domain(filename)
        json_dict[json_name(filename)][domain] = dict()
        json_dict[json_name(filename)][domain]["result"] = res
        json_dict[json_name(filename)][domain]["expected"] = expected
        json_dict[json_name(filename)][domain]["time"] = time
    except:
        return

def get_domain(name):
    if ".box.O3." in name:
        return "boxO3"
    elif ".pk.O3." in name:
        return "pkO3"
    elif ".box." in name:
        return "box"
    elif ".pk." in name:
        return "pk"
    else:
        return "unknown"

def json_name(name):
    res = os.path.basename(name)
    res = res.replace(".box.O3.svcomp.res",".c")
    res = res.replace(".pk.O3.svcomp.res",".c")
    res = res.replace(".box.svcomp.res",".c")
    res = res.replace(".pk.svcomp.res",".c")
    return res

def process_input_files():
    root_dir = sys.argv[1]
    bench = sys.argv[2]
    json_filename = "results_svcomp.json"

    if os.path.isfile(json_filename):
        json_data=open(json_filename)
        json_dict = json.load(json_data)
        json_data.close()
    else:
        json_dict = dict()

    for filename in sys.argv[3:]:
        if json_name(filename) not in json_dict:
            json_dict[json_name(filename)] = dict()
        process_result(filename,json_dict)

    jsonfile = open(json_filename, "w")
    jsonfile.write(json.dumps(json_dict))
    jsonfile.close()

process_input_files()

