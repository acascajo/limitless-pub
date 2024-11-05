#!/bin/bash



host_file=""
master_address=""
while getopts "h?f:m:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    m)  master_address=$OPTARG
        ;;
    f)  host_file=$OPTARG
        ;;
    esac
done



if [ ! -f "$host_file" ]; then
	echo "It is not a valid file"
	exit
fi

#kill LDS
ssh ${master_address} "pkill -9 Test_server"

#kill LDAs
while read IP; do
	echo "${IP}"
	ssh -n ${IP} "pkill -9 Test_server"
done < ${host_file}

ssh ${master_address} "pkill -9 Test_server"
