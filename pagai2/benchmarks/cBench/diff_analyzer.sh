#!/bin/bash


for path in $(ls -R)
do
if
	[ -d $path ]
then
	if [ -d $path/res ] ; then
		echo "===================================="

		for k in `seq 0 27` ; do 
			RES[$k]=0
		done
	
		for i in `ls $path/res/*.res.compare` ; do
			basename=`basename $i`
			basename=${basename%%.*}
	
			if [ ! -z `tail -n 8 $i | grep MATRIX:`  ] ; then
				k=0
				for j in `tail -n 7 $i` ; do
					RES[$k]=$[${RES[$k]}+$j]
					k=$(($k+1))
				done
			fi
		done

		echo $path
		echo EQ LT GT UN
		echo ${RES[0]} ${RES[1]} ${RES[2]} ${RES[3]}		LW/S
		echo ${RES[4]} ${RES[5]} ${RES[6]} ${RES[7]}		PF/S
		echo ${RES[8]} ${RES[9]} ${RES[10]} ${RES[11]}		PF/LW
		echo ${RES[12]} ${RES[13]} ${RES[14]} ${RES[15]}		LW+PF/PF
		echo ${RES[16]} ${RES[17]} ${RES[18]} ${RES[19]}		LW+PF/LW
		echo ${RES[20]} ${RES[21]} ${RES[22]} ${RES[23]}		LW+PF/S
		echo ${RES[24]} ${RES[25]} ${RES[26]} ${RES[27]}		DIS/LW+PF

		echo "===================================="
	fi
fi
done

