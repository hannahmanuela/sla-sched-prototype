#!/bin/bash


cd /sys/fs/cgroup

sudo mkdir one-digit-ms
echo 10000 | sudo tee one-digit-ms/cpu.weight

sudo mkdir two-digit-ms
echo 100 | sudo tee two-digit-ms/cpu.weight

sudo mkdir three-digit-ms
echo 1 | sudo tee three-digit-ms/cpu.weight


