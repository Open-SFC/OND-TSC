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
 **/

#include "controller.h"
#include "dprm.h"
#include "nsrm.h"
#include "nsrmldef.h"
#include "crmapi.h"
#include "tsc_nsc_core.h" 
#include "port_status_mgr.h" 
#include "tsc_nvm_modules.h"

extern uint64_t tsc_domain_handle;
extern struct tsc_nvm_module_interface tsc_nvm_modules[SUPPORTED_NW_TYPE_MAX + 1];
extern uint32_t vn_nsc_info_offset_g;

#define SELECTOR_PRIMARY   1
#define SELECTOR_SECONDARY 2

#define NW_TYPE_OFFSET     60
#define PKT_ORIGIN_OFFSET  56
#define PKT_ORIGIN_MASK    0x0F00000000000000

int32_t nsrm_test_display_service_vms_in_chain(struct  l2_connection_to_nsinfo_bind_node* cn_bind_entry_p,uint8_t selector_type);
int32_t nsc_test(struct ofl_packet_in *pkt_in);
int32_t nsc_tst1(struct ofl_packet_in *pkt_in, uint8_t table_id,uint64_t,uint64_t);
void    nsc_add_nw_unicast_port1_sw1();
void    nsc_add_nw_unicast_port1_sw2();
void    nsc_add_nw_unicast_port1_sw3();

void nsc_add_sw1vm_port1();
void nsc_add_sw2vm_port1();
void nsc_add_sw3vm_port1();

uint8_t sw1vm_mac1[6] = {0xfa,0x16,0x3e,0x28,0xcb,0x83};
uint8_t sw2vm_mac1[6] = {0xfa,0x18,0x3e,0x28,0xcd,0x83};
uint8_t sw3vm_mac1[6] = {0xfa,0x16,0x3e,0xfd,0x61,0x3e};

struct  dprm_datapath_notification_data  dprm_notification_data;

struct  ofl_packet_in packet1,packet2,packet3,packet4,packet5,packet6,packet7,packet8,packet1a,packet5a;

struct nsrm_nwservice_object_key          nwservice_key_object1 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object1[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object2 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object2[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object3 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object3[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object4 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object4[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object5 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object5[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object6 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object6[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object7 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object7[2] = {};
struct nsrm_nwservice_object_key          nwservice_key_object8 ={};
struct nsrm_nwservice_object_config_info  nwservice_config_object8[2] = {};

struct nsrm_nwservice_attach_key          attach_key1 = {};
struct nsrm_nwservice_attach_key          attach_key2 = {};
struct nsrm_nwservice_attach_key          attach_key3 = {};
struct nsrm_nwservice_attach_key          attach_key4 = {};
struct nsrm_nwservice_attach_key          attach_key5 = {};
struct nsrm_nwservice_attach_key          attach_key6 = {};
struct nsrm_nwservice_attach_key          attach_key7 = {};
struct nsrm_nwservice_attach_key          attach_key8 = {};
struct nsrm_nwservice_attach_key          attach_key9 = {};
struct nsrm_nwservice_attach_key          attach_key10 = {};

struct nsrm_nschain_object_key          nschain_key_object1 ={};
struct nsrm_nschain_object_config_info  nschain_config_object1[5] = {};
struct nsrm_nschain_object_key          nschain_key_object2 ={};
struct nsrm_nschain_object_config_info  nschain_config_object2[5] = {};
struct nsrm_nschain_object_key          nschain_key_object3 ={};
struct nsrm_nschain_object_config_info  nschain_config_object3[5] = {};

struct nsrm_nschainset_object_key          nschainset_key_object1 ={};
struct nsrm_nschainset_object_config_info  nschainset_config_object1[2] = {};
struct nsrm_nschainset_object_key          nschainset_key_object2 ={};
struct nsrm_nschainset_object_config_info  nschainset_config_object2[2] = {};

struct nsrm_nschain_selection_rule_key               key_info1 = {};
struct nsrm_nschain_selection_rule_config_info       config_info1[11] = {};
struct nsrm_nschain_selection_rule_key               key_info2 = {};
struct nsrm_nschain_selection_rule_config_info       config_info2[11] = {};
struct nsrm_nschain_selection_rule_key               key_info3 = {};
struct nsrm_nschain_selection_rule_config_info       config_info3[11] = {};
struct nsrm_nschain_selection_rule_key               key_info4 = {};
struct nsrm_nschain_selection_rule_config_info       config_info4[11] = {};

struct nsrm_nwservice_bypass_rule_key               key_infob1 = {};
struct nsrm_nwservice_bypass_rule_config_info       config_infob1[12] = {};
struct nsrm_nwservice_bypass_rule_key               key_infob2 = {};
struct nsrm_nwservice_bypass_rule_config_info       config_infob2[12] = {};
struct nsrm_nwservice_bypass_rule_key               key_infob3 = {};
struct nsrm_nwservice_bypass_rule_config_info       config_infob3[12] = {};
struct nsrm_nwservice_bypass_rule_key               key_infob4 = {};
struct nsrm_nwservice_bypass_rule_config_info       config_infob4[12] = {};

struct nsrm_bypass_rule_nwservice_key               bypass_nwservice_key1 = {};
struct nsrm_bypass_rule_nwservice_key               bypass_nwservice_key2 = {};
struct nsrm_bypass_rule_nwservice_key               bypass_nwservice_key3 = {};
struct nsrm_bypass_rule_nwservice_key               bypass_nwservice_key4 = {};
struct nsrm_bypass_rule_nwservice_key               bypass_nwservice_key5 = {};

struct nsrm_l2nw_service_map_key          l2nw_key_object1 ={};
struct nsrm_l2nw_service_map_config_info  l2nw_config_object1[2] = {};
struct nsrm_l2nw_service_map_key          l2nw_key_object2 ={};
struct nsrm_l2nw_service_map_config_info  l2nw_config_object2[2] = {};

struct dprm_switch_info switch_info1,switch_info2,switch_info3;
struct dprm_datapath_general_info datapath_info1,datapath_info2,datapath_info3;

struct crm_vm_config_info vm_config_info_1={},vm_config_info_2={},vm_config_info_3={},vm_config_info_4={},vm_config_info_5={};
struct crm_vm_config_info vm_config_info_6={},vm_config_info_7={},vm_config_info_8={},vm_config_info_9={},vm_config_info_10={};
struct crm_vm_config_info vm_config_info_11={},vm_config_info_12={},vm_config_info_13={},vm_config_info_14={},vm_config_info_15={};

struct nsrm_nwservice_instance_key si1_key,si1a_key,si2_key,si3_key,si4_key,si5_key,si6_key,si7_key,si8_key,si9_key,si10_key,si11_key,si12_key,si13_key,si14_key,si15_key;

struct crm_vn_config_info config_vn_info={};
struct crm_port_notification_data notification_data = {};

struct crm_attribute_name_value_pair  attr_info1 = {};
struct crm_attribute_name_value_pair  attr_info2 = {};

struct   crm_attribute_name_value_output_info attr_output1  = {};
struct   crm_attribute_name_value_output_info attr_output2  = {};

uint8_t pkt_in_1[54] = {0x01,0x02,0x03,0x78,0x79,0x80,
                        0x01,0x02,0x03,0x78,0x79,0x81,0x81,0x00,0x10,0x17,0x81,0x00,0x10,0x16,
                        0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */  
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x01,0x0d,
                        0xc0,0xa8,0x02,0x0b,  /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,
                        0x00,0x0c,0x00,0x00,
                        0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */ 

uint8_t pkt_in_2[50] = {0x01,0x02,0x03,0x78,0x79,0x80,
                        0x01,0x02,0x03,0x78,0x79,0x81,0x81,0x00,0x10,0x19,
                        0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x01,0x0e,
                        0xc0,0xa8,0x02,0x0c, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,
                        0x00,0x0c,0x00,0x00,
                        0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */

uint8_t pkt_in_3[46] = {0x01,0x02,0x03,0x78,0x79,0x80,0x01,0x02,0x03,0x78,0x79,0x81,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x01,0x0f,0xc0,0xa8,0x02,0x0d, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */

uint8_t pkt_in_4[46] = {0x01,0x02,0x03,0x78,0x79,0x80,0x01,0x02,0x03,0x78,0x79,0x81,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x01,0x0a,0xc0,0xa8,0x02,0x0e, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */


uint8_t pkt_in_5[46] = {0x01,0x02,0x03,0x78,0x79,0x80,0x01,0x02,0x03,0x78,0x79,0x81,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x03,0x0b,0xc0,0xa8,0x04,0x0f, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */


uint8_t pkt_in_6[46] = {0x01,0x02,0x03,0x78,0x79,0x80,0x01,0x02,0x03,0x78,0x79,0x81,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x03,0x0c,0xc0,0xa8,0x04,0x0a, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */


uint8_t pkt_in_7[46] = {0x01,0x02,0x03,0x78,0x79,0x80,0x01,0x02,0x03,0x78,0x79,0x81,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x03,0x0d,0xc0,0xa8,0x04,0x0b, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */


uint8_t pkt_in_8[46] = {0x01,0x02,0x03,0x78,0x79,0x80,0x01,0x02,0x03,0x78,0x79,0x81,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x03,0x0e,0xc0,0xa8,0x04,0x0c, /* Src IP, DST IP */
                        0x12,0x34,0x56,0x78,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */

uint8_t pkt_in_1a[54] = {0x01,0x02,0x03,0x78,0x79,0x81,
                        0x01,0x02,0x03,0x78,0x79,0x80,0x81,0x00,0x10,0x17,0x81,0x00,0x10,0x16,
                        0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x02,0x0b,
                        0xc0,0xa8,0x01,0x0d, /* Src IP, DST IP */
                        0x56,0x78,0x12,0x34,
                        0x00,0x0c,0x00,0x00,
                        0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */

uint8_t pkt_in_5a[46] = {0x01,0x02,0x03,0x78,0x79,0x81,0x01,0x02,0x03,0x78,0x79,0x80,0x08,0x00, /* src mac dst mac IP protocol */
                        0x45,0x00,0x00,0x20, /* ver 4 etc,length = 32 bytes */
                        0x00,0x01,0x40,0x00, /* Identification 1 4000 is about fragment details no fragment offset = 0 */
                        0xff,0x11,0x00,0x00, /* TTL = 255,UDP protocol , checksum = 0 */
                        0xc0,0xa8,0x04,0x0f, /* Src IP, DST IP */
                        0xc0,0xa8,0x03,0x0b,
                        0x56,0x78,0x12,0x34,0x00,0x0c,0x00,0x00,0x89,0xab,0xcd,0xef}; /* UDP Total size 12 bytes SP,DP,Len = 12 bytes, checksum = 0, data = last 4 bytes */

    
uint64_t sw_handle1 , sw_handle2 , sw_handle3 , dp_handle1 , dp_handle2,
         dp_handle3, vm_handle1 , vm_handle2,vm_handle3,vm_handle4,vm_handle5,
         vm_handle6 , vm_handle7,vm_handle8,vm_handle9,vm_handle10,
         vm_handle11 , vm_handle12,vm_handle13,vm_handle14,vm_handle15,
         port_handle1,port_handle2,port_handle3,port_handle4,port_handle5,
         port_handle6,port_handle7,port_handle8,port_handle9,port_handle10,
         port_handle11,port_handle12,port_handle13,port_handle14,port_handle15;

struct ofl_port_desc_info port_desc , port_desc4;
struct ofl_port_desc_info port_desc1,port_desc2,port_desc3;

uint64_t port_hndl;
struct   dprm_port_info port_inf1; 

void dp_port_added_notifications_cbk(uint32_t dprm_notification_type,
                                     uint64_t datapath_handle,
                                     struct   dprm_datapath_notification_data dprm_notification_data,
                                     void*    callback_arg1,
                                     void*    callback_arg2)
{
  OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"entered");
  if(dprm_notification_type == DPRM_DATAPATH_PORT_ADDED)
  {
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"DPRM_DATAPATH_PORT_ADDED");
  }
}

uint64_t sw1_nw_uni_port1_handle,sw2_nw_uni_port1_handle,sw3_nw_uni_port1_handle;

uint64_t sw1vm_port1_handle,sw2vm_port1_handle,sw3vm_port1_handle;

uint8_t vm_port_mac[6];

void nsrm_test();

void nsrm_test()
{
   int32_t ret_val;
   uint64_t nschainset_handle_1,nschainset_handle_2,nschainset_handle_3, nschainset_handle_4,
            nschain_object_handle_1,nschain_object_handle_2;
            
   uint64_t crm_vn_handle;
   char     mac_buf[20];
 
/* CRM configuration */
  strcpy(switch_info1.name,"switch_1");
  switch_info1.switch_type = PHYSICAL_SWITCH;
  switch_info1.ipv4addr.baddr_set = TRUE;
  switch_info1.ipv4addr.addr = 0xc0a80c0c;
  strcpy(switch_info1.switch_fqdn,"http:/nsm/fsl.com.");

  ret_val = dprm_register_switch(&switch_info1,&sw_handle1);

  strcpy(switch_info2.name,"switch_2");
  switch_info2.switch_type = PHYSICAL_SWITCH;
  switch_info2.ipv4addr.baddr_set = TRUE;
  switch_info2.ipv4addr.addr = 0xc0a80c0d;

  strcpy(switch_info2.switch_fqdn,"http:/nsm/fsl.com.");

  dprm_register_switch(&switch_info2,&sw_handle2);

  strcpy(switch_info3.name,"switch_3");
  switch_info3.switch_type = PHYSICAL_SWITCH;
  switch_info3.ipv4addr.baddr_set = TRUE;
  switch_info3.ipv4addr.addr = 0xc0a80c0e;
  strcpy(switch_info3.switch_fqdn,"http:/nsm/fsl.com.");

  dprm_register_switch(&switch_info3,&sw_handle3);
  
  datapath_info1.dpid = 01;
  strcpy(datapath_info1.datapath_name,"br-int");
  strcpy(datapath_info1.switch_name,"switch_1");
 
  dprm_register_datapath(&datapath_info1,sw_handle1,tsc_domain_handle,&dp_handle1);

  datapath_info2.dpid = 02;
  strcpy(datapath_info2.datapath_name,"br-int");
  strcpy(datapath_info2.switch_name,"switch_2");

  dprm_register_datapath(&datapath_info2,sw_handle2,tsc_domain_handle,&dp_handle2);

  datapath_info3.dpid = 03;
  strcpy(datapath_info3.datapath_name,"br-int");
  strcpy(datapath_info3.switch_name,"switch_3");

  dprm_register_datapath(&datapath_info3,sw_handle3,tsc_domain_handle,&dp_handle3);

/* CRM configuration end. */
   strcpy(config_vn_info.nw_name,"VN1");
   strcpy(config_vn_info.tenant_name,"t1");
   config_vn_info.nw_type = VXLAN_TYPE;
   config_vn_info.nw_params.vxlan_nw.nid = 0x123456;
   config_vn_info.nw_params.vxlan_nw.service_port = 4789;
   ret_val = crm_add_virtual_network(&config_vn_info,&crm_vn_handle);
   if(ret_val != OF_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"Failed to add VN1");
   }
   else
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"Successfully added VN1");     
   } 

   nsc_add_nw_unicast_port1_sw1();
   nsc_add_nw_unicast_port1_sw2();

   /* Add port to dprm */
   strcpy(port_inf1.port_name,"port_vm1");
   port_inf1.port_id = 0;
   dprm_add_port_to_datapath(dp_handle1,&port_inf1,&port_hndl);

   strcpy(port_desc1.name,"port_vm1");
   port_desc1.port_no = 0x123478;

   ret_val = of_add_port_to_dp(dp_handle1,&port_desc1);
   if(ret_val == DPRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - port_vm1 Successful in adding port");
   }
   else
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - port_vm1 Failed to add port");
   }

   strcpy(port_desc2.name,"sw1uniport");
   port_desc2.port_no = 0x123489;
   
   ret_val = of_add_port_to_dp(dp_handle1,&port_desc2);
   if(ret_val == DPRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - sw1uniport Successful in adding port");
   }
   else
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - sw1uniport Failed to add port");
   }

   strcpy(port_desc3.name,"sw3uniport");
   port_desc3.port_no = 0x123491;

   ret_val = of_add_port_to_dp(dp_handle3,&port_desc3);
   if(ret_val == DPRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - sw3uniport Successful in adding port");
   }
   else
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - sw3uniport Failed to add port");
   }
  
   nsc_add_nw_unicast_port1_sw3();

   strcpy(port_desc.name,"port_vm6");
   port_desc.port_no = 0x123456;

   ret_val = of_add_port_to_dp(dp_handle1,&port_desc);
   if(ret_val == DPRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - port_vm6 Successful in adding port");
   }
   else
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - port_vm6 Failed to add port");
   }

   strcpy(port_desc4.name,"port_vmns7");
   port_desc4.port_no = 0x123456;

   ret_val = of_add_port_to_dp(dp_handle1,&port_desc4);
   if(ret_val == DPRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - port_vmns7 Successful in adding port");
   }
   else
   {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - port_vmns7 Failed to add port");
   }

   nwservice_key_object1.name_p = malloc(20);
   nwservice_key_object1.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object1.name_p , "service1");
   strcpy(nwservice_key_object1.tenant_name_p , "t1");
   nwservice_config_object1[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object1[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object1,nwservice_config_object1);
 
   nwservice_key_object2.name_p = malloc(20);
   nwservice_key_object2.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object2.name_p , "service2");
   strcpy(nwservice_key_object1.tenant_name_p , "t1");
   nwservice_config_object2[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object2[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object2,nwservice_config_object2);
  
   nwservice_key_object3.name_p = malloc(20);
   nwservice_key_object3.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object3.name_p , "service3");
   strcpy(nwservice_key_object3.tenant_name_p , "t1");
   nwservice_config_object3[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object3[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object3,nwservice_config_object3);
 
   nwservice_key_object4.name_p = malloc(20);
   nwservice_key_object4.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object4.name_p , "service4");
   strcpy(nwservice_key_object4.tenant_name_p , "t1");
   nwservice_config_object4[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object4[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object4,nwservice_config_object4);
  

   nwservice_key_object5.name_p = malloc(20);
   nwservice_key_object5.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object5.name_p , "service5");
   strcpy(nwservice_key_object5.tenant_name_p , "t1");
   nwservice_config_object5[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object5[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object5,nwservice_config_object5);
  
   nwservice_key_object6.name_p = malloc(20);
   nwservice_key_object6.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object6.name_p , "service6");
   strcpy(nwservice_key_object6.tenant_name_p , "t1");
   nwservice_config_object6[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object6[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object6,nwservice_config_object6);

   nwservice_key_object7.name_p = malloc(20);
   nwservice_key_object7.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object7.name_p , "service7");
   strcpy(nwservice_key_object7.tenant_name_p , "t1");
   nwservice_config_object7[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object7[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object7,nwservice_config_object7);

   nwservice_key_object8.name_p = malloc(20);
   nwservice_key_object8.tenant_name_p = malloc(20);
   strcpy(nwservice_key_object8.name_p , "service8");
   strcpy(nwservice_key_object8.tenant_name_p , "t1");
   nwservice_config_object8[0].value.nwservice_object_form_factor_type_e = 0;
   nwservice_config_object8[1].value.admin_status_e = 0;

   ret_val = nsrm_add_nwservice_object(2,&nwservice_key_object8,nwservice_config_object8);
   
   nschain_key_object1.name_p = malloc(20);
   nschain_key_object1.tenant_name_p = malloc(20);
   strcpy(nschain_key_object1.name_p , "chain1");
   strcpy(nschain_key_object1.tenant_name_p , "t1");
   nschain_config_object1[5].value.admin_status_e = 0;

   ret_val = nsrm_add_nschain_object(2,&nschain_key_object1,&nschain_config_object1);

   nschain_key_object2.name_p = malloc(20);
   nschain_key_object2.tenant_name_p = malloc(20);
   strcpy(nschain_key_object2.name_p , "chain2");
   strcpy(nschain_key_object2.tenant_name_p , "t1");
   nschain_config_object2[5].value.admin_status_e = 0;

   ret_val = nsrm_add_nschain_object(2,&nschain_key_object2,&nschain_config_object2);
   attach_key1.name_p = malloc(20);
   attach_key1.nschain_object_name_p = malloc(20);
   strcpy(attach_key1.name_p,"service1");
   strcpy(attach_key1.nschain_object_name_p,"chain1");
   attach_key1.sequence_number = 3;
    OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   attach_key2.name_p = malloc(20);
   attach_key2.nschain_object_name_p = malloc(20);
   strcpy(attach_key2.name_p,"service2");
   strcpy(attach_key2.nschain_object_name_p,"chain1");
   attach_key2.sequence_number = 2;    
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
 
   attach_key3.name_p = malloc(20);
   attach_key3.nschain_object_name_p = malloc(20);
   strcpy(attach_key3.name_p,"service3");
   strcpy(attach_key3.nschain_object_name_p,"chain1");
   attach_key3.sequence_number = 1;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   attach_key4.name_p = malloc(20);
   attach_key4.nschain_object_name_p = malloc(20);
   strcpy(attach_key4.name_p,"service4");
   strcpy(attach_key4.nschain_object_name_p,"chain1");
   attach_key4.sequence_number = 4;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   attach_key5.name_p = malloc(20);
   attach_key5.nschain_object_name_p = malloc(20);
   strcpy(attach_key5.name_p,"service5");
   strcpy(attach_key5.nschain_object_name_p,"chain1");
   attach_key5.sequence_number = 5;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   nsrm_attach_nwservice_object_to_nschain_object(&attach_key1);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key2);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key3);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key4);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key5);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   attach_key6.name_p = malloc(20);
   attach_key6.nschain_object_name_p = malloc(20);
   strcpy(attach_key6.name_p,"service1");
   strcpy(attach_key6.nschain_object_name_p,"chain2");
   attach_key6.sequence_number = 2;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   attach_key7.name_p = malloc(20);
   attach_key7.nschain_object_name_p = malloc(20);
   strcpy(attach_key7.name_p,"service2");
   strcpy(attach_key7.nschain_object_name_p,"chain2");
   attach_key7.sequence_number = 1;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   attach_key8.name_p = malloc(20);
   attach_key8.nschain_object_name_p = malloc(20);
   strcpy(attach_key8.name_p,"service3");
   strcpy(attach_key8.nschain_object_name_p,"chain2");
   attach_key8.sequence_number = 3;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 


   attach_key9.name_p = malloc(20);
   attach_key9.nschain_object_name_p = malloc(20);
   strcpy(attach_key9.name_p,"service4");
   strcpy(attach_key9.nschain_object_name_p,"chain2");
   attach_key9.sequence_number = 5;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   attach_key10.name_p = malloc(20);
   attach_key10.nschain_object_name_p = malloc(20);
   strcpy(attach_key10.name_p,"service5");
   strcpy(attach_key10.nschain_object_name_p,"chain2");
   attach_key10.sequence_number = 4;
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   nsrm_attach_nwservice_object_to_nschain_object(&attach_key6);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key7);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key8);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key9);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nsrm_attach_nwservice_object_to_nschain_object(&attach_key10);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 

   nschainset_key_object1.name_p = malloc(20);
   nschainset_key_object1.tenant_name_p = malloc(20);
   strcpy(nschainset_key_object1.name_p , "chainset1");
   strcpy(nschainset_key_object1.tenant_name_p , "t1");
   nschainset_config_object1[0].value.nschainset_type = 2;
   nschainset_config_object1[1].value.admin_status_e = 0;
   
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   ret_val = nsrm_add_nschainset_object(2,&nschainset_key_object1,&nschainset_config_object1);
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "debug"); 
   nschainset_key_object2.name_p = malloc(20);
   nschainset_key_object2.tenant_name_p = malloc(20);
   strcpy(nschainset_key_object2.name_p , "chainset2");
   strcpy(nschainset_key_object2.tenant_name_p , "t1");
   nschainset_config_object2[0].value.nschainset_type = 2;
   nschainset_config_object2[1].value.admin_status_e = 0;
   ret_val = nsrm_add_nschainset_object(2,&nschainset_key_object2,&nschainset_config_object2);

   l2nw_key_object1.map_name_p = (char*)malloc(20);
   l2nw_key_object1.tenant_name_p = (char*)malloc(20);
   l2nw_config_object1[0].value.nschainset_object_name_p = (char*)malloc(20);
   l2nw_key_object1.vn_name_p = (char*)malloc(20);
   strcpy(l2nw_key_object1.map_name_p , "l2_map1");
   strcpy(l2nw_key_object1.tenant_name_p , "t1");
   strcpy(l2nw_key_object1.vn_name_p , "VN1");
   strcpy(l2nw_config_object1[0].value.nschainset_object_name_p ,"chainset1");
   l2nw_config_object1[1].value.admin_status_e = 1;
   ret_val = nsrm_add_l2nw_service_map_record(2,&l2nw_key_object1,&l2nw_config_object1);
   
   uint8_t mac_src[6] = {1,2,3,0x78,0x79,0x81} ;
   uint8_t mac_dst[6] = {1,2,3,0x78,0x79,0x80};

   key_info1.name_p = (char*)malloc(20);
   key_info1.tenant_name_p = (char*)malloc(20);
   key_info1.nschainset_object_name_p = (char*)malloc(20);
   config_info1[8].value.nschain_object_name_p = (char*)malloc(20);
   config_info1[9].value.relative_name_p = (char*)malloc(20);

   strcpy(key_info1.name_p,"srule1");
   strcpy(key_info1.tenant_name_p ,"t1");
   strcpy(key_info1.nschainset_object_name_p ,"chainset1");
   strcpy(config_info1[8].value.nschain_object_name_p,"chain1");
   strcpy(config_info1[9].value.relative_name_p,"relname");
   config_info1[0].value.src_mac.mac_address_type_e = 0;
   memcpy(config_info1[0].value.src_mac.mac,mac_src,6);
   config_info1[1].value.dst_mac.mac_address_type_e = 0;
   memcpy(config_info1[1].value.dst_mac.mac,mac_dst,6);
   config_info1[2].value.eth_type.ethtype_type_e    = 0;
   config_info1[2].value.eth_type.ethtype           = 0x800;
   config_info1[3].value.sip.ip_object_type_e       = 0;
   config_info1[3].value.sip.ipaddr_start           = 0;
   config_info1[3].value.sip.ipaddr_end             = 0;
   config_info1[4].value.dip.ip_object_type_e       = 0;
   config_info1[4].value.dip.ipaddr_start           = 0;
   config_info1[4].value.dip.ipaddr_end             = 0;
   config_info1[5].value.sp.port_object_type_e      = 0;
   config_info1[5].value.sp.port_start              = 0;
   config_info1[5].value.sp.port_end                = 0;
   config_info1[6].value.dp.port_object_type_e      = 0;
   config_info1[6].value.dp.port_start              = 0;
   config_info1[6].value.dp.port_end                = 0;

   nsrm_add_nschain_selection_rule_to_nschainset(&key_info1,2,&config_info1);

   nsrm_get_nschainset_object_handle("chainset1",&nschainset_handle_1);
   /* Need to fill the packet structure and should call selection lookup*/
  
   key_info2.name_p = (char*)malloc(20);
   key_info2.tenant_name_p = (char*)malloc(20);
   key_info2.nschainset_object_name_p = (char*)malloc(20);
   config_info2[8].value.nschain_object_name_p = (char*)malloc(20);
   config_info2[9].value.relative_name_p = (char*)malloc(20);

   strcpy(key_info2.name_p,"srule2");
   strcpy(key_info2.tenant_name_p ,"t1");
   strcpy(key_info2.nschainset_object_name_p ,"chainset1");
   strcpy(config_info2[8].value.nschain_object_name_p,"chain1");/* NSM changed to chain1 from chain2 */
   strcpy(config_info2[9].value.relative_name_p,"relname");
   config_info2[0].value.src_mac.mac_address_type_e = MAC_ANY;
   memcpy(config_info2[0].value.src_mac.mac,mac_src,6);
   config_info2[1].value.dst_mac.mac_address_type_e = MAC_SINGLE;
   memcpy(config_info2[1].value.dst_mac.mac,mac_dst,6);
   config_info2[2].value.eth_type.ethtype_type_e    = 0;
   config_info2[2].value.eth_type.ethtype           = 0x800;
   config_info2[3].value.sip.ip_object_type_e       = IP_ANY;
   config_info2[3].value.sip.ipaddr_start           = htonl(0xc0a80100);
   config_info2[3].value.sip.ipaddr_end             = htonl(0xc0a8010f);
   config_info2[4].value.dip.ip_object_type_e       = IP_RANGE;
   config_info2[4].value.dip.ipaddr_start           = htonl(0xc0a80200);
   config_info2[4].value.dip.ipaddr_end             = htonl(0xc0a8020f);
   config_info2[5].value.sp.port_object_type_e      = PORT_ANY;
   config_info2[5].value.sp.port_start              = 0;
   config_info2[5].value.sp.port_end                = 0xffff;
   config_info2[6].value.dp.port_object_type_e      = PORT_RANGE;
   config_info2[6].value.dp.port_start              = 0;
   config_info2[6].value.dp.port_end                = 0xffff;

   nsrm_add_nschain_selection_rule_to_nschainset(&key_info2,2,&config_info2);

   nsrm_get_nschainset_object_handle("chainset1",&nschainset_handle_2);
   /* Need to fill the packet structure and should call selection lookup*/
  
   key_info3.name_p = (char*)malloc(20);
   key_info3.tenant_name_p = (char*)malloc(20);
   key_info3.nschainset_object_name_p = (char*)malloc(20);
   config_info3[8].value.nschain_object_name_p = (char*)malloc(20);
   config_info3[9].value.relative_name_p = (char*)malloc(20);

   strcpy(key_info3.name_p,"srule3");
   strcpy(key_info3.tenant_name_p ,"t1");
   strcpy(key_info3.nschainset_object_name_p ,"chainset1");
   strcpy(config_info3[8].value.nschain_object_name_p,"chain2"); /* NSM changed to CHAIN1 from chain 2*/
   strcpy(config_info3[9].value.relative_name_p,"none");
   config_info3[0].value.src_mac.mac_address_type_e = MAC_ANY;
   memcpy(config_info3[0].value.src_mac.mac,mac_src,6);
   config_info3[1].value.dst_mac.mac_address_type_e = MAC_SINGLE;
   memcpy(config_info3[1].value.dst_mac.mac,mac_dst,6);
   config_info3[2].value.eth_type.ethtype_type_e    = 0;
   config_info3[2].value.eth_type.ethtype           = 0x800;
   config_info3[3].value.sip.ip_object_type_e       = IP_ANY;
   config_info3[3].value.sip.ipaddr_start           = htonl(0xc0a80300);
   config_info3[3].value.sip.ipaddr_end             = htonl(0xc0a8030f);
   config_info3[4].value.dip.ip_object_type_e       = IP_RANGE;
   config_info3[4].value.dip.ipaddr_start           = htonl(0xc0a80400);
   config_info3[4].value.dip.ipaddr_end             = htonl(0xc0a8040f);
   config_info3[5].value.sp.port_object_type_e      = PORT_ANY;
   config_info3[5].value.sp.port_start              = 0;
   config_info3[5].value.sp.port_end                = 0xffff;
   config_info3[6].value.dp.port_object_type_e      = PORT_RANGE;
   config_info3[6].value.dp.port_start              = 0;
   config_info3[6].value.dp.port_end                = 0xffff;

   nsrm_add_nschain_selection_rule_to_nschainset(&key_info3,2,&config_info3);
   nsrm_get_nschainset_object_handle("chainset2",&nschainset_handle_3);
   /* Need to fill the packet structure and should call selection lookup*/
 
   key_info4.name_p = (char*)malloc(20);
   key_info4.tenant_name_p = (char*)malloc(20);
   key_info4.nschainset_object_name_p = (char*)malloc(20);
   config_info4[8].value.nschain_object_name_p = (char*)malloc(20);
   config_info4[9].value.relative_name_p = (char*)malloc(20);

   strcpy(key_info4.name_p,"srule4");
   strcpy(key_info4.tenant_name_p ,"t1");
   strcpy(key_info4.nschainset_object_name_p ,"chainset2");
   strcpy(config_info4[8].value.nschain_object_name_p,"chain2");
   strcpy(config_info4[9].value.relative_name_p,"srule1");
   config_info4[0].value.src_mac.mac_address_type_e = 0;
   memcpy(config_info4[0].value.src_mac.mac,mac_src,6);
   config_info4[1].value.dst_mac.mac_address_type_e = 0;
   memcpy(config_info4[1].value.dst_mac.mac,mac_dst,6);
   config_info4[2].value.eth_type.ethtype_type_e    = 0;
   config_info4[2].value.eth_type.ethtype           = 0x800;
   config_info4[3].value.sip.ip_object_type_e       = 0;
   config_info4[3].value.sip.ipaddr_start           = 16216810;
   config_info4[3].value.sip.ipaddr_end             = 162168120;
   config_info4[4].value.dip.ip_object_type_e       = 0;
   config_info4[4].value.dip.ipaddr_start           = 16216820;
   config_info4[4].value.dip.ipaddr_end             = 162168220;
   config_info4[5].value.sp.port_object_type_e      = 0;
   config_info4[5].value.sp.port_start              = 10;
   config_info4[5].value.sp.port_end                = 15;
   config_info4[6].value.dp.port_object_type_e      = 0;
   config_info4[6].value.dp.port_start              = 16;
   config_info4[6].value.dp.port_end                = 20;

   nsrm_add_nschain_selection_rule_to_nschainset(&key_info4,2,&config_info4);
   nsrm_get_nschainset_object_handle("chainset2",&nschainset_handle_4);
   /* Need to fill the packet structure and should call selection lookup*/
    
   key_infob1.name_p = (char*)malloc(20);
   key_infob1.tenant_name_p = (char*)malloc(20);
   key_infob1.nschain_object_name_p = (char*)malloc(20);

   strcpy(key_infob1.name_p,"brule1");
   strcpy(key_infob1.tenant_name_p ,"t1");
   strcpy(key_infob1.nschain_object_name_p ,"chain1");
   
   config_infob1[0].value.src_mac.mac_address_type_e = MAC_SINGLE;
   memcpy(config_infob1[0].value.src_mac.mac,mac_src,6);
   config_infob1[1].value.dst_mac.mac_address_type_e = MAC_ANY;
   memcpy(config_infob1[1].value.dst_mac.mac,mac_dst,6);
   config_infob1[2].value.eth_type.ethtype_type_e    = 0;
   config_infob1[2].value.eth_type.ethtype           = 0x800;
   config_infob1[3].value.sip.ip_object_type_e       = IP_RANGE;
   config_infob1[3].value.sip.ipaddr_start           = htonl(0xc0a80100);
   config_infob1[3].value.sip.ipaddr_end             = htonl(0xc0a8010f); 
   config_infob1[4].value.dip.ip_object_type_e       = IP_ANY;
   config_infob1[4].value.dip.ipaddr_start           = htonl(0xc0a80200);
   config_infob1[4].value.dip.ipaddr_end             = htonl(0xc0a8020f);
   config_infob1[5].value.sp.port_object_type_e      = PORT_RANGE;
   config_infob1[5].value.sp.port_start              = 0;
   config_infob1[5].value.sp.port_end                = 0xffff;
   config_infob1[6].value.dp.port_object_type_e      = PORT_ANY;
   config_infob1[6].value.dp.port_start              = 0;
   config_infob1[6].value.dp.port_end                = 0xffff;

   key_infob1.location_e = 0;
   nsrm_add_nwservice_bypass_rule_to_nschain_object(&key_infob1,2,&config_infob1);

   nsrm_get_nschain_object_handle("chain1",&nschain_object_handle_1);  
  
   /* Need to fill the packet structure and should call bypass lookup*/

   key_infob2.name_p = (char*)malloc(20);
   key_infob2.tenant_name_p = (char*)malloc(20);
   key_infob2.nschain_object_name_p = (char*)malloc(20);

   strcpy(key_infob2.name_p,"brule2");
   strcpy(key_infob2.tenant_name_p ,"t1");
   strcpy(key_infob2.nschain_object_name_p ,"chain1"); /* NSM changed to chain1 from chain2 */
   
   config_infob2[0].value.src_mac.mac_address_type_e = 0;
   memcpy(config_infob2[0].value.src_mac.mac,mac_src,6);
   config_infob2[1].value.dst_mac.mac_address_type_e = 0;
   memcpy(config_infob2[1].value.dst_mac.mac,mac_dst,6);
   config_infob2[2].value.eth_type.ethtype_type_e    = 0;
   config_infob2[2].value.eth_type.ethtype           = 0x800;
   config_infob2[3].value.sip.ip_object_type_e       = IP_RANGE;
   config_infob2[3].value.sip.ipaddr_start           = 17216814;
   config_infob2[3].value.sip.ipaddr_end             = 17216818;
   config_infob2[4].value.dip.ip_object_type_e       = 0;
   config_infob2[4].value.dip.ipaddr_start           = 17216823;
   config_infob2[4].value.dip.ipaddr_end             = 17216829;
   config_infob2[5].value.sp.port_object_type_e      = 0;
   config_infob2[5].value.sp.port_start              = 10;
   config_infob2[5].value.sp.port_end                = 15;
   config_infob2[6].value.dp.port_object_type_e      = 0;
   config_infob2[6].value.dp.port_start              = 16;
   config_infob2[6].value.dp.port_end                = 20;

   key_infob2.location_e = 0;
   nsrm_add_nwservice_bypass_rule_to_nschain_object(&key_infob2,2,&config_infob2);

   nsrm_get_nschain_object_handle("chain2",&nschain_object_handle_2);  
   /* Need to fill the packet structure and should call bypass lookup*/

   key_infob3.name_p = (char*)malloc(20);
   key_infob3.tenant_name_p = (char*)malloc(20);
   key_infob3.nschain_object_name_p = (char*)malloc(20);

   strcpy(key_infob3.name_p,"brule3");
   strcpy(key_infob3.tenant_name_p ,"t1");
   strcpy(key_infob3.nschain_object_name_p ,"chain2");
   config_infob3[0].value.src_mac.mac_address_type_e = MAC_SINGLE;
   memcpy(config_infob3[0].value.src_mac.mac,mac_src,6);
   config_infob3[1].value.dst_mac.mac_address_type_e = MAC_ANY;
   memcpy(config_infob3[1].value.dst_mac.mac,mac_dst,6);
   config_infob3[2].value.eth_type.ethtype_type_e    = 0;
   config_infob3[2].value.eth_type.ethtype           = 0x800;
   config_infob3[3].value.sip.ip_object_type_e       = IP_RANGE;
   config_infob3[3].value.sip.ipaddr_start           = htonl(0xc0a80300);
   config_infob3[3].value.sip.ipaddr_end             = htonl(0xc0a8030f);
   config_infob3[4].value.dip.ip_object_type_e       = IP_ANY;
   config_infob3[4].value.dip.ipaddr_start           = htonl(0xc0a80400);
   config_infob3[4].value.dip.ipaddr_end             = htonl(0xc0a8040f);
   config_infob3[5].value.sp.port_object_type_e      = PORT_RANGE;
   config_infob3[5].value.sp.port_start              = 0;
   config_infob3[5].value.sp.port_end                = 0xffff;
   config_infob3[6].value.dp.port_object_type_e      = 0;
   config_infob3[6].value.dp.port_start              = PORT_ANY;
   config_infob3[6].value.dp.port_end                = 0xffff;

   key_infob3.location_e = 0;
   nsrm_add_nwservice_bypass_rule_to_nschain_object(&key_infob3,2,&config_infob3);

   bypass_nwservice_key1.nschain_object_name_p = (char *)malloc(20);
   bypass_nwservice_key1.bypass_rule_name_p  = (char *)malloc(20);
   bypass_nwservice_key1.nwservice_object_name_p = (char*)malloc(20);
   strcpy(bypass_nwservice_key1.nschain_object_name_p,"chain1");
   strcpy(bypass_nwservice_key1.bypass_rule_name_p,"brule1");
   strcpy(bypass_nwservice_key1.nwservice_object_name_p,"service3");
   
   nsrm_attach_nwservice_to_bypass_rule(&bypass_nwservice_key1);

   bypass_nwservice_key2.nschain_object_name_p = (char *)malloc(20);
   bypass_nwservice_key2.bypass_rule_name_p  = (char *)malloc(20);
   bypass_nwservice_key2.nwservice_object_name_p = (char*)malloc(20);
   strcpy(bypass_nwservice_key2.nschain_object_name_p,"chain1");
   strcpy(bypass_nwservice_key2.bypass_rule_name_p,"brule1");
   strcpy(bypass_nwservice_key2.nwservice_object_name_p,"service5");

   nsrm_attach_nwservice_to_bypass_rule(&bypass_nwservice_key2);

   bypass_nwservice_key3.nschain_object_name_p = (char *)malloc(20);
   bypass_nwservice_key3.bypass_rule_name_p  = (char *)malloc(20);
   bypass_nwservice_key3.nwservice_object_name_p = (char*)malloc(20);
   strcpy(bypass_nwservice_key3.nschain_object_name_p,"chain2");
   strcpy(bypass_nwservice_key3.bypass_rule_name_p,"brule3");
   strcpy(bypass_nwservice_key3.nwservice_object_name_p,"service1");
    
   nsrm_attach_nwservice_to_bypass_rule(&bypass_nwservice_key3);

   bypass_nwservice_key4.nschain_object_name_p = (char *)malloc(20);
   bypass_nwservice_key4.bypass_rule_name_p  = (char *)malloc(20);
   bypass_nwservice_key4.nwservice_object_name_p = (char*)malloc(20);
   strcpy(bypass_nwservice_key4.nschain_object_name_p,"chain2");
   strcpy(bypass_nwservice_key4.bypass_rule_name_p,"brule3");
   strcpy(bypass_nwservice_key4.nwservice_object_name_p,"service2");

   nsrm_attach_nwservice_to_bypass_rule(&bypass_nwservice_key4);

   /* Start: VMs for service1 chain1 switch1 - VM1 switch2 - VM2, VM3  switch3 - VM4,VM5 */ 

   strcpy(vm_config_info_1.vm_name,"VM1");
   strcpy(vm_config_info_1.tenant_name,"t1");
   strcpy(vm_config_info_1.switch_name,"switch_1");
   vm_config_info_1.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_1, &vm_handle1);

   strcpy(vm_config_info_2.vm_name,"VM2");
   strcpy(vm_config_info_2.tenant_name,"t1");
   strcpy(vm_config_info_2.switch_name,"switch_2");
   vm_config_info_2.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_2, &vm_handle2);

   strcpy(vm_config_info_3.vm_name,"VM3");
   strcpy(vm_config_info_3.tenant_name,"t1");
   strcpy(vm_config_info_3.switch_name,"switch_2");
   vm_config_info_3.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_3, &vm_handle3);

   strcpy(vm_config_info_4.vm_name,"VM4");
   strcpy(vm_config_info_4.tenant_name,"t1");
   strcpy(vm_config_info_4.switch_name,"switch_3");
   vm_config_info_4.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_4, &vm_handle4);

   strcpy(vm_config_info_5.vm_name,"VM5");
   strcpy(vm_config_info_5.tenant_name,"t1");
   strcpy(vm_config_info_5.switch_name,"switch_3");
   vm_config_info_5.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_5, &vm_handle5);

/*End: VMs for service1 chain1 */

   /* Start: VMs for service2 chain1 switch1 - VM6,VM7 switch2 - VM8 switch3 - VM9,VM10 */

   strcpy(vm_config_info_6.vm_name,"VM6");
   strcpy(vm_config_info_6.tenant_name,"t1");
   strcpy(vm_config_info_6.switch_name,"switch_1");
   vm_config_info_6.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_6, &vm_handle6);

   strcpy(vm_config_info_7.vm_name,"VM7");
   strcpy(vm_config_info_7.tenant_name,"t1");
   strcpy(vm_config_info_7.switch_name,"switch_1");
   vm_config_info_7.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_7, &vm_handle7);

   strcpy(vm_config_info_8.vm_name,"VM8");
   strcpy(vm_config_info_8.tenant_name,"t1");
   strcpy(vm_config_info_8.switch_name,"switch_2");
   vm_config_info_8.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_8, &vm_handle8);

   strcpy(vm_config_info_9.vm_name,"VM9");
   strcpy(vm_config_info_9.tenant_name,"t1");
   strcpy(vm_config_info_9.switch_name,"switch_3");
   vm_config_info_9.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_9, &vm_handle9);

   strcpy(vm_config_info_10.vm_name,"VM10");
   strcpy(vm_config_info_10.tenant_name,"t1");
   strcpy(vm_config_info_10.switch_name,"switch_3");
   vm_config_info_10.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_10, &vm_handle10);

/*End: VMs for service2 chain1 */

   /* Start: VMs for service3 chain1 switch1 - VM11,VM12 switch2 - VM13,VM14 switch3 - VM15 */

   strcpy(vm_config_info_11.vm_name,"VM11");
   strcpy(vm_config_info_11.tenant_name,"t1");
   strcpy(vm_config_info_11.switch_name,"switch_1");
   vm_config_info_11.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_11, &vm_handle11);

   strcpy(vm_config_info_12.vm_name,"VM12");
   strcpy(vm_config_info_12.tenant_name,"t1");
   strcpy(vm_config_info_12.switch_name,"switch_1");
   vm_config_info_12.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_12, &vm_handle12);

   strcpy(vm_config_info_13.vm_name,"VM13");
   strcpy(vm_config_info_13.tenant_name,"t1");
   strcpy(vm_config_info_13.switch_name,"switch_2");
   vm_config_info_13.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_13, &vm_handle13);

   strcpy(vm_config_info_14.vm_name,"VM14");
   strcpy(vm_config_info_14.tenant_name,"t1");
   strcpy(vm_config_info_14.switch_name,"switch_2");
   vm_config_info_14.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_14, &vm_handle14);

   strcpy(vm_config_info_15.vm_name,"VM15");
   strcpy(vm_config_info_15.tenant_name,"t1");
   strcpy(vm_config_info_15.switch_name,"switch_3");
   vm_config_info_15.vm_type = VM_TYPE_NETWORK_SERVICE;
   crm_add_virtual_machine(&vm_config_info_15, &vm_handle15);

/*End: VMs for service3 chain1 */
  strcpy(mac_buf,"01:02:03:04:05:01");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vm1","switch_1","br-int",VM_PORT,"VN1","VM1",vm_port_mac,&port_handle1);
  strcpy(mac_buf,"01:02:03:04:05:02");
  
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns2","switch_2","br-int",VMNS_PORT,"VN1","VM2",vm_port_mac,&port_handle2);

  strcpy(mac_buf,"01:02:03:04:05:03");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vm3","switch_2","br-int",VMNS_PORT,"VN1","VM3",vm_port_mac,&port_handle3);

  strcpy(mac_buf,"01:02:03:04:05:04");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns4","switch_3","br-int",VMNS_PORT,"VN1","VM4",vm_port_mac,&port_handle4);

  strcpy(mac_buf,"01:02:03:04:05:05");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns5","switch_3","br-int",VMNS_PORT,"VN1","VM5",vm_port_mac,&port_handle5);
  strcpy(mac_buf,"01:02:03:04:05:06");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vm6","switch_1","br-int",VMNS_PORT,"VN1","VM6",vm_port_mac,&port_handle6);
  strcpy(mac_buf,"01:02:03:04:05:07");
  /****attribute add call ***/
  strcpy(attr_info1.name_string,"attr1");
  strcpy(attr_info1.value_string,"val1");
  strcpy(attr_info2.name_string,"attr2");
  strcpy(attr_info2.value_string,"val2");

  crm_add_attribute_to_port("port_vm6",&attr_info1);
  crm_add_attribute_to_port("port_vm6",&attr_info2);
  
  attr_output1.name_length = 110;
  attr_output1.value_length = 110;
  attr_output2.name_length = 110;
  attr_output2.value_length = 110;
  crm_get_port_first_attribute("port_vm6",&attr_output1);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"attr_name :%s",attr_output1.name_string);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"attr_val  :%s",attr_output1.value_string);
  crm_get_port_next_attribute("port_vm6",(attr_output1.name_string),&attr_output2);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"attr_name :%s",attr_output2.name_string);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"attr_val  :%s",attr_output2.value_string); 
  crm_delete_attribute_from_port("port_vm6",&attr_info2);
  crm_get_port_first_attribute("port_vm6",&attr_output1);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"attr_name :%s",attr_output1.name_string);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"attr_val  :%s",attr_output1.value_string);
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns7","switch_1","br-int",VMNS_PORT,"VN1","VM7",vm_port_mac,&port_handle7);

  strcpy(mac_buf,"01:02:03:04:05:08");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vm8","switch_2","br-int",VMNS_PORT,"VN1","VM8",vm_port_mac,&port_handle8);

  strcpy(mac_buf,"01:02:03:04:05:09");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns9","switch_3","br-int",VMNS_PORT,"VN1","VM9",vm_port_mac,&port_handle9);

  strcpy(mac_buf,"01:02:03:04:05:0a");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns10","switch_3","br-int",VMNS_PORT,"VN1","VM10",vm_port_mac,&port_handle10);

  strcpy(mac_buf,"01:02:03:04:05:0b");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vm11","switch_1","br-int",VMNS_PORT,"VN1","VM11",vm_port_mac,&port_handle11);

  strcpy(mac_buf,"01:02:03:04:05:0c");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns12","switch_1","br-int",VMNS_PORT,"VN1","VM12",vm_port_mac,&port_handle12);

  strcpy(mac_buf,"01:02:03:04:05:0d");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vm13","switch_2","br-int",VMNS_PORT,"VN1","VM13",vm_port_mac,&port_handle13);

  strcpy(mac_buf,"01:02:03:04:05:0e");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  ret_val = crm_add_crm_vmport("port_vmns14","switch_2","br-int",VMNS_PORT,"VN1","VM14",vm_port_mac,&port_handle14);

  strcpy(mac_buf,"01:02:03:04:05:0f");
  ret_val = of_flow_match_atox_mac_addr(mac_buf,vm_port_mac);
  crm_add_crm_vmport("port_vmns15","switch_3","br-int",VMNS_PORT,"VN1","VM15",vm_port_mac,&port_handle15);
  
  strcpy(port_desc.name,"port_vm6"); 
  port_desc.port_no = 0x123456;

  ret_val = of_add_port_to_dp(dp_handle1,&port_desc);
  if(ret_val == DPRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - Successful in adding port");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - Failed to add port");
  }

  strcpy(port_desc2.name,"sw1uniport");
  port_desc2.port_no = 0x123489;

  ret_val = of_add_port_to_dp(dp_handle1,&port_desc2);
  if(ret_val == DPRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - sw1uniport Successful in adding port");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"of_add_port_to_dp - sw1uniport Failed to add port");
  }

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 1");
   si1_key.nwservice_instance_name_p = (char*)malloc(10);
   si1_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si1_key.tenant_name_p,"t1");
   si1_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si1_key.nschain_object_name_p,"chain1");
   si1_key.nwservice_object_name_p = (char*)malloc(10);
   si1_key.vm_name_p   = (char*)malloc(10);

   strcpy(si1_key.nwservice_object_name_p,"service1");
   strcpy(si1_key.nwservice_instance_name_p,"inst1");
   strcpy(si1_key.vm_name_p,"VM1");
   si1_key.vlan_id_in  = 12; 
   si1_key.vlan_id_out = 13;
   
   ret_val = nsrm_register_launched_nwservice_object_instance(&si1_key);
   
   si1a_key.nwservice_instance_name_p = (char*)malloc(10);
   si1a_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si1a_key.tenant_name_p,"t1");
   si1a_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si1a_key.nschain_object_name_p,"chain2");
   si1a_key.nwservice_object_name_p = (char*)malloc(10);
   si1a_key.vm_name_p   = (char*)malloc(10);

   strcpy(si1a_key.nwservice_object_name_p,"service1");
   strcpy(si1a_key.nwservice_instance_name_p,"inst8");
   strcpy(si1a_key.vm_name_p,"VM8");
   si1a_key.vlan_id_in  = 88;
   si1a_key.vlan_id_out = 89;
 
   strcpy(si1a_key.nschain_object_name_p,"chain2");  
   ret_val = nsrm_register_launched_nwservice_object_instance(&si1a_key);
 
   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 2");

   si2_key.nwservice_instance_name_p = (char*)malloc(10);
   si2_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si2_key.tenant_name_p,"t1");
   si2_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si2_key.nschain_object_name_p,"chain1");
   si2_key.nwservice_object_name_p = (char*)malloc(10);
   si2_key.vm_name_p   = (char*)malloc(10);

   strcpy(si2_key.nwservice_object_name_p,"service1");

   strcpy(si2_key.nwservice_instance_name_p,"inst2");
   strcpy(si2_key.vm_name_p,"VM2");
   si2_key.vlan_id_in  = 14;
   si2_key.vlan_id_out = 15;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si2_key);

   strcpy(si2_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si2_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 3");
  
   si3_key.nwservice_instance_name_p = (char*)malloc(10);
   si3_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si3_key.tenant_name_p,"t1");
   si3_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si3_key.nschain_object_name_p,"chain1");
   si3_key.nwservice_object_name_p = (char*)malloc(10);
   si3_key.vm_name_p   = (char*)malloc(10);

   strcpy(si3_key.nwservice_object_name_p,"service1");


   strcpy(si3_key.nwservice_instance_name_p,"inst3");
   strcpy(si3_key.vm_name_p,"VM3");
   si3_key.vlan_id_in  = 16;
   si3_key.vlan_id_out = 17;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si3_key);

   strcpy(si3_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si3_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 6");
   si6_key.nwservice_instance_name_p = (char*)malloc(10);
   si6_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si6_key.tenant_name_p,"t1");
   si6_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si6_key.nschain_object_name_p,"chain1");
   si6_key.nwservice_object_name_p = (char*)malloc(10);
   si6_key.vm_name_p   = (char*)malloc(10);

   strcpy(si6_key.nwservice_object_name_p,"service2");
   strcpy(si6_key.nwservice_instance_name_p,"inst6");
   strcpy(si6_key.vm_name_p,"VM6");
   si6_key.vlan_id_in  = 22;
   si6_key.vlan_id_out = 23;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si6_key);

   strcpy(si6_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si6_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 7");
   si7_key.nwservice_instance_name_p = (char*)malloc(10);
   si7_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si7_key.tenant_name_p,"t1");
   si7_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si7_key.nschain_object_name_p,"chain1");
   si7_key.nwservice_object_name_p = (char*)malloc(10);
   si7_key.vm_name_p   = (char*)malloc(10);

   strcpy(si7_key.nwservice_object_name_p,"service2");
   strcpy(si7_key.nwservice_instance_name_p,"inst7");
   strcpy(si7_key.vm_name_p,"VM7");
   si7_key.vlan_id_in  = 24;
   si7_key.vlan_id_out = 25;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si7_key);

   strcpy(si7_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si7_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 8");
   si8_key.nwservice_instance_name_p = (char*)malloc(10);
   si8_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si8_key.tenant_name_p,"t1");
   si8_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si8_key.nschain_object_name_p,"chain1");
   si8_key.nwservice_object_name_p = (char*)malloc(10);
   si8_key.vm_name_p   = (char*)malloc(10);

   strcpy(si8_key.nwservice_object_name_p,"service2");
   strcpy(si8_key.nwservice_instance_name_p,"inst8");
   strcpy(si8_key.vm_name_p,"VM8");
   si8_key.vlan_id_in  = 26;
   si8_key.vlan_id_out = 27;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si8_key);

   strcpy(si8_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si8_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 11");
   si11_key.nwservice_instance_name_p = (char*)malloc(10);
   si11_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si11_key.tenant_name_p,"t1");
   si11_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si11_key.nschain_object_name_p,"chain1");
   si11_key.nwservice_object_name_p = (char*)malloc(10);
   si11_key.vm_name_p   = (char*)malloc(10);

   strcpy(si11_key.nwservice_object_name_p,"service3");
   strcpy(si11_key.nwservice_instance_name_p,"inst11");
   strcpy(si11_key.vm_name_p,"VM11");
   si11_key.vlan_id_in  = 32;
   si11_key.vlan_id_out = 33;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si11_key);

   strcpy(si11_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si11_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 12");
   si12_key.nwservice_instance_name_p = (char*)malloc(10);
   si12_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si12_key.tenant_name_p,"t1");
   si12_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si12_key.nschain_object_name_p,"chain1");
   si12_key.nwservice_object_name_p = (char*)malloc(10);
   si12_key.vm_name_p   = (char*)malloc(10);

   strcpy(si12_key.nwservice_object_name_p,"service3");
   strcpy(si12_key.nwservice_instance_name_p,"inst12");
   strcpy(si12_key.vm_name_p,"VM12");
   si12_key.vlan_id_in  = 34;
   si12_key.vlan_id_out = 35;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si12_key);

   strcpy(si12_key.nschain_object_name_p,"chain2");
   ret_val = nsrm_register_launched_nwservice_object_instance(&si12_key);

   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NS launch 13");
   si13_key.nwservice_instance_name_p = (char*)malloc(10);
   si13_key.tenant_name_p  = (char*)malloc(10);
   strcpy(si13_key.tenant_name_p,"t1");
   si13_key.nschain_object_name_p = (char*)malloc(10);
   strcpy(si13_key.nschain_object_name_p,"chain1");
   si13_key.nwservice_object_name_p = (char*)malloc(10);
   si13_key.vm_name_p   = (char*)malloc(10);

   strcpy(si13_key.nwservice_object_name_p,"service3");
   strcpy(si13_key.nwservice_instance_name_p,"inst13");
   strcpy(si13_key.vm_name_p,"VM13");
   si13_key.vlan_id_in  = 36;
   si13_key.vlan_id_out = 37;

   ret_val = nsrm_register_launched_nwservice_object_instance(&si13_key);

   strcpy(si13_key.nschain_object_name_p,"chain2");

   ret_val = nsrm_register_launched_nwservice_object_instance(&si13_key);

   crm_del_crm_vmport("port_vmns7",
                       VMNS_PORT,
                       "VN1",
                       "switch_1",
                       "VM7",
                       "br-int");
 
   nsrm_register_delaunch_nwswervice_object_instance(&si6_key);

   crm_del_crm_vmport("port_vm6",
                       VMNS_PORT,
                       "VN1",
                       "switch_1",
                       "VM6",
                       "br-int");
}
/*************************************************************************************************/
int32_t nsc_test(struct ofl_packet_in *pkt_in)
{
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   crm_virtual_network*      vn_entry_p;
  struct   l2_connection_to_nsinfo_bind_node* cn_bind_entry_p;
  uint8_t  selector_type;
  struct   nsc_selector_node  selector;
  struct   nsc_selector_node* selector_p;
  uint64_t vn_handle;  
  int32_t  retval = OF_FAILURE;
  uint32_t vni    = 0x123456;

  selector_p = &selector;

  retval  = nsc_extract_packet_fields(pkt_in,selector_p,PACKET_NORMAL,TRUE);
  if(retval != OF_SUCCESS)
  {
     OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"extract packet fields failed");
     return OF_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test1a");
  
  retval = nsc_get_cnbind_entry(pkt_in,
                                selector_p,
                                VXLAN_TYPE,
                                vni,
                                &cn_bind_entry_p,
                                &selector_type,
                                FALSE);

  if(retval == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_get_cnbind_entry() returned OF_FAILURE");  
    return OF_FAILURE;  /* The caller drops the packets. This error is a systemic error mainly due to memory allocation failures. */
  }
  OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test2");
  OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"selector_type : %d",selector_type);
  if(retval == NSC_CONNECTION_BIND_ENTRY_CREATED)
  {
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"A new cn_bind entry is created");
    retval = nsc_lookup_l2_nschaining_info(pkt_in,
                                           VXLAN_TYPE,
                                           vni,
                                           cn_bind_entry_p);

    if(retval == OF_FAILURE)
    { 
      OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_lookup_l2_nschaining_info() returned failure due to system issues or NSC_INCOMPLETE_NSCHAIN_CONFIGURATION");
      return retval;   /* The caller drops the packets. */
    }
  
    CNTLR_RCU_READ_LOCK_TAKE();
    
    retval = tsc_nvm_modules[VXLAN_TYPE].nvm_module_get_vnhandle_by_nid(vni,&vn_handle);
    if(retval == OF_SUCCESS)
      retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);

    if(retval != OF_SUCCESS)
    {  
      CNTLR_RCU_READ_LOCK_RELEASE();
      return OF_FAILURE;
    }

    vn_nsc_info_p = (struct vn_service_chaining_info *)(*(uint32_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test4");

    retval = nsc_add_selectors(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p,vn_nsc_info_p->nsc_cn_bind_mempool_g,cn_bind_entry_p);

    CNTLR_RCU_READ_LOCK_RELEASE();
    
  }
  OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"selector_type : %d",selector_type);
  nsrm_test_display_service_vms_in_chain(cn_bind_entry_p,selector_type);
  return OF_SUCCESS;
}

int32_t nsrm_test_display_service_vms_in_chain(struct  l2_connection_to_nsinfo_bind_node* cn_bind_entry_p,uint8_t selector_type)
{
  struct  nwservice_instance* nwservice_instance_p;
  struct  nw_services_to_apply *nw_services_p,*nw_services1_p;
  int8_t  ii,no_of_nwservice_instances;
  int32_t retval = OF_FAILURE;

  if((cn_bind_entry_p->nsc_config_status ==  NSC_NWSERVICES_CONFIGURED))
  {
    no_of_nwservice_instances  =  cn_bind_entry_p->nwservice_info.no_of_nwservice_instances;
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"no_of_nwservice_instances = %d",no_of_nwservice_instances);

    nw_services_p = (struct  nw_services_to_apply *)cn_bind_entry_p->nwservice_info.nw_services_p;
    CNTLR_RCU_READ_LOCK_TAKE();
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"selector_type : %d",selector_type);
    if(selector_type == SELECTOR_PRIMARY)
    {
      for(ii = 0; ii < no_of_nwservice_instances; ii++)
      {
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test11");
    
        retval = nsrm_get_nwservice_instance_byhandle(nw_services_p->nschain_object_handle,
                                                      nw_services_p->nwservice_object_handle,
                                                      nw_services_p->nwservice_instance_handle,
                                                      &nwservice_instance_p);
        
        if(retval == OF_FAILURE)
        {
          OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test12");
          continue;  /* TBD  Start from next service. A re-lookup may be required. */
        }
     
        nw_services_p++;
      
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"name           : %s",nwservice_instance_p->name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vm_name        : %s",nwservice_instance_p->vm_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"tenant_name    : %s",nwservice_instance_p->tenant_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"service_name   : %s",nwservice_instance_p->nwservice_object_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"switch_name    : %s",nwservice_instance_p->switch_name);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vlanid in      : %d",nwservice_instance_p->vlan_id_in);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vlanid out     : %d",nwservice_instance_p->vlan_id_out);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vmns port name : %s",nwservice_instance_p->vm_ns_port1_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"portid         : %d",nwservice_instance_p->port1_id);
      }
      CNTLR_RCU_READ_LOCK_RELEASE();
   }
   else
   {
      OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nw_services_p : %p",nw_services_p); 
      nw_services1_p = (struct nw_services_to_apply*)(nw_services_p + no_of_nwservice_instances - 1); 
      for(ii = no_of_nwservice_instances; ii > 0; ii--)
      {
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test11");
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nw_services_p : %p",nw_services1_p);

        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nw_services1_p : %p",nw_services1_p);        
        retval = nsrm_get_nwservice_instance_byhandle(nw_services1_p->nschain_object_handle,
                                                      nw_services1_p->nwservice_object_handle,
                                                      nw_services1_p->nwservice_instance_handle,
                                                      &nwservice_instance_p);

        if(retval == OF_FAILURE)
        {
          OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_test12");
          continue;  /* TBD  Start from next service. A re-lookup may be required. */
        }

        nw_services1_p--;

        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"name           : %s",nwservice_instance_p->name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vm_name        : %s",nwservice_instance_p->vm_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"tenant_name    : %s",nwservice_instance_p->tenant_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"service_name   : %s",nwservice_instance_p->nwservice_object_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"switch_name    : %s",nwservice_instance_p->switch_name);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vlanid in      : %d",nwservice_instance_p->vlan_id_in);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vlanid out     : %d",nwservice_instance_p->vlan_id_out);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"vmns port name : %s",nwservice_instance_p->vm_ns_port1_name_p);
        OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"portid         : %d",nwservice_instance_p->port1_id);
      }
      CNTLR_RCU_READ_LOCK_RELEASE();
   } 
  }
  else
  {  
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"NSC_NWSERVICES_NOT_CONFIGURED");
    return OF_FAILURE;
  } 
   OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"nsc_tes15");
   CNTLR_RCU_READ_LOCK_RELEASE();
   return OF_SUCCESS;
}
/**********************************************************************************************************************************/
int32_t nsc_tst1(struct ofl_packet_in *pkt_in,uint8_t table_id,uint64_t dp_handle,uint64_t metadata)
{
  struct   of_msg* msg_p;
  uint32_t in_port_id;
  uint8_t  nw_type,pkt_origin;
  int32_t  retval;

  in_port_id = 1;

  if(table_id == 1)
  { 
    pkt_origin = (uint8_t)((metadata & PKT_ORIGIN_MASK) >> PKT_ORIGIN_OFFSET);
    nw_type = (uint8_t)(metadata >> NW_TYPE_OFFSET);

    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"pkt_origin = %d",pkt_origin);
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"nw_type    = %d",nw_type);

    retval = tsc_outbound_ns_chain_table_1_miss_entry_pkt_rcvd(dp_handle,msg_p,pkt_in,in_port_id);
  } 
   return retval;
}
/************************************************************************************************************************************/
void nsc_add_sw1vm_port1()
{
  int32_t ret_val;

  ret_val = crm_add_crm_vmport ("qvo20cbe62b-59",
                                "switch-1",
                                "br-int",
                                 VM_PORT,
                                "VN1",
                                "SWITCH1-VM",
                                 sw1vm_mac1,
                                 &sw1vm_port1_handle
                               );

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n SWITCH1-VM1 VM port addition failed");
  }
  else
    puts("\r\n SWITCH1-VM1 VM port is added successfully");
}

void nsc_add_sw2vm_port1()
{
  int32_t ret_val;

  ret_val = crm_add_crm_vmport ("qvo21cbe62c-59",
                                "switch-2",
                                "br-int",
                                 VM_PORT,
                                "VN1",
                                "SWITCH2-VM",
                                 sw1vm_mac1,
                                 &sw1vm_port1_handle
                               );

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n SWITCH1-VM1 VM port addition failed");
  }
  else
    puts("\r\n SWITCH1-VM1 VM port is added successfully");
}

void nsc_add_sw3vm_port1()
{
  int32_t ret_val;

  ret_val = crm_add_crm_vmport ("qvo22cbe62c-59",
                                "switch-3",
                                "br-int",
                                 VM_PORT,
                                "VN1",
                                "SWITCH3-VM",
                                 sw1vm_mac1,
                                 &sw1vm_port1_handle
                               );

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n SWITCH1-VM1 VM port addition failed");
  }
  else
    puts("\r\n SWITCH1-VM1 VM port is added successfully");
}

void nsc_add_nw_unicast_port1_sw1()
{
  int32_t ret_val;
  struct  crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"sw1uniport");

  nw_port_info.port_type = NW_UNICAST_PORT;
  nw_port_info.portId    = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch_1");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0;
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw1_nw_uni_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n sw1 nw unicast port1 addition failed");
  }
  else
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_ERROR," sw1 nw unicast port1 addition successful");
}

void nsc_add_nw_unicast_port1_sw2()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"sw2uniport");

  nw_port_info.port_type = NW_UNICAST_PORT;
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch_2");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0;
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw2_nw_uni_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_ERROR,"\r\n sw2 nw unicast port1 addition failed");
  }
  else
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_ERROR," sw2 nw unicast port1 addition successful");
}

void nsc_add_nw_unicast_port1_sw3()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"sw3uniport");

  nw_port_info.port_type = NW_UNICAST_PORT;
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch_3");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0;
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw3_nw_uni_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_ERROR,"\r\n sw3 nw unicast port1 addition failed");
  }
  else
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_ERROR," sw3 nw unicast port1 addition successful");
}

