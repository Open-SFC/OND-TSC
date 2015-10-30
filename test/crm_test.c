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

/*File: nsc_test.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains test module for Flow Control module.
 */

/* INCLUDE_START ************************************************************/
#include "controller.h"
#include "of_utilities.h"
#include "of_msgapi.h"
#include "cntrl_queue.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "dprmldef.h"

#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_nvm_vxlan.h"
#include "tsc_controller.h"

#define CRM_VN_ALL_NOTIFICATIONS 0 

/* DPRM Resources */ 
struct   dprm_switch_info switch_1,switch_2,switch_3;
struct   dprm_datapath_general_info datapath_info_1,datapath_info_2,datapath_info_3;
uint64_t switch1_handle,switch2_handle,switch3_handle;
uint64_t dp_handle_1,dp_handle_2,dp_handle_3;
struct   dprm_port_info port_info1,port_info2,port_info3,port_info4;
uint64_t sw1_port1_handle,sw1_port2_handle,sw1_port3_handle,sw1_port4_handle;
uint64_t sw2_port1_handle,sw2_port2_handle,sw2_port3_handle,sw2_port4_handle;
uint64_t sw3_port1_handle,sw3_port2_handle,sw3_port3_handle,sw3_port4_handle;

#define crm_debug printf

uint64_t sw1_nw_uni_port1_handle;
uint64_t sw3_nw_uni_port1_handle;

uint64_t sw1_3_brd_port1_handle; 

uint64_t sw3_1_brd_port1_handle; 

uint64_t sw1vm1_port1_handle;
uint64_t sw3vm1_port1_handle;

uint8_t  sw1vm1_mac1[6] = {0xfa,0x16,0x3e,0x28,0xcb,0x83};
uint8_t  sw3vm5_mac1[6] = {0xfa,0x16,0x3e,0xfd,0x61,0x3e};

struct crm_vm_config_info  vm_config_info1;
struct crm_vm_config_info  vm_config_info2;
struct crm_vm_config_info  vm_config_info;
struct vm_runtime_info     runtime_info;

void tsc_add_tenant1();

void tsc_add_sw1_vm1();
void tsc_add_sw3_vm5();

void tsc_add_vn1();

void tsc_add_sw1vm1_port1();

void tsc_add_sw3vm5_port1();

void tsc_add_nw_unicast_port1_sw1();
void tsc_add_nw_unicast_port1_sw3();

void tsc_add_nw_brdcast_port1_sw1_3();
void tsc_add_nw_brdcast_port2_sw1_3();

void tsc_add_nw_brdcast_port1_sw3_1();
void tsc_add_nw_brdcast_port2_sw3_1();

void printvalues_vm(); 
void printcrmretvalue(int32_t ret_value);

int32_t tsc_config_dprm();

int32_t tsc_test_app_init(uint64_t domain_handle)
{
  int32_t status = OF_SUCCESS;

  /* Configure DPRM */
  status = tsc_config_dprm(domain_handle);
  if(status != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n config DPRM resources failed");
    return OF_FAILURE;
  }

  tsc_add_tenant1();
  
  tsc_add_sw1_vm1();
  tsc_add_sw3_vm5();

  tsc_add_vn1();
 
  tsc_add_nw_brdcast_port1_sw1_3();
  tsc_add_nw_brdcast_port2_sw1_3(); 

  tsc_add_nw_brdcast_port1_sw3_1();
  tsc_add_nw_brdcast_port2_sw3_1();

  tsc_add_nw_unicast_port1_sw1();
  tsc_add_nw_unicast_port1_sw3();
 
  tsc_add_sw1vm1_port1();
  tsc_add_sw3vm5_port1();

  return OF_SUCCESS;  
} 

int32_t tsc_config_dprm(uint64_t domain_handle)
{
  int32_t status = OF_SUCCESS;

  do
  {
    /*Add switch 1 to DPRM DB*/
    strcpy(switch_1.name, "switch-1");
    switch_1.switch_type = PHYSICAL_SWITCH;
    switch_1.ipv4addr.baddr_set = TRUE;
    switch_1.ipv4addr.addr = 0x0a0a0a65;    
 
    status = dprm_register_switch(&switch_1,&switch1_handle);
    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add Switch 1 to the system fail,status %d",status);
      status = OF_FAILURE;
      break;
    }

    /*Add switch 3 to DPRM db */
    strcpy(switch_3.name, "switch-3");
    switch_3.switch_type = PHYSICAL_SWITCH;
    switch_3.ipv4addr.baddr_set = TRUE;
    switch_3.ipv4addr.addr = 0x0a0a0a67;

    status = dprm_register_switch(&switch_3,&switch3_handle);
    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add Switch 3 to the system fail,status %d",status);
      status = OF_FAILURE;
      break;
    }
    /* Add dp-1 to switch_1 */
    datapath_info_1.dpid = 0x00007a76363f4d45; 
    strcpy(datapath_info_1.datapath_name,"br-int");
    strcpy(datapath_info_1.expected_subject_name_for_authentication,"dp_1@xyz.com");
    status = dprm_register_datapath(&datapath_info_1,switch1_handle,
                                    domain_handle,&dp_handle_1);
    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,
                         "Add Data path to the system fail status=%d",status);
      status = OF_FAILURE;
      break;
    }
    
    strcpy(port_info2.port_name,"qvo20cbe62b-59");
    port_info2.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info2,&sw1_port1_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM switch1 VM1 Port1 Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM switch1 VM1 Port1 Addition Successful"); 

    strcpy(port_info3.port_name,"6JoSuF7DzQNW3C6");
    port_info3.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info3,&sw1_port3_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,
                            "Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW VxLan Port Addition Successful");

    strcpy(port_info3.port_name,"sw1_vxuni_port2");
    port_info3.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info3,&sw1_port3_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,
                            "Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW VxLan Port Addition Successful");

    strcpy(port_info4.port_name,"sw12_vxbr_port1");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw12_vxbr_port1 Addition Successful");

    strcpy(port_info4.port_name,"sw12_vxbr_port2");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw12_vxbr_port2 Addition Successful");
    
    strcpy(port_info4.port_name,"uc2ol1yAz6fEy2T");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw13_vxbr_port1 Addition Successful");

    strcpy(port_info4.port_name,"wXtuonU8gO2sv2x");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_1,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw13_vxbr_port2 Addition Successful");

    /* Add dp-3 to switch_3 */
    datapath_info_3.dpid = 0x00006eeae9a7424e;
    strcpy(datapath_info_3.datapath_name,"br-int");
    strcpy(datapath_info_3.expected_subject_name_for_authentication,"dp_3@xyz.com");
    status = dprm_register_datapath(&datapath_info_3,switch3_handle,
                                   domain_handle,&dp_handle_3);
    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add Data path dp3 to the system fail status=%d",status);
      status = OF_FAILURE;
      break;
    }

    strcpy(port_info2.port_name,"qvo2d6846e2-eb");
    port_info2.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_3,&port_info2,&sw3_port1_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM switch3 VM1 Port1 Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM switch3 VM5 Port1 Addition Successful");

    strcpy(port_info3.port_name,"lw2kJci05022hCq");
    port_info3.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_3,&port_info3,&sw3_port3_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM sw3 NW VxLan unicast Port Addition Successful");

    strcpy(port_info4.port_name,"Pbh11mnsSoE59S6");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_3,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw31_vxbr_port1 Addition Successful");
    
    strcpy(port_info4.port_name,"KQmzIFM9df9svRA");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_3,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw31_vxbr_port2 Addition Successful");
    
    strcpy(port_info4.port_name,"sw32_vxbr_port1");
    port_info4.port_id = 0;
    status = dprm_add_port_to_datapath(dp_handle_3,&port_info4,&sw1_port4_handle);

    if(status != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Add port to the datapath fail status=%d",status);
      puts("\r\n DPRM NW Broadcast Port Addition Failed ");
      status = OF_FAILURE;
      break;
    }
    puts("\r\n DPRM NW Brodcast VxLan Port sw32_vxbr_port1 Addition Successful");
  }while(0);  
  
  if(status == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"DPRM configuration failed");
    /*TBD release of all resources*/
    return OF_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"DPRM configuration is successfull");
  return OF_SUCCESS;
}

void tsc_add_tenant1()
{
  int32_t  ret_val;
  uint64_t tenant_handle;
  struct   crm_tenant_config_info tenant_config_info;

  strcpy(tenant_config_info.tenant_name, "tenant1");
  ret_val = crm_add_tenant(&tenant_config_info,&tenant_handle);
  printcrmretvalue(ret_val);
  if(ret_val==CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n tenant1 added successfully!.\n");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n failed to add tenant1 \n");
  }
}  

/* Add VM */
void tsc_add_sw1_vm1()
{
  uint64_t sw1_vm1_handle;
  int32_t  lRet_value1;

  strcpy(vm_config_info1.vm_name,"SWITCH1-VM1");
  strcpy(vm_config_info1.tenant_name,"tenant1");
  strcpy(vm_config_info1.switch_name,"switch-1");
  vm_config_info1.vm_type = VM_TYPE_NORMAL_APPLICATION;

  lRet_value1 = crm_add_virtual_machine(&vm_config_info1, &sw1_vm1_handle);

  printcrmretvalue(lRet_value1);
  
  if(lRet_value1 == CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Virtual Machine SWITCH1-VM1 Added to DB successfully");
  }
  else
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Virtual Machine SWITCH1-VM1 Added to DB failed");
}

void tsc_add_sw3_vm5()
{
  uint64_t sw3_vm1_handle;
  int32_t  lRet_value1;

  strcpy(vm_config_info1.vm_name,"SWITCH3-VM5");
  strcpy(vm_config_info1.tenant_name,"tenant1");
  strcpy(vm_config_info1.switch_name,"switch-3");
  vm_config_info1.vm_type = VM_TYPE_NORMAL_APPLICATION;

  lRet_value1 = crm_add_virtual_machine(&vm_config_info1, &sw3_vm1_handle);

  printcrmretvalue(lRet_value1);

  if(lRet_value1 == CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Virtual Machine SWITCH3-VM5 Added to DB successfully");
  }
  else
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Virtual Machine SWITCH3-VM5 Added to DB failed");
}

void tsc_add_vn1()
{
  struct   crm_vn_config_info config_info;
  struct   crm_vn_config_info config_info_output;
  struct   vn_runtime_info runtime_info;
  uint64_t output_handle;
  int32_t  retval;

  strcpy(config_info.nw_name,"virtnet1");
  strcpy(config_info.tenant_name,"tenant1");

  config_info.nw_type = VXLAN_TYPE;
  config_info.nw_params.vxlan_nw.nid = 0x123456;
  config_info.nw_params.vxlan_nw.service_port = 4789; /* 4789 Recently allocated by IANA */

  retval = crm_add_virtual_network(&config_info,&output_handle);
  if(retval != CRM_SUCCESS)
  {  
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Add vn1 error value = %d",retval);
    return;   
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n VN1 addition successful");
    retval = crm_get_vn_by_name("virtnet1", &config_info_output, &runtime_info);
    
    if(retval != CRM_SUCCESS)
    {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Failed to get VN1 by name");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Successfully obtained VN1 by name");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n VN name = %s",config_info_output.nw_name);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n VN type = %d",config_info_output.nw_type);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n VN vni = %x",config_info_output.nw_params.vxlan_nw.nid);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n VN service port = %d",config_info_output.nw_params.vxlan_nw.service_port);
    }  
  }
  retval =  crm_add_crm_compute_node_to_vn("virtnet1","switch-1","br-int");
  retval =  crm_add_crm_compute_node_to_vn("virtnet1","switch-2","br-int");
  retval =  crm_add_crm_compute_node_to_vn("virtnet1","switch-3","br-int");
}

void tsc_add_nw_unicast_port1_sw1()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"6JoSuF7DzQNW3C6");
  
  nw_port_info.port_type = NW_UNICAST_PORT; 
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE; 
  strcpy(nw_port_info.switch_name,"switch-1"); 
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
    puts("\r\n sw1 nw unicast port1 addition successful");
}

void tsc_add_nw_unicast_port1_sw3()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"lw2kJci05022hCq");

  nw_port_info.port_type = NW_UNICAST_PORT;
  nw_port_info.portId = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch-3");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0;
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw3_nw_uni_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n sw3 nw unicast port addition failed");
  }
  else
    puts("\r\n sw3 nw unicast port addition successful");
}

void tsc_add_nw_brdcast_port1_sw1_3()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"uc2ol1yAz6fEy2T");

  nw_port_info.port_type = NW_BROADCAST_PORT;
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch-1");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0; /* nss 0x123456; */
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0x0a0a0a67;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw1_3_brd_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n sw1_3 nw broadcast port1 addition failed");
  }
  else
    puts("\r\n sw1_3 nw broadcast port1 addition successful");
}

void tsc_add_nw_brdcast_port2_sw1_3()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"wXtuonU8gO2sv2x");

  nw_port_info.port_type = NW_BROADCAST_PORT;
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch-1");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0; /* nss 0x123456; */
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0x0a0a0a64;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw1_3_brd_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n sw1_3 nw broadcast port2 addition failed");
  }
  else
    puts("\r\n sw1_3 nw broadcast port2 addition successful");
}

void tsc_add_nw_brdcast_port1_sw3_1()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"Pbh11mnsSoE59S6");

  nw_port_info.port_type = NW_BROADCAST_PORT;
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch-3");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0; /* 0x123456; */
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0x0a0a0a65;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw3_1_brd_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n sw3_1 nw broadcast port1 addition failed");
  }
  else
    puts("\r\n sw3_1 nw broadcast port1 addition successful");
}

void tsc_add_nw_brdcast_port2_sw3_1()
{
  int32_t ret_val;
  struct crm_nwport_config_info nw_port_info;

  strcpy(nw_port_info.port_name,"KQmzIFM9df9svRA");

  nw_port_info.port_type = NW_BROADCAST_PORT;
  nw_port_info.portId   = 0;
  nw_port_info.nw_type   = VXLAN_TYPE;
  strcpy(nw_port_info.switch_name,"switch-3");
  strcpy(nw_port_info.br_name,"br-int");
  nw_port_info.nw_port_config_info.vxlan_info.nid = 0; /* 0x123456; */
  nw_port_info.nw_port_config_info.vxlan_info.service_port = 4789;
  nw_port_info.nw_port_config_info.vxlan_info.remote_ip = 0x0a0a0a64;

  ret_val = crm_add_crm_nwport(&nw_port_info,&sw3_1_brd_port1_handle);

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n sw3_1 nw broadcast port2 addition failed");
  }
  else
    puts("\r\n sw3_1 nw broadcast port2 addition successful");
}

void tsc_add_sw1vm1_port1()
{
  int32_t ret_val;

  ret_val = crm_add_crm_vmport("qvo20cbe62b-59",
                               "switch-1",
                               "br-int",
                                VM_PORT,
                               "virtnet1",
                               "SWITCH1-VM1",
                                sw1vm1_mac1,
                                0x0c0c0d05,
                                &sw1vm1_port1_handle
                               );

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n SWITCH1-VM1 VM port addition failed");
  }
  else
    puts("\r\n SWITCH1-VM1 VM port is added successfully");
}

void tsc_add_sw3vm5_port1()
{
  int32_t ret_val;

  ret_val = crm_add_crm_vmport("qvo2d6846e2-eb",
                               "switch-3",
                               "br-int",
                                VM_PORT,
                               "virtnet1",
                               "SWITCH3-VM5",
                                sw3vm5_mac1,
                                0x0c0c0d06,
                                &sw3vm1_port1_handle
                              );

  if(ret_val != CRM_SUCCESS)
  {
    puts("\r\n SWITCH3-VM5 VM port addition failed");
  }
  else
    puts("\r\n SWITCH3-VM5 VM port is added successfully");
}

void printvalues_vm()
{
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"\r\n Virtual Machine Name = %s",vm_config_info.vm_name);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"\r\n Virtual Machin  Type = %d",vm_config_info.vm_type);

  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"\r\n Virtual Machine Switch Name = %s",vm_config_info.switch_name);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"\r\n Virtual Machine Tenant Name = %s",vm_config_info.tenant_name);
}

void printcrmretvalue(int32_t ret_value)
{
  if(ret_value == CRM_ERROR_INVALID_VM_TYPE)
    crm_debug("\r\n Virtual Machine Type Invalid!. \r\n");
  else if(ret_value == CRM_ERROR_DUPLICATE_VM_RESOURCE)
    crm_debug("\r\n Virtual Machine Type Duplicate!. \r\n");
  else if(ret_value == CRM_ERROR_VM_NAME_INVALID)
    crm_debug("\r\n Virtual Machine Name not in database!. \r\n");
  else if(ret_value == CRM_ERROR_VM_NAME_NULL)
    crm_debug("\r\n Virtual Machine Name null!. \r\n");
  else if(ret_value == CRM_ERROR_NO_MORE_ENTRIES)
    crm_debug("\r\n No More Entries in the database \r\n");
  else if(ret_value == CRM_ERROR_TENANT_NAME_INVALID)
    crm_debug("\r\n Tenant name invalid \r\n");
  else if(ret_value == CRM_ERROR_DUPLICATE_TENANT_RESOURCE)
    crm_debug("\r\n Tenant name duplicate \r\n");
  else if(ret_value == CRM_ERROR_TENANT_ADD_FAIL)
    crm_debug("\r\n Tenant add fail\r\n");
}


