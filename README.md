# screen
The goal of this project are to create a LLVM pass which runs over the S2N codebase on each commit and performs compiler assisted invariant detection.

This lets us find every control-flow and variable range deviation automatically, on every checkin.


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
Include the screen.h header file in all builds of LLVM bitcode.

Annotations are as follows:
For full function anaylsis use:

SCREEN(screen_function_paths) 

For analysis in between two instructions use:
SCREEN_START(screen_paths_start) char a = 'a';
SCREEN_END(screen_paths_end) char b = 'b';

## Opt Command Line Flags (Manditory)
```
For the SCreen stats pass:

-screen
	Runs main pass

-screen-output=<string>                                        
	Provide an output file for screen output

-screen-start-symbol=<string>                                  
	Provide a symbol in the program to treat as the _start, , usually main for most cases

-screen-annotations=<file>
        Provide an external definition file for code segments under analysis. Lines in the format of: `{func/start/end} {name} {file} [lineno]`

For the Invariant analysis pass:

-invariant_analysis                                          
	Variable Invariant Analysis Pass

-invariant-output=<string>                                     
	Provide an output file for screen output

-invariant-debug (optional) 
	Print extra debug information

```

## Example walkthrough
```
sophia@litterbox:~/SCR/screen/ $ ./build/llvm/bin/clang -I./include/ -o test.bc test.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wunused-parameter

sophia@litterbox:~/SCR/screen/ $ ./build/llvm/bin/opt -load build/lib/screen.dylib test.bc -o test_transformed.bc -screen -screen-output OUTPUT -screen-start-symbol main

[+] Compiling tests...
[+] Running screen pass...
SCreening Paths of Program: test.bc

[-] Using start symbol: main
[-] Found Ending Annotation
	InstEnd = b
[-] Found Starting Annotation
	InstStart = a


[ STARTING MAIN ANALYSIS ]
Detected sensitive code region, tracking code paths for function: foo
Detected sensitive code region, tracking code paths for function: main

[+] Dumping annotation path results...
In between annotated variables
	[ 5 ] Instructions
	[ 0 ] Branch Instructions

[+] Dumping function path results...
In annotated function: foo
	[ 5 ] Instructions
	[ 0 ] Branch Instructions
In annotated function: main
	[ 28 ] Instructions
	[ 3 ] Branch Instructions
[ CallInst CFG ]
Pulling out CallInst paths for each possible program execution path

PATH [0]
main() -> printf() -> fun1() -> llvm.var.annotation() -> printf() -> llvm.var.annotation() -> printf() -> printf()
PATH [1]
main() -> printf() -> foo() -> printf() -> printf()

[+] Verifying paths:
B1: main-> printf-> fun1-> anno-> printf-> anno-> printf-> printf
B2: main-> printf-> foo-> printf-> printf

sophia@litterbox:~/SCR/screen/ $ ./pagai/src/pagai -i test.bc --output-bc-v2 test_annotated.bc 

sophia@litterbox:~/SCR/screen/ $ ./build/llvm/bin/opt -load ./build/lib/range.dylib -invariant_analysis -invariant-output=OUTPUT_INVARIANTS test_annotated.bc -o test_fin.bc

sophia@litterbox:~/SCR/screen/ $ cat OUTPUT_INVARIANTS
[{ "main-19" : 
  "3-i >= 0;i >= 0;" }]
  
```
# Setup
1. For the initial setup, run quick_start.sh
1. Edit source/screen.cpp
1. To build it after changes, run build.sh
1. To test it, edit test.sh and run test.sh

# Copyright Attribution
Screen utilizes code from the Pagai project (http://pagai.forge.imag.fr/) Copyright or © or Copr. Julien Henry, David Monniaux, Matthieu Moy, Rahul Nanda (2011 - 2016)
