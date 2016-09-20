#!/bin/bash

if [ -z $1 ] ; then
	echo error : please use the command \"make ndiff\"
	exit
fi

DIR=`pwd`
cd $1

for k in `seq 0 3` ; do 
	ALL[$k]=0
done

LATEX=0
PRINT_TIME=0


for k in `seq 0 3` ; do 
	RES[$k]=0
	ITERATIONS[$k]=0
done

for k in `seq 0 7` ; do 
	TIME[$k]=0
done

for k in `seq 0 2` ; do 
	FUNCTIONS[$k]=0
done
#IGNORED=0


for i in *.res.narrow ; do
	basename=`basename $i`
	basename=${basename%%.*}

	if [ ! -z `tail -n 2 $i | grep MATRIX:`  ] ; then
		k=0
		for j in `tail -n 1 $i` ; do
			RES[$k]=$[${RES[$k]}+$j]
			k=$(($k+1))
		done
	fi

	if [ ! -z `tail -n 13 $i | grep ITERATIONS`  ] ; then
		k=0
		for j in `tail -n 12 $i | head -n 2` ; do
			ITERATIONS[$k]=$[${ITERATIONS[$k]}+$j]
			k=$(($k+1))
		done
	fi

	if [ ! -z `tail -n 16 $i | grep FUNCTIONS`  ] ; then
		k=0
		for j in `tail -n 15 $i | head -n 1` ; do
			FUNCTIONS[$k]=$[${FUNCTIONS[$k]}+$j]
			k=$(($k+1))
		done
	fi

	#if [ ! -z `tail -n 18 $i | grep IGNORED:`  ] ; then
	#	NFUNC=`tail -n 17 $i | head -n 1`
	#	IGNORED=$[$IGNORED+$NFUNC]
	#fi

	if [ ! -z `tail -n 22 $i | grep TIME`  ] ; then
		k=0
		for j in `tail -n 21 $i | head -n 4` ; do
			TIME[$k]=$[${TIME[$k]}+$j]
			k=$(($k+1))
		done
	fi

done
TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
for k in `seq 0 3` ; do 
	AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
done
TIME_1=`echo "scale=0;(${TIME[0]}*1000000+${TIME[1]})/1000000" | bc`
TIME_2=`echo "scale=0;(${TIME[2]}*1000000+${TIME[3]})/1000000" | bc`
TIME_3=`echo "scale=0;(${TIME[4]}*1000000+${TIME[5]})/1000000" | bc`
TIME_4=`echo "scale=0;(${TIME[6]}*1000000+${TIME[7]})/1000000" | bc`
if [ $LATEX -eq 0 ] ; then
	echo "#####"
	echo $dir
	#echo "IGNORED : $IGNORED / $FUNCTIONS"
	#echo TIME
	echo "             ASC      DESC"
	echo "IMPROVED ${ITERATIONS[0]} ${ITERATIONS[1]}"
	echo "CLASSIC  ${ITERATIONS[2]} ${ITERATIONS[3]}"
	echo ""
	echo "FUNCTIONS"
	echo "EQ  " ${FUNCTIONS[0]}
	echo "NE  " ${FUNCTIONS[1]}
	echo "TOT " ${FUNCTIONS[2]}
	echo ""
	echo "TIME"
	echo "IMPROVED " $TIME_1 
	echo "CLASSIC  " $TIME_2 
	echo ""
	echo "SAME RESULT:"
	echo "IMPROVED " $TIME_3 
	echo "CLASSIC  " $TIME_4 
	#echo $TIME_PF PF
	#echo $TIME_C LW+PF
	#echo $TIME_DIS DIS
	echo ""
	echo $TOTAL
	echo EQ LT GT UN
	echo ${RES[0]} ${RES[1]} ${RES[2]} ${RES[3]}		
	echo ""
	echo EQ LT GT UN
	echo ${AVG[0]} ${AVG[1]} ${AVG[2]} ${AVG[3]}
	echo "#####"
fi
for k in `seq 0 3` ; do 
	ALL[$k]=$[${RES[$k]}+${ALL[$k]}]
done


#echo "#####"
#echo "TOTAL"
#echo EQ LT GT UN
#echo ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}
#echo "#####"
#
#TOTAL=$[${ALL[0]}+${ALL[1]}+${ALL[2]}+${ALL[3]}]
#for k in `seq 0 3` ; do 
#	ALL[$k]=`echo "scale=2;${ALL[$k]}*100/$TOTAL"| bc`
#done
#
#echo "#####"
#echo "TOTAL"
#echo EQ LT GT UN
#echo G/S      ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}			
#echo "#####"

cd $DIR
