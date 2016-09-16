#!/bin/bash
if [ -z $1 ] ; then
	echo error : please use the command \"make ndiff\"
	exit
fi

DIR=`pwd`
cd $1


for k in `seq 0 3` ; do 
	ALL[$k]=0
	TOTAL_ITERATIONS[$k]=0
done

for k in `seq 0 2` ; do 
	TOTAL_FUNCTIONS[$k]=0
done

for k in `seq 0 7` ; do 
	TOTAL_TIME[$k]=0
done

LATEX=0
PRINT_TIME=0


for dir in * ; do
	if [ -d $dir ] ; then
		cd $dir
		for k in `seq 0 3` ; do 
			RES[$k]=0
			ITERATIONS[$k]=0
		done
		for k in `seq 0 2` ; do 
			FUNCTIONS[$k]=0
		done
		for k in `seq 0 7` ; do 
			TIME[$k]=0
		done
		if [ -d res ] ; then	
			for i in res/*.res.narrow ; do
				basename=`basename $i`
				basename=${basename%%.*}
			
				if [ ! -z `tail -n 2 $i | head -n 1 | grep MATRIX:`  ] ; then
					k=0
					for j in `tail -n 1 $i` ; do
						RES[$k]=$[${RES[$k]}+$j]
						k=$(($k+1))
					done
				fi

				if [ ! -z `tail -n 16 $i | head -n 1 | grep FUNCTIONS`  ] ; then
					k=0
					for j in `tail -n 15 $i | head -n 1` ; do
						FUNCTIONS[$k]=$[${FUNCTIONS[$k]}+$j]
						k=$(($k+1))
					done
				fi

				if [ ! -z `tail -n 13 $i | head -n 1 | grep ITERATIONS`  ] ; then
					k=0
					for j in `tail -n 12 $i | head -n 2` ; do
						ITERATIONS[$k]=$[${ITERATIONS[$k]}+$j]
						k=$(($k+1))
					done
				fi

				if [ ! -z `tail -n 22 $i| head -n 1 | grep TIME`  ] ; then
					k=0
					for j in `tail -n 21 $i | head -n 4` ; do
						TIME[$k]=$[${TIME[$k]}+$j]
						k=$(($k+1))
					done
				fi
			
			done
			TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
			if [ $TOTAL -ne 0 ] ; then
				for k in `seq 0 3` ; do 
					AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
					TOTAL_ITERATIONS[$k]=$[${TOTAL_ITERATIONS[$k]}+${ITERATIONS[$k]}]
					ALL[$k]=$[${RES[$k]}+${ALL[$k]}]
				done
				for k in `seq 0 2` ; do 
					TOTAL_FUNCTIONS[$k]=$[${TOTAL_FUNCTIONS[$k]}+${FUNCTIONS[$k]}]
				done
				for k in `seq 0 7` ; do 
					TOTAL_TIME[$k]=$[${TOTAL_TIME[$k]}+${TIME[$k]}]
				done
				TIME_1=`echo "scale=0;(${TIME[0]}*1000000+${TIME[1]})/1000000" | bc`
				TIME_2=`echo "scale=0;(${TIME[2]}*1000000+${TIME[3]})/1000000" | bc`
				TIME_3=`echo "scale=0;(${TIME[4]}*1000000+${TIME[5]})/1000000" | bc`
				TIME_4=`echo "scale=0;(${TIME[6]}*1000000+${TIME[7]})/1000000" | bc`
				echo "############################"
				echo "$dir"
				echo "############################"
				echo "             ASC      DESC"
				echo "IMPROVED ${ITERATIONS[0]} ${ITERATIONS[1]}"
				echo "CLASSIC  ${ITERATIONS[2]} ${ITERATIONS[3]}"
				echo ""
				echo "FUNCTIONS"
				echo "${FUNCTIONS[0]} ${FUNCTIONS[1]} ${FUNCTIONS[2]}"
				echo ""
				echo "TIME"
				echo "IMPROVED" $TIME_1 
				echo "CLASSIC " $TIME_2 
				echo ""
				echo "SAME RESULT:"
				echo "IMPROVED " $TIME_3 
				echo "CLASSIC " $TIME_4 
				echo ""
				echo ${RES[0]} ${RES[1]} ${RES[2]} ${RES[3]}
				echo ${AVG[0]} ${AVG[1]} ${AVG[2]} ${AVG[3]}
			fi
		fi
		cd ..
	fi
done


echo
echo "############################"
echo "TOTAL"
echo "############################"
TOTAL=$[${ALL[0]}+${ALL[1]}+${ALL[2]}+${ALL[3]}]
for k in `seq 0 3` ; do 
ALL[$k]=`echo "scale=2;${ALL[$k]}*100/$TOTAL"| bc`
done

TOTAL_TIME_1=`echo "scale=0;(${TOTAL_TIME[0]}*1000000+${TOTAL_TIME[1]})/1000000" | bc`
TOTAL_TIME_2=`echo "scale=0;(${TOTAL_TIME[2]}*1000000+${TOTAL_TIME[3]})/1000000" | bc`
TOTAL_TIME_3=`echo "scale=0;(${TOTAL_TIME[4]}*1000000+${TOTAL_TIME[5]})/1000000" | bc`
TOTAL_TIME_4=`echo "scale=0;(${TOTAL_TIME[6]}*1000000+${TOTAL_TIME[7]})/1000000" | bc`
echo
echo "             ASC      DESC"
echo "IMPROVED ${TOTAL_ITERATIONS[0]} ${TOTAL_ITERATIONS[1]}"
echo "CLASSIC  ${TOTAL_ITERATIONS[2]} ${TOTAL_ITERATIONS[3]}"
echo
echo "FUNCTIONS"
echo "${TOTAL_FUNCTIONS[0]} ${TOTAL_FUNCTIONS[1]} ${TOTAL_FUNCTIONS[2]}"
echo
echo "TIME"
echo "IMPROVED" $TOTAL_TIME_1 
echo "CLASSIC " $TOTAL_TIME_2 
echo ""
echo "SAME RESULT:"
echo "IMPROVED " $TOTAL_TIME_3 
echo "CLASSIC " $TOTAL_TIME_4 
echo ""
echo EQ LT GT UN
echo ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}			

cd $DIR
