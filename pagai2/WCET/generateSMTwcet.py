import sys
import commands
import re
import argparse
import json
from heapq import heappush, heappop

number_articulations = 0
labels = {}
vars = {}
debug = False
cut_id = 0
first_label = ''
last_label = ''
first_var = ''
first_var_bd = ''
last_var = ''
heap_cuts = []
my_graph = {}

def match_names(blocks):
    global labels
    global vars
    global first_label
    global last_label
    global first_var
    global first_var_bd
    global last_var

    first_var_ok = False
    for i in range(1, len(blocks)):
        id, instructions = blocks[i].split(':', 1)

        var = id[1:]
        if instructions.find('<label>:') != -1:
            #print 'NOT IMPLEMENTED'
            label = instructions.split('<label>:')[1].split(' ')[0]
        else:
            label = instructions.split('\n', 1)[1].split(':')[0]

        if "bd_" in var:
            if first_var_ok:
                last_label = label
                last_var = var
            else:
                first_var_ok = True
                first_label = label
                first_var_bd = var
                var = var.replace("bd","bs")
                first_var = var

        labels[var] = label
        vars[label] = var

label_costs = {}
transition_costs = {}
# read the file given by Mihail's code, that consists in tuples
# of the form (source,destination,cost) if cost is attached to transitions
# or form (block,cost) if cost is attached to blocks
def readmatchingfile(matchingfile):
    global costs
    #for line in matchingfile:
    #    name,arm,cost = line.split(',')
    #    name = name.replace("(%", "")
    #    name = name.replace(" ", "")
    #    cost = cost.replace(")\n", "")
    #    cost = int(cost.replace(" ", ""))
    #    label_costs[name] = cost
    for line in matchingfile:
        source,dest,cost = line.split(',')
        source = source.replace("(", "")
        source = source.replace("%", "")
        source = source.replace(" ", "")
        dest = dest.replace("%", "")
        dest = dest.replace(" ", "")
        cost = cost.replace(")\n", "")
        cost = int(cost.replace(" ", ""))
        if dest == "":
            label_costs[source] = cost
        else:
            #print source
            #print dest
            if source not in transition_costs:
                transition_costs[source] = {}
            transition_costs[source][dest] = cost

heads = []
tails = []
semanticcut = []
semanticcut_maxcost = []

def addautomaticcuts():
    global my_graph
    #for mnode in my_graph[last_var][2]:
    mnode = last_var
    node = mnode
    mergingpoints = [node]
    while node != first_var:
        node_dominator = my_graph[node][4]
        mergingpoints.append(node_dominator)
        node = node_dominator
    N = len(mergingpoints)
    step = 2
    #print mergingpoints
    while step < N:
        k = 0
        done=False
        while not done:
            tail = mergingpoints[k]
            if k+step < N:
                head = mergingpoints[k+step]
            else:
                head = mergingpoints[N-1]
                done=True
            #print "between " + tail + " " + head + " " + str(step)
            heads.append([labels[head]])
            tails.append([labels[tail]])
            semanticcut.append("")
            semanticcut_maxcost.append(0)
            k += step
        step = step*2



def readcutsfile(cutsfile):
    # with this file, we can set that the portion between head1 and tail1
    # and the portion between head2 and tail2 are exclusive
    index=0
    heads.append([])
    tails.append([])
    semanticcut.append("")
    semanticcut_maxcost.append(0)
    for line in cutsfile:
        if "#" in line:
            index = index+1
            heads.append([])
            tails.append([])
            semanticcut.append("")
            semanticcut_maxcost.append(0)
        else:
            head,tail = line.split(',')
            head = head.replace("%", "")
            head = head.replace(" ", "")
            head = head.replace("\n", "")
            tail = tail.replace("%", "")
            tail = tail.replace(" ", "")
            tail = tail.replace("\n", "")
            heads[index].append(head)
            tails[index].append(tail)

def getcost(label):
    global costs
    if label in label_costs:
        return label_costs[label]
    else:
        return 0

def getcost_transition(source_label,dest_label):
    #print "getcost_transition " + source_label + " " + dest_label
    global costs
    if source_label in transition_costs:
        if dest_label in transition_costs[source_label]:
            return transition_costs[source_label][dest_label]
    #print "missing cost for transition: " + source_label + " " + dest_label +\
    #":" + get_transition_varname(vars[source_label],vars[dest_label])
    return 0

def get_tcost_var(var):
    return 'c_' + var.split('_')[1] + '_' + var.split('_')[2]

def get_transition_varname(sourcevar,destvar):
    #print sourcevar + " --> " + destvar
    return "t_" + sourcevar.split('_')[1] + "_" + destvar.split('_')[1]

# count the number of distinct path between the starting point of the graph and
# a specific node
def getNumPaths(graph, node):
    if graph[node][6] != -1:
        return graph[node][6]
    num = 0
    empty = True
    for n in graph[node][2]:
        empty = False
        num += getNumPaths(graph, n)
    if empty:
        num = 1
    graph[node][6] = num
    return num

def Maximum(x,y):
    if x > y:
        return x
    else:
        return y

def get_summary_max(graph,start,end,current,costs,tcosts):
    global debug
    global labels

    if current in costs:
        return costs[current]
    current_cost = graph[current][1]

    if debug:
        print 'get_summary_max : ' + labels[current] + '  :  ' + str(len(graph[current][2])) + ' predecessors\n'
    if current != start:
        #iterate through incoming edges
        max = 0
        for n in graph[current][2]:
            if debug:
                print '    pred ' + labels[current] + '  :  ' + labels[n] + '\n'
            cost_transition_var = get_transition_varname(n,current)
            tcosts[cost_transition_var] = getcost_transition(labels[n],labels[current])
            #print "tcost = " + str(tcosts[cost_transition_var])
            max = Maximum(max,get_summary_max(graph,start,end,n,costs,tcosts)+tcosts[cost_transition_var])
            #max = Maximum(max,get_summary_max(graph,start,end,n,costs,tcosts))
        current_cost += max
    costs[current] = current_cost
    return current_cost

def get_summary_max_with_path(graph,start,end,current,costs,tcosts,longestpaths):
    global debug
    global labels

    if current in costs:
        return costs[current], longestpaths[current]
    current_cost = graph[current][1]
    longestpath = ""

    if debug:
        print 'get_summary_max : ' + labels[current] + '  :  ' + str(len(graph[current][2])) + ' predecessors\n'
    if current != start:
        #iterate through incoming edges
        max = 0
        max_last_transition = 0
        for n in graph[current][2]:
            if debug:
                print '    pred ' + labels[current] + '  :  ' + labels[n] + '\n'
            cost_transition_var = get_transition_varname(n,current)
            tcosts[cost_transition_var] = getcost_transition(labels[n],labels[current])
            #print "tcost = " + str(tcosts[cost_transition_var])
            #max = Maximum(max,get_summary_max(graph,start,end,n,costs,tcosts)+tcosts[cost_transition_var])
            c,p = get_summary_max_with_path(graph,start,end,n,costs,tcosts,longestpaths)
            if max < c+tcosts[cost_transition_var]:
                longestpath = p
                max = c+tcosts[cost_transition_var]
                max_last_transition = tcosts[cost_transition_var]
            #max = Maximum(max,get_summary_max(graph,start,end,n,costs,tcosts))
        # output Julien:
        #longestpath += " --("+str(max_last_transition)+")-> " + labels[current] +\
        #"(" + str(graph[current][1]) + ")"
        # output Mihail:
        longestpath += ", " + labels[current] + ")\n(" +  labels[current]
        current_cost += max
    else:
        # output Mihail:
        longestpath += "(" + labels[current]
        # output Julien:
        #longestpath += labels[current] + "(" + str(graph[current][1]) + ")"
        #longestpath += ""
    costs[current] = current_cost
    longestpaths[current] = longestpath
    return current_cost,longestpath

def main():
    global number_articulations
    global labels
    global vars
    global debug
    global semanticcut_maxcost
    global semanticcut
    global cut_id
    global first_label
    global last_label
    global first_var
    global first_var_bd
    global last_var
    global heap_cuts
    global my_graph

    parser = argparse.ArgumentParser(description='generateSMTwcet')
    parser.add_argument("--nosummaries", help="do not add extra information to the SMT formula",action="store_true")
    parser.add_argument("--recursivecuts", help="add automatic recursive cuts",action="store_true")

    parser.add_argument('filename', type=str,
                   help='the file name')
    parser.add_argument('--matchingfile', type=str,
                   help='name of the matching file')
    parser.add_argument('--smtmatching', type=str,
                   help='name of the file matching labels to booleans')
    parser.add_argument('--cutsfile', type=str,
                   help='name of the cuts file')
    parser.add_argument('--printlongestsyntactic', type=str,
                   help='name of the file storing the longest syntactic path')
    parser.add_argument('--printcutslist', type=str,
                   help='name of the file that lists the different cuts, in order of difficulty')

    args = parser.parse_args()

    if args.matchingfile:
        usematching = True
        matchingfile = open(args.matchingfile, 'r')
        readmatchingfile(matchingfile)
    else:
        usematching = False

    if args.cutsfile:
        cutsfile = open(args.cutsfile, 'r')
        readcutsfile(cutsfile)

    add_summaries = not args.nosummaries
    #read file
    file = open(args.filename, 'r')
    smt_formula, blocks = file.read().rsplit('-------', 1)

    smt_declarations, smt_assertions = smt_formula.split('(assert')
    smt_declarations = smt_declarations.strip() + '\n'
    smt_assertions = '(assert \n' + smt_assertions.strip() + '\n'
    blocks = blocks.split('BasicBlock')

    # do the matching between block labels and block Booleans
    match_names(blocks)

    # init some variables
    extra_assertions = ''
    cost_sum = '(= cost (+'
    cost_max = 0

    # graph
    graph = {}
    edges = []
    sorted_keys = []



    nb_cuts = 0

    # for each block, create a constraint with a cost value
    for i in range(1, len(blocks)):
        id, instructions = blocks[i].split(':', 1)

        #var = 'bs_' + id[1:]
        var = id[1:]
        if var == "bd_0":
            var = "bs_0"
        if var == "bd_1":
            var = "bs_1"
        k = int(var.split("_")[1])
        label = labels[var]
        if instructions.find('<label>:') != -1:
            dominator = instructions.split('Dominator = ',1)[1].split('\n',1)[0]
        else:
            dominator = instructions.split('Dominator = ',1)[1].split('\n',1)[0]
        if dominator == first_var_bd:
            dominator = first_var

        if usematching:
            cost = getcost(label)
        else:
            cost = int(instructions.split(' ')[1])
        cost_max += cost
        var_cost = k
        cost_sum += ' c' + str(var_cost)
        if var == first_var:
            extra_assertions += '(= c' + str(var_cost) + ' ' + str(cost) + ')\n'
        else:
            if cost != 0:
                extra_assertions+='(= c' + str(var_cost) + ' (ite ' + var + ' ' + str(cost) + ' 0))\n'
            else:
                extra_assertions+='(= c' + str(var_cost) + ' 0)\n'
            #extra_assertions += '(or (and (= ' + var + ' true) (= c' + str(var_cost) + ' ' + str(cost) +\
            #                    ')) (and (= ' + var + ' false) (= c' + str(var_cost) + ' 0)))\n'
        smt_declarations += '(declare-fun c' + str(var_cost) + ' ()  Int)\n'

        # build graph incrementally
        sorted_keys.append(label)
        # graph[node] = [variable_string, cost, income_edges, outgoing_edges, max_path_cost, min_path_cost, count_paths, var_cost_id, timeDFS, lowDFS, preDFS, isArticulationPoint]
        #my_graph[var] = [label,cost,incoming_edges,outgoing_edges,dominator,count_paths]
        my_graph[var] = [label,cost,[],[],dominator, 'c' + str(var_cost),-1]

        #if i < len(blocks)-1:
        if instructions.find('br ') != -1:
            br_num = 0
            outgoing = instructions.split('br ')[1].split('label')
            for edge in outgoing:
                e = edge.strip();
                if len(e) >= 2 and e[0] == '%':
                    br_num = br_num + 1
                    if e[-1] == ',':
                        dest =e[1:-1]
                    else:
                        dest =e[1:]
                    edges.append((dest, label))
                    # add the cost of the transition
                    edge_name = "t_" + vars[label].split('_')[1] + "_" + vars[dest].split('_')[1]

                    if usematching:
                        tcost = getcost_transition(label,dest)
                    else:
                        tcost = 0
                    cost_max += cost
                    edge_cost = "c_" + vars[label].split('_')[1] + "_" + vars[dest].split('_')[1]
                    cost_sum += ' ' + edge_cost
                    smt_declarations += '(declare-fun ' + str(edge_cost) + ' ()  Int)\n'
                    #extra_assertions += '(or (and (= ' + edge_name + ' true) (= ' + str(edge_cost) + ' ' + str(tcost)
                    #extra_assertions += ')) (and (= ' + edge_name + ' false) (= ' + str(edge_cost) + ' 0)))\n'
                    extra_assertions+='(= ' + str(edge_cost) + ' (ite ' +\
                                        edge_name + ' ' + str(tcost) + ' 0))\n'


    # create the edges of the graph
    for e in edges:
        my_graph[vars[e[0]]][2].append(vars[e[1]])
        my_graph[vars[e[1]]][3].append(vars[e[0]])


    # insert the extra assertions in the formula
    smt_declarations += '(declare-fun cost ()  Int)\n'
    cost_sum += '))\n'
    extra_assertions += cost_sum

    #print json.dumps(my_graph)
    #sys.exit(0)
    # Find extra constraints from bifurcations using intermediate result of (Max, +) algorithm
    if args.recursivecuts:
        addautomaticcuts()
    if add_summaries:
        for node in my_graph:
            if len(my_graph[node][2]) > 1 :
                # this is an interesting point since it has more than 1 incoming edge
                costs = {}
                tcosts = {}
                node_dominator = my_graph[node][4]
                longest_path = get_summary_max(my_graph,node_dominator,node,node,costs,tcosts)
                constraint = ''
                constraint_size = 0
                for n in costs:
                    constraint += ' ' + my_graph[n][5]
                    constraint_size += 1
                for t in tcosts:
                    constraint += ' ' + get_tcost_var(t)
                    constraint_size += 1
                cut = "cut" + str(cut_id)

                heappush(heap_cuts,(constraint_size,cut,str(longest_path)))

                cut_id += 1
                smt_declarations += '(declare-fun ' + cut + ' ()  Int)\n'
                extra_assertions += '(= ' + cut + ' (+ ' + constraint + '))\n'
                extra_assertions += '(<= ' + cut + ' ' + str(longest_path) + '); between blocks ' + labels[node_dominator] + ' and ' + labels[node] + '\n'
                nb_cuts += 1
            #print my_graph[node][0] + "?"
            index = 0
            for t in tails:
                if my_graph[node][0] in t:
                    tail = my_graph[node][0]
                    #print ";" + tail + " is in tails"
                    position=t.index(tail)
                    head = heads[index][position]
                    head_var = vars[head]
                    tail_var = vars[tail]
                    costs = {}
                    tcosts = {}
                    node_dominator = my_graph[node][4]
                    longest_path = get_summary_max(my_graph,head_var,tail_var,tail_var,costs,tcosts)
                    constraint = ''
                    constraint_size = 0
                    for n in costs:
                        constraint += ' ' + my_graph[n][5]
                        constraint_size += 1
                    for t in tcosts:
                        constraint += ' ' + get_tcost_var(t)
                        constraint_size += 1
                    #semanticcut_maxcost[index] = max(semanticcut_maxcost[index],longest_path)
                    #semanticcut[index] += constraint

                    cut = "cut" + str(cut_id)
                    heappush(heap_cuts,(constraint_size,cut,str(longest_path)))

                    cut_id += 1
                    smt_declarations += '(declare-fun ' + cut + ' ()  Int)\n'
                    extra_assertions += '(= ' + cut + ' (+ ' + constraint + '))\n'
                    extra_assertions += '(<= ' + cut + ' ' + str(longest_path)+\
                    '); portion between ' + labels[head_var] + ' and ' + labels[tail_var] + '\n'
                    nb_cuts += 1
                index = index+1



    # global cost variable
    globalcosts = {}
    globaltcosts = {}
    longestpathsstrings = {}
    #debug = True
    longest_path, stringpath = get_summary_max_with_path(my_graph,first_var,last_var,last_var,globalcosts,globaltcosts,longestpathsstrings)
    #for n in globalcosts:
    #    print labels[n] + " : " + str(globalcosts[n])
    if add_summaries:
        constraint = ''
        for n in globalcosts:
            constraint += ' ' + my_graph[n][5]
        for t in globaltcosts:
            constraint += ' ' + get_tcost_var(t)
            #extra_assertions += '(<= (+' + constraint + ') ' + str(longest_path) + '); between blocks ' + labels[first_var] + ' and ' + labels[last_var] + ' (longest path)\n'
        extra_assertions += '(<= (+' + constraint + ') ' + str(longest_path) +')\n'
        extra_assertions += '(<= cost ' + str(longest_path) + '); longest path\n'

        for index in range(len(semanticcut_maxcost)):
            if semanticcut_maxcost[index] > 0:
                extra_assertions += '(<= (+' + semanticcut[index] + ') ' +\
                str(semanticcut_maxcost[index]) + '); semantic cut num' +\
                str(index) + '\n'

    smt_formula = smt_declarations + smt_assertions + '(assert (and (= ' + first_var + ' true) (= ' + last_var + ' true)))\n(assert\n(and\n' + extra_assertions + '))\n'

    print smt_formula
    print '; NB_PATHS = ' + str(getNumPaths(my_graph, vars[last_label]))
    print '; NB_PATHS_DIGITS = ' + str(len(str(getNumPaths(my_graph,vars[last_label]))))
    print '; NB_CUTS ' + str(nb_cuts)
    print '; LONGEST_PATH ' + str(longest_path)

    if args.smtmatching:
        smtmatching = open(args.smtmatching, 'w')
        for k in vars:
            smtmatching.write(vars[k]+','+k+'\n')
        smtmatching.close()

    if args.printlongestsyntactic:
        longestpath_file = open(args.printlongestsyntactic, 'w')
        longestpath_file.write(stringpath+'\n')
        longestpath_file.close()

    if args.printcutslist:
        cuts_file = open(args.printcutslist, 'w')
        for x in xrange(len(heap_cuts)):
            c = heappop(heap_cuts)
            cuts_file.write(c[1]+' '+c[2]+'\n')
        cuts_file.close()

    #print json.dumps(my_graph[last_var])
sys.setrecursionlimit(10000)
main()
