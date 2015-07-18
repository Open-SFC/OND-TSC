/**
 * Copyright (c) 2012, 2013  Freescale.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*File: tsc_ofplugin_1_3_ldef.h 
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 *
 */
   
  #define FLMOD_MSG_LEN_NW_PORT_OF_1_3  \
                 OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+ \
                 OFU_IN_PORT_FIELD_LEN+ \
                 OFU_ETH_DST_MAC_ADDR_MASK_LEN+ \
                 OFU_VLAN_VID_FIELD_LEN+ \
                 OFU_TUNNEL_ID_FIELD_LEN+ \
                 OFU_APPLY_ACTION_INSTRUCTION_LEN+ \
                 OFU_POP_VLAN_HEADER_ACTION_LEN+ \
                 OFU_WRITE_META_DATA_INSTRUCTION_LEN+ \
                 OFU_GOTO_TABLE_INSTRUCTION_LEN \

  #define FLMOD_MSG_LEN_VM_PORT_OF_1_3   \
                OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+ \
                OFU_IN_PORT_FIELD_LEN+ \
                OFU_ETH_DST_MAC_ADDR_MASK_LEN+ \
                OFU_VLAN_VID_MASK_LEN+ \
                OFU_APPLY_ACTION_INSTRUCTION_LEN+ \
                OFU_POP_VLAN_HEADER_ACTION_LEN+ \
                OFU_WRITE_META_DATA_INSTRUCTION_LEN+ \
                OFU_GOTO_TABLE_INSTRUCTION_LEN \

  #define FLMOD_MSG_LEN_VM_NS_PORT_OF_1_3  \
                OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+ \
                OFU_IN_PORT_FIELD_LEN+ \
                OFU_ETH_DST_MAC_ADDR_MASK_LEN+ \
                OFU_VLAN_VID_MASK_LEN+ \
                OFU_APPLY_ACTION_INSTRUCTION_LEN+ \
                OFU_POP_VLAN_HEADER_ACTION_LEN+ \
                OFU_WRITE_META_DATA_INSTRUCTION_LEN+ \
                OFU_GOTO_TABLE_INSTRUCTION_LEN \

int32_t tsc_ofplugin_v1_3_init(uint64_t domain_handle);

int32_t tsc_ofplugin_v1_3_uninit(uint64_t domain_handle);


