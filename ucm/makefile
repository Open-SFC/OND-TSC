# Copyright  2012, 2013  Freescale Semiconductor
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

# File name:  makefile
# Author: Freescale Semiconductor    
# Date:   03/15/2013
# Description:

include ../rules.mk

SHARED_LIB = $(OND_OF_LIB_DIR)/apps

TARGET = $(BIN_DIR)/ucmtsccbk.o

CFLAGS += -DOF_CM_SUPPORT

$(BIN_DIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)
 
UCM_OBJS = $(BIN_DIR)/tsc_selector.o
UCM_OBJS += $(BIN_DIR)/nsc_repository_cbk.o
UCM_OBJS += $(BIN_DIR)/nsc_tbl3_repository_cbk.o
UCM_OBJS += $(BIN_DIR)/tsc_cli_init.o

all: $(TARGET)

$(TARGET): $(UCM_OBJS) 
	$(LD) -r -O2 $(UCM_OBJS) -o $@

clean:
	$(RM) $(UCM_OBJS) $(TARGET)

