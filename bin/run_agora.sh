#!/bin/bash

export LD_LIBRARY_PATH=/user/home/agora/third/memcached/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/user/home/agora/third/libevent/lib:$LD_LIBRARY_PATH

./agora init.conf
