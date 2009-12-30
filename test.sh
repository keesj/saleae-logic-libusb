#!/bin/sh
#set -e 
#set -x 

speeds="24MHz 16MHz 12MHz 8MHz 4MHz 2MHz 1MHz 500kHz 250kHz 200kHz"
timeouts="10 20 40 80 100 200"
buffer_sizes="512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288"

LOG=log.csv

new_log(){
	echo -n "#new log" > $LOG
	date  >> $LOG
}

add_entry(){
	text=$1
	echo -n "$text, " >> $LOG
}

new_entry(){
	echo >> $LOG
}

new_log
add_entry "counter,speed,buffer_count,timeout,buffer_size,cmd,sucess,success_1"
new_entry
COUNTER=0
for buffer_count in `seq 2 4`
do 
	for speed in  `echo $speeds`
	do
		for timeout in  `echo $timeouts`
		do 
			for buffer_size in  `echo $buffer_sizes`
			do 
				COUNTER=$(($COUNTER +1))
				add_entry $COUNTER
				add_entry "\"$speed\""
				add_entry $buffer_count
				add_entry $timeout
				add_entry $buffer_size
				cmd="./main -f out.log -r $speed -t $buffer_count -o $timeout -b $buffer_size"
				add_entry "\"$cmd\""
				echo -n $COUNTER $cmd
				if ($cmd 2>&1 )  > /dev/null
				then
					echo " OK"
					add_entry OK
					add_entry 1
				else
					echo " NOK"
					add_entry NOK
					add_entry 0
				fi
				new_entry
			done
		done
	#./main -f out.log -r 8MHz -n 8000000
	done
done
