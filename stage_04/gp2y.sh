#!/bin/bash

#safely create and delete pipes
pipe=gp2y_data
trap "rm -rf $pipe" EXIT
if [[ ! -p $pipe ]]
then
        mkfifo $pipe
fi

#launch gp2y
./gp2y -q 1000 $pipe &

#read in while and echo result in stdout and file
while true
do
        read data <$pipe
	#echo $data
        echo "scale=5; 0.567/($data*187.5*0.000001 - 0.133)" | bc
        echo "scale=5; 0.567/($data*187.5*0.000001 - 0.133)" | bc > result.txt
done
