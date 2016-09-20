#!/bin/bash

function usage () {
echo "
Usage:
./pagai.sh <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-o [FILE ]: name of the IR generated file
	-a        : arguments given to pagai
	-u        : unroll loops once
	-I [level]: inline functions
	-s        : silent mode
	-G        : generate the .dot CFG
	-p        : use the trunk version of pagai
	-t        : set a time limit (Pagai is killed after this time, default 800s)
"
}

PRINT=0
GRAPH=0
UNROLL=0
TIME_LIMIT=30
RESULT=
BITCODE=
PAGAI="pagai"
REQUIRED=0
SILENT=0
INLINE=0
ARGS=" "

while getopts "a:hpruGi:o:t:sI:" opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
		i)
			FILENAME=$OPTARG
			REQUIRED=1
			;;
		u)
			UNROLL=1
			;;
		p)
			PAGAI=pagai
			;;
		G)
			GRAPH=1
			;;
		s)
			SILENT=1
			;;
		a)
			ARGS=$OPTARG
			;;
		I)
			INLINE=$OPTARG
			;;
		t)
			TIME_LIMIT=$OPTARG
			;;
		o)
			BITCODE=$OPTARG
			;;
		r)
			RESULT=$OPTARG
			;;
        ?)
            usage
            exit
            ;;
     esac
done

if [ $REQUIRED -eq 0 ] ; then
	usage
	exit
fi

BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z $BITCODE ] ; then 
	if [ $UNROLL -eq 0 ] ; then
		BITCODE=/tmp/${NAME}.bc
	else
		BITCODE=/tmp/${NAME}_unroll.bc
	fi
fi

if [ $UNROLL -eq 1 ] ; then
	opt -mem2reg -inline -lowerswitch -loops  -loop-simplify -loop-rotate -lcssa -loop-unroll -unroll-count=1 $FILENAME -o $BITCODE
else
	opt -mem2reg -inline -lowerswitch $FILENAME -o $BITCODE
fi

if [ ! $INLINE -eq 0 ] ; then
opt -inline -inline-threshold=$INLINE -inlinehint-threshold=$INLINE  $BITCODE -o $BITCODE
fi

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg $BITCODE -o $BITCODE
	mv *.dot $DIR
	for i in `ls $DIR/*.dot` ; do
		dot -Tsvg -o $i.svg $i
	done
fi

NAME=`basename $BITCODE`
RESULT=/tmp/${NAME%%.*}.result

ulimit -t $TIME_LIMIT


$PAGAI -i $BITCODE $ARGS
xs=$?

case $xs in
 0) OUT="ok -- $NAME"
	 ;; # all fine
 *) if [ $xs -gt 127 ]; then
       OUT="killed -- $NAME"
    else
       OUT="error -- $NAME"
    fi
esac

if [ $SILENT -eq 0 ] ; then
	echo $OUT	
else
	echo $OUT  1>&2
fi

exit 0
