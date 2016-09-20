#!/bin/bash

LATEX=1

cd $1

for k in `seq 0 3` ; do 
	TOTALRES[$k]=0
done


function diff_domains {
	for k in `seq 0 3` ; do 
		RES[$k]=0
	done
	for i in `find ./res -name "*\.res\.dcompare\.$1\.$2"` ; do
		if [ ! -z `tail -n 2 $i | grep MATRIX:`  ] ; then
			k=0
			for j in `tail -n 1 $i` ; do
				RES[$k]=$[${RES[$k]}+$j]
				TOTALRES[$k]=$[${TOTALRES[$k]}+$j]
				k=$(($k+1))
			done
		fi
	done
	TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
	for k in `seq 0 3` ; do 
		if [ $TOTAL -eq 0 ] ; then
			AVG[$k]=0
		else
			AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
		fi
	done
	echo -n ${AVG[1]} \& ${AVG[2]} \& ${AVG[3]}
}

function diff_project {
	if [ -d res ] ; then	
		echo -n "$1 & "
		#diff_domains $TECHNIQUE $D1 $D2
		#echo -n " \& "
		#diff_domains pk oct
		#echo -n " & "
		#diff_domains pk box
		#echo -n " & "
		#diff_domains oct box
		#echo -n " & "
		#diff_domains pk pkeq
		#echo -n " & "
		#diff_domains pk pkgrid
		#echo -n " & "
		diff_domains ppl_poly ppl_poly_bagnara
		echo " \\\\ \\hline"
	fi
}

for dir in */ ; do
	if [ -d $dir ] ; then
		cd $dir
		diff_project $dir
		cd ..
	fi
done

TOTAL=$[${TOTALRES[0]}+${TOTALRES[1]}+${TOTALRES[2]}+${TOTALRES[3]}]

echo TOTAL : 
for k in `seq 0 3` ; do 
	if [ $TOTALRES -eq 0 ] ; then
		AVG[$k]=0
	else
		AVG[$k]=`echo "scale=2;${TOTALRES[$k]}*100/$TOTAL"| bc`
	fi
done
echo -n ${AVG[1]} \& ${AVG[2]} \& ${AVG[3]}


