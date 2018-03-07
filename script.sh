#!/bin/bash

sudo rmmod syscall.ko
make
sudo insmod syscall.ko
