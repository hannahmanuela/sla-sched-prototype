#!/bin/bash

# start recording

cd ~/schedviz/util
sudo ./trace.sh -out "/users/hmng/traces" -capture_seconds 5 > /dev/null &

# run
cd ~/lnx-test/build

# # start load balancer
./lb > /dev/null &
pid_lb=$!
echo $pid_lb
sleep 0.2

# # start dispatcher
sudo ./dispatcher > /dev/null &
pid_dispatcher=$!
echo $pid_dispatcher

sleep 0.2

# start client
./website-client > /dev/null &


#wait for it to run, then kill them all
sleep 4
kill -9 $pid_lb



