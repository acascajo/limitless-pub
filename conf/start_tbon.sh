#!/bin/bash



host_file=""
master_address=""
port_master=""
port_listener=""
while getopts "h?m:p:r:s:p:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage: ./start.sh -m <lds_address> -r <lds_port> -p <port_listener> -f <lda_hostfile>"
        exit 0
        ;;
    m)  master_address=$OPTARG
        ;;
    r)	port_master=$OPTARG
	      ;;
    f)  host_file=$OPTARG
        ;;
    p)  port_listener=$OPTARG
	      ;;
    esac
done



if [ ! -f "$host_file" ]; then
	echo "It is not a valid file"
	exit
fi

# LDS deployment
ssh ${master_address} "DISPLAY=:0 nohup ${HOME}/limitless-pub/test_server/build/Test_server -p ${port_master} < /dev/null > /dev/null 2> /dev/null &"

#LDAs deployment
while read IP; do
	echo "${IP}"
	ssh -n ${IP} "DISPLAY=:0 nohup ${HOME}/limitless-pub/test_server/build/Test_server -p ${port_listener} -r ${port_master} -s ${master_address} < /dev/null > /dev/null 2> /dev/null &"
done < ${host_file}
