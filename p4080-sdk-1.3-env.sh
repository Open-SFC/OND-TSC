export PSP_INSTALL_PATH=$PWD/switch
export JAVA_HOME=/projects/vortiqabuilds_nbk/SDN/jdk1.7.0_17
export PATH=$PATH:$JAVA_HOME/bin
export SRC_DIR=$PWD
export CLASSPATH=.:$JAVA_HOME/lib/dt.jar:$JAVA_HOME/lib/tools.jar
export BUILD_CONFIG=build/config/p4080ds-sdk1.3.config
export TARGET_HOST=powerpc-fsl-linux
export TARGET_BOX=p4080ds
export TARGET_PLATFORM=p4080ds
export SDK_INSTALL_PATH=/projects/vortiqabuilds_nbk/SDN/QorIQ-SDK-V1.3-20121114-yocto
export YOCTO_INSTALL_PATH=$SDK_INSTALL_PATH
export USDPAA_SDK_INSTALL_PATH=$YOCTO_INSTALL_PATH
export KSRC=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/work/p4080ds-fsl-linux/linux-qoriq-sdk-3.0.48-r5/git
export PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/i686-linux/usr/bin/ppce500mc-fsl-linux:$PATH 
export USDPAA_PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/work/ppce500mc-fsl-linux/usdpaa-git-r11/git
export SYSROOT_PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds
export PKG_CONFIG_SYSROOT_DIR=$SYSROOT_PATH
export ARCH=powerpc
export CROSS_COMPILE=$TARGET_HOST-
export XCFLAGS="-m32 -mhard-float  -mcpu=e500mc -fPIC --sysroot $USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds -I$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds/usr/include"
export XLDFLAGS="--sysroot $USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds"
#Run common environment script
. switch/build/env.sh
#below required for urcu-0.6.6 library build
export ac_cv_func_malloc_0_nonnull=yes
export CCLD=powerpc-fsl-linux-ld
export CC=$TARGET_HOST-gcc
export LD=$TARGET_HOST-ld
export CP=cp
export LTIB_DIR=$SDK_INSTALL_PATH
export KERNEL_MAJOR_VERSION=3.0
export KERNEL_MINOR_VERSION=48
export KERNEL_PATH=$KSRC
export USDPAA_PATH=$USDPAA_SDK_INSTALL_PATH
export AUTOCONF_DIR=$KERNEL_PATH
export MODPOST_PATH=$KERNEL_PATH/scripts/mod
export DOTCONFIG_PATH=$KERNEL_PATH
export KERNEL_SRC_PATH=$KERNEL_PATH
