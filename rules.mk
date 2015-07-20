#working directory

ondir_platform_inst_dir = /usr/local/ondir_platform/2.0

tscapp_dir = $(PWD)

BIN_DIR  = $(tscapp_dir)/bin

ifeq ($(TARGET_PLATFORM), p2041rdb)
  LIBRARY_PATH=$(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb-tcbootstrap/usr/lib:$LIBRARY_PATH
  export LIBRARY_PATH
  ond_pf_lib_dir = ${ofproto_dir}/build/lib-p2041
OF_NCURSES_LIB = -L$(ond_pf_lib_dir) -lncurses
INCLUDE += -I $(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/include
INCLUDE += -I $(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/lib
INCLUDE += -I $(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/lib/powerpc-fsl-linux/4.6.2

endif

ifeq ($(TARGET_PLATFORM), p4080ds)
  ond_pf_lib_dir = ${ofproto_dir}/build/lib-p4080
INCLUDE += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include
INCLUDE += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/lib
INCLUDE += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/powerpc-fsl_networking-linux/4.7.2

endif

ifeq ($(TARGET_PLATFORM), t4240qds)
ond_pf_lib_dir = ${ofproto_dir}/build/lib-t4240
INCLUDE  = -I$(SYSROOT_PATH)/usr/include
INCLUDE += -I$(SYSROOT_PATH)/usr/lib64
INCLUDE += -I$(SYSROOT_PATH)/usr/lib64/powerpc64-fsl_networking-linux/4.7.2
endif

ifeq ($(TARGET_PLATFORM), X86)
ond_pf_lib_dir = $(ondir_platform_inst_dir)/lib 
  INCLUDE = -I/usr/include
endif

ifeq ($(TARGET_PLATFORM), X86-64bit)
ond_pf_lib_dir = $(ondir_platform_inst_dir)/lib
  INCLUDE = -I/usr/include
endif


ifneq ($(TARGET_PLATFORM), p2041rdb)
INCLUDE += -I $(ondir_platform_inst_dir)/include\
          -I $(ondir_platform_inst_dir)/include/ofproto/src/include\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/common\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/infra/dm\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/infra/je\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/infra/transport\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/lxos\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/utils/dslib\
          -I $(ondir_platform_inst_dir)/include/cminfra/include/utils/netutil\
          -I $(ondir_platform_inst_dir)/include/ucmcbk/includes/global\
          -I $(ondir_platform_inst_dir)/include/ucmcbk/includes/generated\
          -I $(ondir_platform_inst_dir)/include/ucmcbk/cntlrucm/include\
          -I $(ondir_platform_inst_dir)/include/ucmcbk/comm/include\
          -I $(ondir_platform_inst_dir)/include/ucmcbk/dprm/include\
          -I $(ondir_platform_inst_dir)/include/ucmcbk/crm/include\
          -I $(ondir_platform_inst_dir)/include/third_party_opensrc/futex\
          -I $(ondir_platform_inst_dir)/include/third_party_opensrc/openssl-1.0.0/include/openssl\
          -I $(ondir_platform_inst_dir)/include/third_party_opensrc/urcu\
          -I $(tscapp_dir)/src/include

CFLAGS += $(INCLUDE) -Wall -D_GNU_SOURCE -g -O
endif

LIBS += -L $(ond_pf_lib_dir) -lurcu-mb -lurcu-common -lurcu
LIBS += -L $(ond_pf_lib_dir)/apps
LIBS += -lpthread
LIBS += -lrt
LIBS += -ldl
LIBS += -rdynamic

APP_FLAGS = -DNICIRA_EXT_SUPPORT

CFLAGS += $(MGMT_FLAGS)
CFLAGS += $(APP_FLAGS)
CFLAGS += $(FUTEX_FLAGS)
CFLAGS += -g
CFLAGS += -fPIC
