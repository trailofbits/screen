#!/bin/bash

for inode in `ls`
do
if
	[ -d $inode ]
then
		echo "===================================="
		echo $inode
		echo
		cp move.sh $inode
		cd $inode
		
		./move.sh
		echo "End of move"
		echo "===================================="
		cd ..
fi
done
 
