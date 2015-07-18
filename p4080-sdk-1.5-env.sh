export SRC_DIR=$PWD
export JAVA_HOME=/projects/vortiqabuilds_nbk/SDN/jdk1.7.0_17
export PATH=$PATH:$JAVA_HOME/bin
export CLASSPATH=.:$JAVA_HOME/lib/dt.jar:$JAVA_HOME/lib/tools.jar
export BUILD_HOST=i686-pc-linux-gnu
export BUILD_CONFIG=build/config/p4080ds-sdk1.5.config
export TARGET_HOST=powerpc-fsl_networking-linux
export TARGET_BOX=p4080ds
export TARGET_PLATFORM=p4080ds
export YOCTO_INSTALL_PATH=/projects/vortiqabuilds_nbk/SDN/QorIQ-SDK-V1.5-20131219-yocto
export USDPAA_SDK_INSTALL_PATH=$YOCTO_INSTALL_PATH
export KSRC=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/work/p4080ds-fsl_networking-linux/linux-qoriq-sdk/3.8-r11.1/git
export PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/i686-linux/usr/bin/ppce500mc-fsl_networking-linux:$PATH
export USDPAA_PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/work/p4080ds-fsl_networking-linux/usdpaa/git-r4/git/
export SYSROOT_PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds
export SYSROOT_USR_LIB_PATH=$SYSROOT_PATH/usr/lib
export SYSROOT_EXTN_PATH=$SYSROOT_PATH/usr/lib/powerpc-fsl_networking-linux/4.7.3/
export BOOTSTRAP_LIB_PATH=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds-tcbootstrap/usr/lib
export ARCH=powerpc
export CROSS_COMPILE=$TARGET_HOST-
export XCFLAGS="-m32 -mhard-float  -mcpu=e500mc -fPIC --sysroot=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds -I$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds/usr/include"
export XLDFLAGS="--sysroot=$USDPAA_SDK_INSTALL_PATH/build_p4080ds_release/tmp/sysroots/p4080ds"
#below required for urcu-0.6.6 library build
export ac_cv_func_malloc_0_nonnull=yes
export CCLD=powerpc-fsl_networking-linux-ld
export CC=$TARGET_HOST-gcc
export LD=$TARGET_HOST-ld
export CP=cp
export LIB_BOARD_DIR=lib-p4080
export LIBTOOL_FILE_PATH=$SYSROOT_PATH/usr/bin/crossscripts/$TARGET_HOST-libtool
export SDK_VERSION=`echo $USDPAA_SDK_INSTALL_PATH | cut -d '/' -f 5 | cut -d '-' -f 3`
