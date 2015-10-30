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

TARGET = $(BIN_DIR)/traffic_steering-app.o

$(BIN_DIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(TARGET)

OBJS =  $(BIN_DIR)/tsc_init.o\
        $(BIN_DIR)/tsc_controller.o\
        $(BIN_DIR)/tsc_nvm_modules.o\
        $(BIN_DIR)/tsc_nsc_iface.o\
        $(BIN_DIR)/tsc_nsc_iface_zone_left.o\
        $(BIN_DIR)/tsc_nsc_iface_zone_right.o\
        $(BIN_DIR)/tsc_nsc_core.o\
        $(BIN_DIR)/tsc_nsc_cnbind.o\
        $(BIN_DIR)/tsc_nsc_notify.o\
        $(BIN_DIR)/tsc_nvm_vxlan.o\
        $(BIN_DIR)/tsc_vxn_views_init.o\
	$(BIN_DIR)/tsc_vxn_views_api.o\
        $(BIN_DIR)/nicira_tun_ext.o\
        $(BIN_DIR)/tsc_ofv1_3_plugin.o\
     #  $(BIN_DIR)/tsc_bintree.o\
        $(BIN_DIR)/list_selectors.o\
        $(BIN_DIR)/list_repository.o\

$(TARGET) : $(OBJS) 
	$(LD) -r $(OBJS) -o $(TARGET) 

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJS)       
  
