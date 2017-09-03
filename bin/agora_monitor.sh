#! /bin/bash

export LD_LIBRARY_PATH=/user/home/agora/third/memcached/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/user/home/agora/third/libevent/lib:$LD_LIBRARY_PATH

num=`ps -ef | grep agora | grep -v "agora_monitor" | grep -v "grep" | wc -l`

stime=`date +%Y%m%d%H%M%S`

if [[ $num -lt 1 ]]
then
	cpu=`ps aux | grep agora | grep -v "agora_monitor" | grep -v "grep" | awk '{print $3}'`
	mem=`ps aux | grep agora | grep -v "agora_monitor" | grep -v "grep" | awk '{print $4}'`
	echo "stat: cpu - "$cpu", mem - "$mem
	echo $stime": restart"
	./agora init.conf
fi

#./agora init.conf

