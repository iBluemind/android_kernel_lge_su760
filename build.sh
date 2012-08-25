#!/bin/bash

HWVER=rev_d
CLEAN=""


WORK_PATH=`pwd`
IMAGE_PART1='../../../../../android/images/p920/part1'


for argument in "$@"
do
	case "$argument" in
    hw*)
      HWVER=${argument:3} 
      ;;
	"clean")
	  CLEAN=clean 
	  make clean
	  exit 0
	  ;;
	esac
done


echo "##############building kernel HWVER = ${HWVER} ####################"

make ${CLEAN} ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- cosmo_${HWVER}_defconfig
make ${CLEAN} -j8 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- uImage

#if [ ! -d ${IMAGE_PART1} ]
#then
#	mkdir ${IMAGE_PART1}
#fi

#cp -vf arch/arm/boot/uImage ${IMAGE_PART1}
