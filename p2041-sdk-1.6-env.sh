export SRC_DIR=$PWD
export JAVA_HOME=/projects/vortiqabuilds_nbk/SDN/jdk1.7.0_17
export PATH=$PATH:$JAVA_HOME/bin
export CLASSPATH=.:$JAVA_HOME/lib/dt.jar:$JAVA_HOME/lib/tools.jar
export BUILD_CONFIG=build/config/p2041rdb-sdk1.6.config
export TARGET_HOST=powerpc-fsl-linux
export TARGET_BOX=p2041rdb
export TARGET_PLATFORM=p2041rdb
export YOCTO_INSTALL_PATH=/projects/sdkbuilds_nbk/SDN/SDK1.6/QorIQ-SDK-V1.6-20140619-yocto
export USDPAA_SDK_INSTALL_PATH=$YOCTO_INSTALL_PATH
export KSRC=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/work/p2041rdb-fsl-linux/linux-qoriq-sdk/3.12-r0/git
export PATH=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/sysroots/i686-linux/usr/bin/ppce500mc-fsl-linux:$PATH
export USDPAA_PATH=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/work/p2041rdb-fsl-linux/usdpaa/git-r0/git
export SYSROOT_PATH=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/sysroots/p2041rdb
export SYSROOT_EXTN_PATH=$SYSROOT_PATH/usr/lib/powerpc-fsl-linux/4.8.1/
export SYSROOT_USR_LIB_PATH=$SYSROOT_PATH/usr/lib
export BOOTSTRAP_LIB_PATH=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/sysroots/p2041rdb-tcbootstrap/usr/lib
export ARCH=powerpc
export CROSS_COMPILE=$TARGET_HOST-
export XCFLAGS="-m32 -mhard-float -mcpu=e500mc -fPIC --sysroot=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/sysroots/p2041rdb -I$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/include"
export XLDFLAGS="--sysroot=$USDPAA_SDK_INSTALL_PATH/build_p2041rdb_release/tmp/sysroots/p2041rdb"
#below required for urcu-0.6.6 library build
export ac_cv_func_malloc_0_nonnull=yes
export CCLD=powerpc-fsl-linux-ld
export CC=$TARGET_HOST-gcc
export LD=$TARGET_HOST-ld
export CP=cp
export LIB_BOARD_DIR=lib-p2041
export LIBTOOL_FILE_PATH=$SYSROOT_PATH/usr/bin/crossscripts/$TARGET_HOST-libtool
export SDK_VERSION=`echo $USDPAA_SDK_INSTALL_PATH | cut -d '/' -f 6 | cut -d '-' -f 3`

