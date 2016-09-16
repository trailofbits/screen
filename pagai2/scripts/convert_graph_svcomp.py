#!/usr/bin/python

import sys
import json
import os
import re


input = sys.argv[1]
if os.path.isfile(input):
    data=open(input)
    content = data.readlines()
    data.close()

previoustime=0.
for line in content:
    point,time = line.split()
    adjustedtime = float(time) - previoustime
    previoustime = float(time)
    print str(point) + ' ' + str(adjustedtime)
