#!/bin/bash

function usage () {
echo "
Usage:
./compile_llvm.sh <OPTION> -i [FILE]

OPTION :
	-h        : help

	-o [FILE ]: name of the output file
	-p        : print the llvm bytecode
	-g        : debug version
	-M        : compile in 64 bits
	-I        : inline functions
	-G        : generate the .dot CFG
	-O        : apply optimisations to the bytecode
	-N        : no undefined behaviour trap handlers
"
}

PRINT=0
GRAPH=0
OPT=0
DEBUG=0
INLINE=0
NOTRAP=0
M64BITS=0

while getopts "hpgi:o:OINM" opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
        p)
			PRINT=1
            ;;
        I)
			INLINE=1
            ;;
        N)
			NOTRAP=1
            ;;
        M)
			M64BITS=1
            ;;
		i)
			FILENAME=$OPTARG
			;;
		G)
			GRAPH=1
			;;
		g)
			DEBUG=1
			;;
		O)
			OPT=1
			;;
		o)
			OUTPUT=$OPTARG
			;;
        ?)
            usage
            exit
            ;;
     esac
done

if [ -z "$FILENAME" ]; then
	echo "Please provide a filename with -i FILE."
	exit 1
fi

BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z "$OUTPUT" ] ; then 
	OUTPUT=${DIR}/${NAME}.bc
fi

if [ $NOTRAP -eq 1 ] ; then
 TRAP=" "
else
 TRAP=" -fsanitize=undefined -fsanitize=local-bounds "
fi

if [ $M64BITS -eq 1 ] ; then
 TRAP="$TRAP  -m64 "
fi
TRAP="$TRAP -Wno-return-type "

QUIET=" -Wno-implicit-function-declaration -Wno-parentheses-equality "
if [ $DEBUG -eq 1 ] ; then
	echo clang $TRAP -emit-llvm -I .. -g -c $FILENAME -o $OUTPUT
	clang $TRAP -emit-llvm $QUIET -I .. -I . -g -c $FILENAME -o $OUTPUT
else
	echo clang $TRAP -emit-llvm -I ..  -c $FILENAME -o $OUTPUT
	clang $TRAP -emit-llvm $QUIET -I .. -I . -c $FILENAME -o $OUTPUT
fi
if [ $OPT -eq 1 ] ; then
	INLINE=1
	opt -mem2reg -inline -lowerswitch -loops  -loop-simplify -loop-rotate -lcssa -loop-unroll -unroll-count=1 $OUTPUT -o $OUTPUT
else
	opt -mem2reg -lowerswitch $OUTPUT -o $OUTPUT
fi

if [ $INLINE -eq 1 ] ; then
	opt -inline -inline-threshold=150000 $OUTPUT -o $OUTPUT
	opt -internalize $OUTPUT -o $OUTPUT -internalize-public-api-list=main
	opt -inline -inline-threshold=150000 $OUTPUT -o $OUTPUT
fi

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg $OUTPUT -o $OUTPUT
	mv *.dot $DIR
	for i in `ls $DIR/*.dot` ; do
		dot -Tsvg -o $i.svg $i
	done
fi

if [ $PRINT -eq 1 ] ; then
	opt -S $OUTPUT
fi
