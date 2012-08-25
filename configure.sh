#!/bin/bash

HWVER=rev_c
CLEAN=""

export PATH=../../../../toolchain/arm-2010q1/bin:${PATH}

#echo "##############configure kernel HWVER = ${HWVER} ####################"

ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- make menuconfig 

