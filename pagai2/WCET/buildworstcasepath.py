import argparse
import re
from sets import Set

# contains the name of llvm basicblock the semantic worst case trace goes through
Blocks = Set([])
# contains the name of llvm basicblock the syntactic worst case trace goes through
Blocks_syntactic = Set([])

# maps Boolean variable names to LLVM basicblock names
to_llvm = {}
# maps LLVM basicblock names to Boolean variable names
to_Boolean = {}
# reads the file that maps Boolean variables of the SMT encoding to
# the basicblock they are encoding in LLVM
def readbooleanmatchingfile(matchingfile):
    global to_llvm, to_Boolean
    for line in matchingfile:
        smtvar,llvmblock = line.split(',')
        smtvar = smtvar.replace("b_", "")
        smtvar = smtvar.replace("bs_", "")
        smtvar = smtvar.replace("bd_", "")
        llvmblock = llvmblock.replace("\n", "")
        to_llvm[smtvar] = llvmblock
        to_Boolean[llvmblock] = smtvar


def readmodeloflongesttrace(smtopt_output):
    model = False
    for line in smtopt_output:
        if "MODEL" in line:
            if not model:
                model = True
            else:
                return
        # we search only for the booleans associated to transitions
        if model and line.startswith('t_'):
            boolvar, boolvalue = line.split(' -> ')
            if not "true" in boolvalue:
                continue
            # we found a transition that is taken
            ignore,start,end = boolvar.split('_')
            Blocks.add(to_llvm[start])
            Blocks.add(to_llvm[end])


def loadlongestsyntacticpath(syntacticfile):
    for line in syntacticfile:
        if not ',' in line:
            continue
        source,dest = line.split(',')
        source = source.replace("(", "")
        source = source.replace(" ", "")
        dest = dest.replace(" ", "")
        dest = dest.replace(")\n", "")
        Blocks_syntactic.add(source)
        Blocks_syntactic.add(dest)

def producenewdot(dotfile):
    for line in dotfile:
        match=re.match('\tNode0x([0-9a-f]+) \[shape=record,label=\"\{([A-Za-z0-9_.]+):([^\n]*)', line)
        #match=re.match('@([A-Za-z0-9_.]+) =(?: external| internal| common)?(?: unnamed_addr)? (?:global|constant)? (float|double|i32|i16|i8|i1) ?([A-Z0-9.e+-x]+|true|false)?(?:, align 4)?', line)
        if match:
            nodeid = match.group(1)
            label = match.group(2)
            lastpart = match.group(3)
            attributes = 'shape=record,'
            if label in Blocks:
                attributes = attributes + 'style="filled", fillcolor="#ADD8E6", color=black,'
            if label in Blocks_syntactic:
                attributes = attributes + 'fontcolor="#204a87",'
            print '\tNode0x' + nodeid + ' [' + attributes + 'label=\"{' + label + ':' + lastpart + '\n'
        print line

def main():

    parser = argparse.ArgumentParser(description='buildworstcasepath')
    parser.add_argument('--smtmatching', type=str,
                   help='name of the file matching labels to booleans')
    parser.add_argument('--smtoptoutput', type=str,
                   help='output of smtopt that contains the model (should be run with -m option)')
    parser.add_argument('--longestsyntactic', type=str,
                   help='name of the file representing the longest syntactic path')
    parser.add_argument('--dotgraph', type=str,
                   help='dot graph')

    args = parser.parse_args()

    if args.smtmatching:
        matchingfile = open(args.smtmatching, 'r')
        readbooleanmatchingfile(matchingfile)
        matchingfile.close()

    if args.smtoptoutput:
        smtoptfile = open(args.smtoptoutput, 'r')
        readmodeloflongesttrace(smtoptfile)
        smtoptfile.close()

    if args.longestsyntactic:
        syntacticfile = open(args.longestsyntactic, 'r')
        loadlongestsyntacticpath(syntacticfile)
        syntacticfile.close()

    if args.dotgraph:
        dotfile = open(args.dotgraph, 'r')
        producenewdot(dotfile)
        dotfile.close()

main()
