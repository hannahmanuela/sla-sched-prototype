#!/bin/bash


cd /sys/fs/cgroup

echo "+cpu" | sudo tee cgroup.subtree_control
echo "+cpuset" | sudo tee cgroup.subtree_control

sudo mkdir one-digit-ms
echo 1000 | sudo tee one-digit-ms/cpu.weight

sudo mkdir two-digit-ms
echo 100 | sudo tee two-digit-ms/cpu.weight

sudo mkdir three-digit-ms
echo 1 | sudo tee three-digit-ms/cpu.weight


