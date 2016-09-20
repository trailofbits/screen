#!/bin/bash

function usage () {
echo "
Usage:
./pagai.sh <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-a        : arguments given to pagai
	-s        : silent mode
	-S        : SV-Comp mode
	-p        : specify the pagai executable
	-t        : set a time limit (Pagai is killed after this time, default 1200s)
"
}

PRINT=0
TIME_LIMIT=1000
MEMORY_LIMIT=6000000
PAGAI="pagai"
REQUIRED=0
SILENT=0
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
		p)
			PAGAI=$OPTARG
			;;
		s)
			SILENT=1
			;;
		a)
			ARGS=$OPTARG
			;;
		t)
			TIME_LIMIT=$OPTARG
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

ulimit -t $TIME_LIMIT 
ulimit -m $MEMORY_LIMIT -v $MEMORY_LIMIT

echo $PAGAI -i $FILENAME $ARGS
$PAGAI -i $FILENAME $ARGS
xs=$?

case $xs in
 0) OUT="ok -- $FILENAME"
	 ;; # all fine
 *) if [ $xs -gt 127 ]; then
       OUT="killed -- $FILENAME"
    else
       OUT="error -- $FILENAME"
    fi
esac

if [ $SILENT -eq 0 ] ; then
	echo $OUT	
else
	echo $OUT  1>&2
fi

exit 0
