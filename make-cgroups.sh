#!/bin/bash

# Check if argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <number_of_directories>"
    exit 1
fi

# Retrieve the number of directories to create
num_dirs=$1

# Create directories
for ((i=1; i<=$num_dirs; i++))
do
    sudo mkdir /sys/fs/cgroup/test-cg-$i
done

