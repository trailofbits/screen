# screen
The goal of this project are to create a LLVM pass which runs over the S2N codebase on each commit and performs compiler assisted invariant detection, this lets us find every such deviation automatically, on every checkin.


Current S2N defenses:
- constant time routines to maintain speed
- if any errors in client<->server connection: randomization [10,30]
- human review for memory, want: taint analysis to verify code paths are constant, memory access patterns side channels

Purpose of this tool, using annotations:

1. A-B no branches
2. A-C no branches that depend on secret memory
3. A-D branch that depend on secret memory: print upper bound of set (e.g., proof that no new memory access patterns)

Targeting: arm, x86

## Running the pass
Annotations are as follows:
For full function anaylsis use:

__attribute__((annotate("screen_function_paths"))) 

For analysis in between two instructions use:
__attribute__((annotate("screen_paths_start"))) char a = 'a';
__attribute__((annotate("screen_paths_end"))) char b = 'b';


## Example output
```
[+] Running screen pass...
SCreening Paths of Program: tests/test1.bc
	[-] Found Ending Annotation
	InstEnd = b
	[-] Found Starting Annotation
	InstStart = a

Detected sensitive code region, tracking code paths for function: foo

Detected sensitive code region, tracking code paths for function: main

[+] Dumping annotation path results...
In between annotated variables
	[ 5 ] Instructions
	[ 0 ] Branch Instructions

[+] Dumping function path results...
Function: foo => Branches: 0
Function: main => Branches: 3
```
