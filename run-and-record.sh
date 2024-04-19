#!/bin/bash

# start recording

# cd ~/schedviz/util
# sudo ./trace.sh -out "/home/hannahmanuela/traces" -capture_seconds 5 > /dev/null &

# run
cd ~/lnx-test/build

# # start load balancer
./lb > /dev/null  &
pid_lb=$!
echo $pid_lb
sleep 1

# # start dispatcher
sudo ./dispatcher &
pid_dispatcher=$!
echo $pid_dispatcher

sleep 1

# # start client
./website > /dev/null &


# # # kill them all
sleep 4
# kill -9 $pid_lb
# sleep 1
# kill -9 $pid_dispatcher



