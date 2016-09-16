#!/usr/bin/python

import sys
from xml.dom import minidom

input = sys.argv[1]

def get_expected_status(filename):
    if "true" in filename:
        return "true"
    else:
        return "false"

def sum_time(l):
    sum_l = []
    current = 0
    for elt in l:
        current = current+float(elt)
        sum_l.append(current)
    return sum_l

li = []
xmldoc = minidom.parse(input)
filelist = xmldoc.getElementsByTagName('sourcefile')

points = 0
for s in filelist :
    eltlist = s.getElementsByTagName('column')
    filename = s.attributes["name"].value
    expected_status = get_expected_status(filename)
    for e in eltlist :
        title = e.attributes["title"].value
        value = e.attributes["value"].value
        if "status" in title:
            status = value
        if "cputime" in title:
            cputime = value
    if "true" in expected_status and "true" in status:
        li.append(float(cputime))
        li.append(float(cputime))
    #if "false" in expected_status and "false" in status:
    #    li.append(float(cputime))
    if "false" in expected_status and "true" in status:
        points = points - 8
    if "true" in expected_status and "false" in status:
        points = points - 4


li.sort()
#for elt in sum_time(li):
#    points = points + 2
#    print str(points)+" "+str(elt)

pair = 1
print str(points)+" 0.0"
for elt in li:
    points = points + 1
    if len(li) < 2000 or pair == 1:
        print str(points)+" "+str(elt)
    pair = 1 - pair
