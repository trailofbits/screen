#!/bin/bash
if [ -z $1 ] ; then
	echo error : please use the command \"make cdiff\"
	exit
fi

DIR=`pwd`
cd $1


for k in `seq 0 27` ; do 
	ALL[$k]=0
done

LATEX=1
PRINT_TIME=0

for dir in `ls` ; do
	if [ -d $dir ] ; then
		cd $dir
		for k in `seq 0 27` ; do 
			RES[$k]=0
		done
		for k in `seq 0 9` ; do 
			TIME[$k]=0
		done
		FUNCTIONS=0
		IGNORED=0
		if [ -d res ] ; then	
			for i in `ls res/*.res.compare` ; do
				basename=`basename $i`
				basename=${basename%%.*}
			
				if [ ! -z `tail -n 8 $i | grep MATRIX:`  ] ; then
					k=0
					for j in `tail -n 7 $i` ; do
						RES[$k]=$[${RES[$k]}+$j]
						k=$(($k+1))
					done
				fi

				if [ ! -z `tail -n 21 $i | grep FUNCTIONS:`  ] ; then
					NFUNC=`tail -n 20 $i | head -n 1`
					FUNCTIONS=$[$FUNCTIONS+$NFUNC]
				fi

				if [ ! -z `tail -n 18 $i | grep IGNORED:`  ] ; then
					NFUNC=`tail -n 17 $i | head -n 1`
					IGNORED=$[$IGNORED+$NFUNC]
				fi

				if [ ! -z `tail -n 15 $i | grep TIME:`  ] ; then
					k=0
					for j in `tail -n 14 $i | head -n 5` ; do
						TIME[$k]=$[${TIME[$k]}+$j]
						k=$(($k+1))
					done
				fi
			
			done
			TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
			for k in `seq 0 27` ; do 
				if [ $TOTAL -ne 0 ] ; then
					AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
				else
					AVG[$k]=0
				fi
			done
			TIME_S=`echo "scale=0;(${TIME[0]}*1000000+${TIME[1]})/1000000" | bc`
			TIME_LW=`echo "scale=0;(${TIME[2]}*1000000+${TIME[3]})/1000000" | bc`
			TIME_PF=`echo "scale=0;(${TIME[4]}*1000000+${TIME[5]})/1000000" | bc`
			TIME_C=`echo "scale=0;(${TIME[6]}*1000000+${TIME[7]})/1000000" | bc`
			TIME_DIS=`echo "scale=0;(${TIME[8]}*1000000+${TIME[9]})/1000000" | bc`
			if [ $LATEX -eq 0 ] ; then
				echo "#####"
				echo $dir
				echo "IGNORED : $IGNORED / $FUNCTIONS"
				echo ""
				echo TIME
				echo $TIME_S S
				echo $TIME_LW LW
				echo $TIME_PF PF
				echo $TIME_C LW+PF
				echo $TIME_DIS DIS
				echo ""
				echo $TOTAL
				echo EQ LT GT UN
				echo ${RES[0]} ${RES[1]} ${RES[2]} ${RES[3]}		LW/S
				echo ${RES[4]} ${RES[5]} ${RES[6]} ${RES[7]}		PF/S
				echo ${RES[8]} ${RES[9]} ${RES[10]} ${RES[11]}		PF/LW
				echo ${RES[12]} ${RES[13]} ${RES[14]} ${RES[15]}		LW+PF/PF
				echo ${RES[16]} ${RES[17]} ${RES[18]} ${RES[19]}		LW+PF/LW
				echo ${RES[20]} ${RES[21]} ${RES[22]} ${RES[23]}		LW+PF/S
				echo ${RES[24]} ${RES[25]} ${RES[26]} ${RES[27]}		DIS/LW+PF
				echo ""
				echo EQ LT GT UN
				echo ${AVG[0]} ${AVG[1]} ${AVG[2]} ${AVG[3]}		LW/S
				echo ${AVG[4]} ${AVG[5]} ${AVG[6]} ${AVG[7]}		PF/S
				echo ${AVG[8]} ${AVG[9]} ${AVG[10]} ${AVG[11]}		PF/LW
				echo ${AVG[12]} ${AVG[13]} ${AVG[14]} ${AVG[15]}		LW+PF/PF
				echo ${AVG[16]} ${AVG[17]} ${AVG[18]} ${AVG[19]}		LW+PF/LW
				echo ${AVG[20]} ${AVG[21]} ${AVG[22]} ${AVG[23]}		LW+PF/S
				echo ${AVG[24]} ${AVG[25]} ${AVG[26]} ${AVG[27]}		DIS/LW+PF
				echo "#####"
			else
				if [ $PRINT_TIME -eq 1 ] ; then
					echo $dir \& $TIME_S \& $TIME_LW \& $TIME_PF \& $TIME_C \& \
					$TIME_DIS \\\\ 
				else
					echo $dir \& ${AVG[1]} \& ${AVG[2]} \& ${AVG[3]} \& ${AVG[5]} \& ${AVG[6]}\
					\& ${AVG[7]} \& ${AVG[9]} \& ${AVG[10]} \& ${AVG[11]} \& ${AVG[13]}\
					\& ${AVG[14]} \& ${AVG[15]} \& ${AVG[17]} \& ${AVG[18]} \&\
					${AVG[19]} \& ${AVG[25]} \& ${AVG[26]} \& ${AVG[27]} \\\\ \\hline
				fi
			fi
			for k in `seq 0 27` ; do 
			ALL[$k]=$[${RES[$k]}+${ALL[$k]}]
			done
		fi
		cd ..
	fi
done


echo "#####"
echo "TOTAL"
echo EQ LT GT UN
echo ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}		LW/S
echo ${ALL[4]} ${ALL[5]} ${ALL[6]} ${ALL[7]}		PF/S
echo ${ALL[8]} ${ALL[9]} ${ALL[10]} ${ALL[11]}		PF/LW
echo ${ALL[12]} ${ALL[13]} ${ALL[14]} ${ALL[15]}		LW+PF/PF
echo ${ALL[16]} ${ALL[17]} ${ALL[18]} ${ALL[19]}		LW+PF/LW
echo ${ALL[20]} ${ALL[21]} ${ALL[22]} ${ALL[23]}		LW+PF/S
echo ${ALL[24]} ${ALL[25]} ${ALL[26]} ${ALL[27]}		DIS/LW+PF
echo "#####"

TOTAL=$[${ALL[0]}+${ALL[1]}+${ALL[2]}+${ALL[3]}]
for k in `seq 0 27` ; do 
ALL[$k]=`echo "scale=2;${ALL[$k]}*100/$TOTAL"| bc`
done

echo "#####"
echo "TOTAL"
echo EQ LT GT UN
echo G/S      ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}			
echo PF/S      ${ALL[4]} ${ALL[5]} ${ALL[6]} ${ALL[7]}			
echo PF/G     ${ALL[8]} ${ALL[9]} ${ALL[10]} ${ALL[11]}			
echo G+PF/PF  ${ALL[12]} ${ALL[13]} ${ALL[14]} ${ALL[15]}		
echo G+PF/G  ${ALL[16]} ${ALL[17]} ${ALL[18]} ${ALL[19]}		
echo G+PF/S   ${ALL[20]} ${ALL[21]} ${ALL[22]} ${ALL[23]}		
echo DIS/G+PF ${ALL[24]} ${ALL[25]} ${ALL[26]} ${ALL[27]}		
echo "#####"
#echo ${ALL[0]} ${ALL[4]} ${ALL[8]} ${ALL[12]} ${ALL[16]} ${ALL[20]} ${ALL[24]}
#echo ${ALL[1]} ${ALL[5]} ${ALL[9]} ${ALL[13]} ${ALL[17]} ${ALL[21]} ${ALL[25]}
#echo ${ALL[2]} ${ALL[6]} ${ALL[10]} ${ALL[14]} ${ALL[18]} ${ALL[22]} ${ALL[26]}
#echo ${ALL[3]} ${ALL[7]} ${ALL[11]} ${ALL[15]} ${ALL[19]} ${ALL[23]} ${ALL[27]}

cd $DIR
