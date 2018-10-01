#! /bin/sh

export CROSS_COMPILE=${HOME}/gcc-linaro-7.3.1/bin/arm-linux-gnueabihf-

export TRAINING_ROOT=${HOME}/opi/

export BUILD_KERNEL=${HOME}/build/

#export BUILD_ROOTFS=${TRAINING_ROOT}/rootfs

#export KERNEL_IMG=${BUILD_KERNEL}/arch/x86/boot/bzImage
#export ROOTFS_IMG=${TRAINING_ROOT}/rootfs.img

echo -e "\t CROSS_COMPILE \t = ${CROSS_COMPILE}"
echo -e "\t TRAINING_ROOT \t = ${TRAINING_ROOT}"
echo -e "\t BUILD_KERNEL \t = ${BUILD_KERNEL}"
#echo -e "\t BUILD_ROOTFS \t = ${BUILD_ROOTFS}"

#cp ${BUILD_ROOTFS}/images/rootfs.ext2 ${TRAINING_ROOT}/rootfs.img
#qemu-system-i386 -kernel ${KERNEL_IMG} -append "root=/dev/sda" -hda ${ROOTFS_IMG} -redir tcp:8022::22 &
