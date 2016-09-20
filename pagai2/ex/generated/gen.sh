#!/bin/bash
# usage: gen.sh nb defs tmpl
# nb: number of replications
# defs: variable definitions
# tmpl: code template


if test $# -ne 3; then
	echo "bad number of arguments" > /dev/stderr
	exit 1
fi

if ! test 0 -le $1; then
	echo "$1 is not a positive number"
	exit 1
fi

if test ! -f $2; then
	echo "cannot read file $3" > /dev/stderr
	exit 1
fi

if test ! -f $3; then
	echo "cannot read file $3" > /dev/stderr
	exit 1
fi

nb=$1
vardef=$2
tmpl=$3

out="$3_$1.c"

if test -f $out; then
	echo "output file $out exists" > /dev/stderr
	exit 1
fi

echo "int main() {" > $out

for (( i=0 ; $i < $nb ; i++ )); do
	cat $vardef | sed -e 's/^/\t/' -e "s/@/$i/g" >> $out
done

echo >> $out
echo "	for(;;) {" >> $out
for ((i=0 ; $i < $nb ; i++)); do
	cat $tmpl | sed -e 's/^/\t\t/' -e "s/@/$i/g" >> $out
done
echo "	}" >> $out

echo >> $out
echo "	return 0;" >> $out
echo "}" >> $out

exit 0
