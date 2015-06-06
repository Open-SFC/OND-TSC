#working directory

OND_OF_DRVR_INST_DIR = /usr/local/ond_of_driver/1.0

tscapp_dir = $(PWD)

BIN_DIR  = $(tscapp_dir)/bin

ifeq ($(TARGET_PLATFORM), p2041rdb)
  LIBRARY_PATH=$(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb-tcbootstrap/usr/lib:$LIBRARY_PATH
  export LIBRARY_PATH
  OND_OF_LIB_DIR = ${ofproto_dir}/build/lib-p2041
OF_NCURSES_LIB = -L$(OND_OF_LIB_DIR) -lncurses
INCLUDE += -I $(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/include
INCLUDE += -I $(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/lib
INCLUDE += -I $(USDPAA_SDK_INSTALL_PATH)/build_p2041rdb_release/tmp/sysroots/p2041rdb/usr/lib/powerpc-fsl-linux/4.6.2

endif

ifeq ($(TARGET_PLATFORM), p4080ds)
  OND_OF_LIB_DIR = ${ofproto_dir}/build/lib-p4080
INCLUDE += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include
INCLUDE += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/lib
INCLUDE += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/powerpc-fsl_networking-linux/4.7.2

endif

ifeq ($(TARGET_PLATFORM), t4240qds)
OND_OF_LIB_DIR = ${ofproto_dir}/build/lib-t4240
INCLUDE  = -I$(SYSROOT_PATH)/usr/include
INCLUDE += -I$(SYSROOT_PATH)/usr/lib64
INCLUDE += -I$(SYSROOT_PATH)/usr/lib64/powerpc64-fsl_networking-linux/4.7.2
endif

ifeq ($(TARGET_PLATFORM), X86)
OND_OF_LIB_DIR = $(OND_OF_DRVR_INST_DIR)/lib 
  INCLUDE = -I/usr/include
endif

ifeq ($(TARGET_PLATFORM), X86-64bit)
OND_OF_LIB_DIR = $(OND_OF_DRVR_INST_DIR)/lib
  INCLUDE = -I/usr/include
endif


ifneq ($(TARGET_PLATFORM), p2041rdb)
INCLUDE += -I $(OND_OF_DRVR_INST_DIR)/include\
          -I $(OND_OF_DRVR_INST_DIR)/include/ofproto/src/include\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/common\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/infra/dm\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/infra/je\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/infra/transport\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/lxos\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/utils/dslib\
          -I $(OND_OF_DRVR_INST_DIR)/include/cminfra/include/utils/netutil\
          -I $(OND_OF_DRVR_INST_DIR)/include/ucmcbk/includes/global\
          -I $(OND_OF_DRVR_INST_DIR)/include/ucmcbk/includes/generated\
          -I $(OND_OF_DRVR_INST_DIR)/include/ucmcbk/cntlrucm/include\
          -I $(OND_OF_DRVR_INST_DIR)/include/ucmcbk/comm/include\
          -I $(OND_OF_DRVR_INST_DIR)/include/ucmcbk/dprm/include\
          -I $(OND_OF_DRVR_INST_DIR)/include/ucmcbk/crm/include\
          -I $(OND_OF_DRVR_INST_DIR)/include/third_party_opensrc/futex\
          -I $(OND_OF_DRVR_INST_DIR)/include/third_party_opensrc/openssl-1.0.0/include/openssl\
          -I $(tscapp_dir)/src/include

CFLAGS += $(INCLUDE) -Wall -D_GNU_SOURCE -g -O
endif

LIBS += -L $(OND_OF_LIB_DIR) -lurcu-mb -lurcu-common -lurcu
LIBS += -L $(OND_OF_LIB_DIR)/apps
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
