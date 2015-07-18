export SRC_DIR=$PWD
export JAVA_HOME=/projects/vortiqabuilds_nbk/SDN/jdk1.7.0_17
export PATH=$PATH:$JAVA_HOME/bin
export CLASSPATH=.:$JAVA_HOME/lib/dt.jar:$JAVA_HOME/lib/tools.jar
export BUILD_HOST=i686-pc-linux-gnu
export BUILD_CONFIG=build/config/t4240rdb-64b-sdk1.6.config
export TARGET_HOST=powerpc64-fsl-linux
export TARGET_BOX=t4240rdb
export TARGET_PLATFORM=t4240rdb
export YOCTO_INSTALL_PATH=/projects/sdkbuilds_nbk/SDN/SDK1.6/QorIQ-SDK-V1.6-20140619-yocto
export USDPAA_SDK_INSTALL_PATH=$YOCTO_INSTALL_PATH
export KSRC=$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/work/t4240rdb_64b-fsl-linux/linux-qoriq-sdk/3.12-r0/git
export PATH=$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/sysroots/i686-linux/usr/bin/ppc64e6500-fsl-linux:$PATH
export USDPAA_PATH=$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/work/t4240rdb_64b-fsl-linux/usdpaa/git-r0/git
export SYSROOT_PATH=$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/sysroots/t4240rdb-64b
export SYSROOT_USR_LIB_PATH=$SYSROOT_PATH/usr/lib64
export SYSROOT_EXTN_PATH=$SYSROOT_PATH/usr/lib64/powerpc64-fsl-linux/4.8.1
export BOOTSTRAP_LIB_PATH=$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/sysroots/t4240rdb-64b-tcbootstrap/usr/lib64
export ARCH=powerpc
export CROSS_COMPILE=$TARGET_HOST-
export XCFLAGS="-m64 -mhard-float  -mcpu=e6500 -fPIC --sysroot=$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/sysroots/t4240rdb-64b -I$USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/sysroots/t4240rdb-64b/usr/include"
export XLDFLAGS="--sysroot $USDPAA_SDK_INSTALL_PATH/build_t4240rdb-64b_release/tmp/sysroots/t4240rdb-64b"

#below required for urcu-0.6.6 library build
export ac_cv_func_malloc_0_nonnull=yes
export CCLD=$TARGET_HOST-ld
export CC=$TARGET_HOST-gcc
export LD=$TARGET_HOST-ld
export CP=cp
export LIB_BOARD_DIR=lib-t4240
export LIBTOOL_FILE_PATH=$SYSROOT_PATH/usr/bin/crossscripts/$TARGET_HOST-libtool
export SDK_VERSION=`echo $USDPAA_SDK_INSTALL_PATH | cut -d '/' -f 6|cut -d '-' -f 3`
