#!/bin/bash

#safely create and delete pipes
pipe=hcsr04_data
trap "rm -rf $pipe" EXIT
if [[ ! -p $pipe ]]
then
        mkfifo $pipe
fi

#launch gp2y
sudo ./hcsr04 -q 1000 $pipe &
#sleep 1
#read in while and echo result in stdout and file
while true
do
        read data <$pipe
	#echo $data
        echo "scale=5; $data*170" | bc
        echo "scale=5; $data*170" | bc > result.txt
done
