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

/*File: tsc_ofv1_3_plugin.c 
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This module contains OpenFlow version 1.3 specific code.
 **/

/* INCLUDE_START ************************************************************/
#include "controller.h"
#include "of_utilities.h"
#include "of_multipart.h"
#include "of_msgapi.h"
#include "cntrl_queue.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "dprmldef.h"

#include "crmapi.h"
#include "tsc_controller.h"
#include "tsc_ofv1_3_plugin.h"
#include "tsc_ofplugin_v1_3_ldef.h"
#include "nicira_tun_ext.h"
#include "of_utilities.h"
/* INCLUDE_END **************************************************************/

#define TSC_L2_SERVICE_CHAINING_ENABLE 1

#define TSC_APP_PKT_IN_PRIORITY  2
#define TSC_APP_PKT_FLOW_IN_PRIORITY 104

#define TSC_PKT_UNICAST   1
#define TSC_PKT_BROADCAST 2

#define TSC_PKT_UNICAST_VM_APPLN     3
#define TSC_PKT_UNICAST_VMNS_NW      4
#define TSC_PKT_UNICAST_ARP          5
#define TSC_PKT_UNICAST_DNS_UDP      6
#define TSC_PKT_UNICAST_DHCP_UDP     7
#define TSC_PKT_UNICAST_DHCP_UDP_67  8


uint8_t ucast_mac_addr_chk[6] = {00,00,00,00,00,00};
uint8_t ucast_mac_mask[6]     = {01,00,00,00,00,00};
uint8_t bcast_mac_addr_chk[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
uint8_t bcast_mac_mask[6]     = {0xff,0xff,0xff,0xff,0xff,0xff};

uint16_t    flags;

int32_t 
tsc_ofplugin_v1_3_add_table_miss_entry(uint64_t datapath_handle,uint8_t table_id);

int32_t tsc_ofplugin_v1_3_del_table_miss_entry(uint64_t datapath_handle,uint8_t table_id);

int32_t tsc_del_flwmod_1_3_msg_table_0(uint64_t dp_handle,
                                       uint16_t msg_len,
                                       uint32_t *input_port_id_p,
                                       uint8_t  pkt_type
                                      );

int32_t tsc_add_flwmod_1_3_msg_table_0(uint64_t  dp_handle,
                                       uint16_t  msg_len,
                                       uint16_t  flow_priority,     
                                       uint32_t  *input_port_id_p,
                                       uint8_t   pkt_type,
                                       uint16_t  *vlan_id_p,
                                       uint8_t   vlan_mask_req,
                                       uint16_t  *vlan_id_mask_p,
                                       uint64_t  metadata,
                                       uint64_t  metadata_mask,
                                       uint8_t   goto_table_id
                                      );
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_init
Input Parameters:
	domain_handle: Handle to "TSC_DOMAIN" which identifies Traffic Steering Application.
Output Parameters:
	NONE
Description:This Function registers with the OF-Controller infrastructure to receive Packet-in messages from Logical Switches.
            For Table 0, flows are added proactively and Packet-in mesages are not expected.
            Packet-in messages received for Tables 1-5 are processed and Flows are added to logical switches using 
            NBAPI which uses OF-Flowmod messages. 
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_init(uint64_t domain_handle)
{
  int32_t status;
  
  do
  {
    #if 1    
    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_CLASSIFY_TABLE_ID_0,
                                                   OF_ASYNC_MSG_PACKET_IN_EVENT,TSC_APP_PKT_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_table_0_miss_entry_pkt_rcvd,
                                                   (void *)(long)TSC_APP_CLASSIFY_TABLE_ID_0,NULL);

    #endif

    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1,
                                                   OF_ASYNC_MSG_PACKET_IN_EVENT,TSC_APP_PKT_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_table_1_miss_entry_pkt_rcvd,
                                                   (void *)(long)TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1,NULL);
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of OUTBOUND NS CHAIN TABLE 1");
      break;
    }

    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1,
                                                   OF_ASYNC_MSG_FLOW_REMOVED_EVENT,TSC_APP_PKT_FLOW_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_nschain_table_1_flow_entry_removed_rcvd,
                                                   (void *)(long)TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1,NULL);
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of OUTBOUND NS CHAIN TABLE 1");
      break;
    }

    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2,
                                                   OF_ASYNC_MSG_PACKET_IN_EVENT,TSC_APP_PKT_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_table_2_miss_entry_pkt_rcvd,
                                                   (void *)(long)TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2,NULL);
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of INBOUND NS CHAIN TABLE 2");
      break;
    }

      status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2,
                                                     OF_ASYNC_MSG_FLOW_REMOVED_EVENT,TSC_APP_PKT_FLOW_IN_PRIORITY,
                                                     tsc_ofplugin_v1_3_nschain_table_2_flow_entry_removed_rcvd,
                                                     (void *)(long)TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2,NULL);
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of INBOUND NS CHAIN TABLE 2");
      break;
    }

    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_UNICAST_TABLE_ID_3, 
                                                   OF_ASYNC_MSG_PACKET_IN_EVENT,TSC_APP_PKT_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_table_3_miss_entry_pkt_rcvd,
                                                   (void *)(long)TSC_APP_UNICAST_TABLE_ID_3,NULL);  
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of VM UNICAST OUTBOUND table");
      break;
    }

    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_UNICAST_TABLE_ID_3,
                                                   OF_ASYNC_MSG_FLOW_REMOVED_EVENT,TSC_APP_PKT_FLOW_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_fc_table_flow_entry_removed_rcvd,
                                                   (void *)(long)TSC_APP_UNICAST_TABLE_ID_3,NULL);
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of VM UNICAST OUTBOUND table");
      break;
    }

    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4,
                                                   OF_ASYNC_MSG_PACKET_IN_EVENT,TSC_APP_PKT_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_table_4_miss_entry_pkt_rcvd,
                                                   (void *)(long)TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4,NULL);                                                           
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of VM BROADCAST OUTBOUND table");
      break;
    }
    
    status = of_register_asynchronous_message_hook(domain_handle, TSC_APP_BROADCAST_INBOUND_TABLE_ID_5,
                                                   OF_ASYNC_MSG_PACKET_IN_EVENT,TSC_APP_PKT_IN_PRIORITY,
                                                   tsc_ofplugin_v1_3_table_5_miss_entry_pkt_rcvd,
                                                   (void *)(long)TSC_APP_BROADCAST_INBOUND_TABLE_ID_5,NULL);                                                           
    if(status == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_INFO,"registering sample apps to Asyc message of NW UNICAST INBOUND table");
      break;
    }

  }while(0);

  if(status == OF_FAILURE)
  {
    /* TBD no unregister API available */    
    return status;
  } 

  status = cntlr_register_asynchronous_event_hook(domain_handle,DP_READY_EVENT,1,
                                                  tsc_ofplugin_v1_3_dp_ready_event,NULL,NULL);
  if(status == OF_FAILURE) 
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Register DP Ready event cbk failed");
    /* TBD no unregister API available */
  } 
  status = cntlr_register_asynchronous_event_hook(domain_handle,DP_DETACH_EVENT,1,
                                                  tsc_ofplugin_v1_3_dp_detach_event,NULL,NULL);
  if(status == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Register DP Detach event cbk failed");
    return OF_FAILURE;
  }
 
  return status;   
}
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_dp_ready_event
Input Parameters:
        domain_handle  : Handle to "TSC_DOMAIN" which identifies Traffic Steering Application.
        datapath_handle: Identifies the logical switch that is connected to controller.
Output Parameters:
        NONE
Description:The Controller infrastructure raises this event when OF handshake is completed with a logical switch.
****************************************************************************************************************************/
int32_t
tsc_ofplugin_v1_3_dp_ready_event(uint64_t  controller_handle,
                      uint64_t  domain_handle,
                      uint64_t  datapath_handle,
                      void      *cbk_arg1,
                      void      *cbk_arg2)
{
  uint8_t table_id;
  struct dprm_datapath_entry *pdatapath_info;
  int32_t status = OF_SUCCESS;
  
  do 
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"DATA PATH READY EVENT RECEIVED");
    if(get_datapath_byhandle(datapath_handle, &pdatapath_info) != DPRM_SUCCESS)
    { 
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_WARN, "No DP Entry with given datapath_handle=%llu",datapath_handle);
      status = OF_FAILURE;
      break;
    }   
    /* Add miss entries to the OF tables 1,2,3,4,5 */
    for(table_id = 1; table_id < 6; table_id++)
    {
      if(tsc_ofplugin_v1_3_add_table_miss_entry(datapath_handle,table_id) != OF_SUCCESS)
      {
        status = OF_FAILURE;
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Adding miss entries failed for table %d ",table_id);
        break;
      }  
    }
  }while(0);
  return status;
}  
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_add_table_miss_entry
Input Parameters:
        datapath_handle: Identifies the logical switch that is connected to controller.
        table_id       : Identifies the OF Table for which a miss entry is to be added.
Output Parameters:
        NONE
Description:This function adds a miss entry in the given logical switch for the required OF Table to receive packets from
            the logical switch that could not be matched with any flow present in the logical switch.
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_add_table_miss_entry(uint64_t datapath_handle,uint8_t table_id)
{
  struct   of_msg *msg;
  uint16_t msg_len,instruction_len,action_len,flags;
  uint8_t* inst_start_loc = NULL;
  uint16_t match_fd_len=0;
  uint8_t* match_start_loc = NULL;
  uint8_t* action_start_loc = NULL;
  int32_t  retval = OF_SUCCESS;
  int32_t  status = OF_SUCCESS;
  void*    conn_info_p;  
                                                
  msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
             OFU_APPLY_ACTION_INSTRUCTION_LEN+OFU_OUTPUT_ACTION_LEN);
 
  do                  
  {                   
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)  
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
       break;
    }
    flags = 0;
    flags |= OFPFF_RESET_COUNTS;
    retval = of_create_add_flow_entry_msg(msg,datapath_handle,table_id, 
                                          0, /* Low priority */
                                          OFPFC_ADD,
                                          0, /* cookie */
                                          0, /* Cookie mask */
                                          OFP_NO_BUFFER,flags,
                                          0, /* No Hard timeout */
                                          0, /* No Idle timeout */
                                          &conn_info_p
                                          );

   if(retval != OFU_ADD_FLOW_ENTRY_TO_TABLE_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding miss entry arp table = %d",retval);
     status = OF_FAILURE;
   }
   ofu_start_setting_match_field_values(msg);
   ofu_end_setting_match_field_values(msg,match_start_loc,&match_fd_len);

   ofu_start_pushing_instructions(msg, match_fd_len);
   retval = ofu_start_pushing_apply_action_instruction(msg);
   if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding apply action instruction err = %d",retval);
     status = OF_FAILURE;
     break;
   }

   retval = ofu_push_output_action(msg,OFPP_CONTROLLER, OFPCML_NO_BUFFER);
   if(retval != OFU_ACTION_PUSH_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding output action  err = %d",retval);
     status = OF_FAILURE;
     break;
   }
   ofu_end_pushing_actions(msg,action_start_loc,&action_len);
   ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);
   ofu_end_pushing_instructions(msg,inst_start_loc,&instruction_len);

   retval=of_cntrlr_send_request_message(msg,datapath_handle,conn_info_p,NULL,NULL);
   if(retval != OF_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding miss entry to table = %d %d",retval,table_id);
     status = OF_FAILURE;
   }
 }
 while(0);
 return status;
}
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        tsc_params_p      : Parameters required to add/delete a flow.
        flow_mod_operation: OFPFC_ADD->Add a flow.OFPFC_DELETE->Delete a Flow
Output Parameters:
        NONE
Description:This function adds/delete flows to classification table 0.These flows are added proactively.
            A flow for each network port is added as and when they are added and deleted as and when they are deleted.
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports(uint64_t dp_handle,
                                                               struct tsc_ofver_plugin_iface* tsc_params_p,
                                                               uint8_t flow_mod_operation)
{         
  int32_t  retval;
  int32_t  status = OF_SUCCESS;
  uint16_t msg_len;
  uint16_t vlan_id,vlan_id_mask;
  uint8_t  vlan_mask_req; /* 1=required 0= not required */
  uint8_t  next_table_id;
  
  do
  {  
      msg_len = FLMOD_MSG_LEN_NW_PORT_OF_1_3;

      vlan_id       = 0x0000;  
      vlan_id_mask  = 0x0000;  /* Mask not to be specified */
      vlan_mask_req = FALSE;
      next_table_id = TSC_APP_UNICAST_TABLE_ID_3; 
      
      if(flow_mod_operation ==  OFPFC_ADD)
      {
        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               1,
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_UNICAST,
                                               &vlan_id,vlan_mask_req,&vlan_id_mask, 
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               next_table_id);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

      #if 0
        vlan_id       = 0x1000;  
        vlan_id_mask  = 0x1000;  /* Mask to be specified */
        vlan_mask_req = TRUE;
      #endif

        #ifdef TSC_L2_SERVICE_CHAINING_ENABLE
          next_table_id = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
        #endif
        
        if(tsc_params_p->vlan_id_in != 0)  
        {
          retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                  msg_len,
                                                  2,
                                                  &(tsc_params_p->in_port_id),
                                                  TSC_PKT_UNICAST_VMNS_NW,
                                                  &(tsc_params_p->vlan_id_in),vlan_mask_req,&vlan_id_mask,
                                                  tsc_params_p->metadata,
                                                  tsc_params_p->metadata_mask,
                                                  next_table_id);
          if(retval != OF_SUCCESS)
          {
            status = OF_FAILURE;
            break;
          }

        }
        if(tsc_params_p->vlan_id_out != 0)
        {    
          retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                  msg_len,
                                                  2,
                                                  &(tsc_params_p->in_port_id),
                                                  TSC_PKT_UNICAST_VMNS_NW,
                                                  &(tsc_params_p->vlan_id_out),vlan_mask_req,&vlan_id_mask,
                                                  tsc_params_p->metadata,
                                                  tsc_params_p->metadata_mask,
                                                  next_table_id);
          if(retval != OF_SUCCESS)
          {
            status = OF_FAILURE;
            break;
          }
        }   

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               3,
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_UNICAST_ARP,
                                               &vlan_id,vlan_mask_req,&vlan_id_mask,
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DNS_UDP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DHCP_UDP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DHCP_UDP_67,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
            status = OF_FAILURE;
            break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                1,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_BROADCAST,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_BROADCAST_INBOUND_TABLE_ID_5);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      }
      else if(flow_mod_operation ==  OFPFC_DELETE) 
      {
        retval = tsc_del_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                &(tsc_params_p->in_port_id),  
                                                TSC_PKT_UNICAST
                                               );
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_del_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_BROADCAST
                                               );
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      }
      else
       status = OF_FAILURE;
  }while(0);
  if(status == OF_FAILURE)
  {
    /* free msg */
  }
  return status;
}
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_send_flow_mod_classify_table_vmports
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        tsc_params_p      : Parameters required to add/delete a flow.
        flow_mod_operation: OFPFC_ADD->Add a flow.OFPFC_DELETE->Delete a Flow
Output Parameters:
        NONE
Description:This function adds/delete flows to classification table 0.These flows are added proactively.
            A flow for each VM port is added as and when they are added and deleted as and when they are deleted.
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_send_flow_mod_classify_table_vmports(uint64_t dp_handle,
                                       struct tsc_ofver_plugin_iface* tsc_params_p,
                                       uint8_t flow_mod_operation)
{
  uint16_t msg_len;
  uint16_t vlan_id,vlan_id_mask;
  uint8_t  vlan_mask_req; 
  int32_t  retval;
  int32_t  status = OF_SUCCESS;
  uint8_t  next_table_id = TSC_APP_UNICAST_TABLE_ID_3;

  if(tsc_params_p->pkt_origin_e == VM_PORT)
  {
    do
    {
      msg_len = FLMOD_MSG_LEN_VM_PORT_OF_1_3;

      vlan_id       = 0x1000;  /* Match any VLAN ID but with a 802.1Q Header */
      vlan_id_mask  = 0x1000;
      vlan_mask_req = TRUE;

      #ifdef TSC_L2_SERVICE_CHAINING_ENABLE
        next_table_id = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
      #endif

      if(flow_mod_operation ==  OFPFC_ADD)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," vm port id = %d ",tsc_params_p->in_port_id);
        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               2,       
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_UNICAST,
                                               &vlan_id,vlan_mask_req,&vlan_id_mask, /* not used */
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               next_table_id);
   
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," vm port id = %d ",tsc_params_p->in_port_id);
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," vm port dp handle = %x ",dp_handle);
 
        vlan_id       = 0x0000;
        vlan_id_mask  = 0x0000;  /* Mask not to be specified */
        vlan_mask_req = FALSE;

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                1,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask, /* not used */
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                next_table_id);

        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_ARP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
    
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," vm port id = %d ",tsc_params_p->in_port_id);

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DNS_UDP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }


        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DHCP_UDP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
         
        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DHCP_UDP_67,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
            status = OF_FAILURE;
            break;
        }

        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," vm port id = %d ",tsc_params_p->in_port_id);

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               1,
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_BROADCAST,
                                               &vlan_id,vlan_mask_req,&vlan_id_mask,
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      }
      else if(flow_mod_operation ==  OFPFC_DELETE)
      {
        retval = tsc_del_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST
                                               );
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_del_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_BROADCAST
                                               );
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      }
      else
       status = OF_FAILURE;
    }while(0);
  } 
  else if(tsc_params_p->pkt_origin_e == VMNS_PORT)  
  { 
    do
    {
      msg_len = FLMOD_MSG_LEN_VM_NS_PORT_OF_1_3;  
    
      vlan_id       = 0x0000;  
      vlan_id_mask  = 0x0000;  /* Mask not to be specified */
      vlan_mask_req = FALSE;
      next_table_id = TSC_APP_UNICAST_TABLE_ID_3;

      if(flow_mod_operation ==  OFPFC_ADD)
      {
        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               1, 
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_UNICAST, 
                                               &vlan_id,vlan_mask_req,&vlan_id_mask, /* Not Used */
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               next_table_id);

        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
        
      #if 0 
        vlan_id       = 0x1000;  
        vlan_id_mask  = 0x0000;  /* Mask not to be specified */
        vlan_mask_req = FALSE;
      #endif

    #ifdef TSC_L2_SERVICE_CHAINING_ENABLE
        next_table_id = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
    #endif

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               2,
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_UNICAST_VMNS_NW,
                                               &(tsc_params_p->vlan_id_out),vlan_mask_req,&vlan_id_mask,
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               next_table_id);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                2,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_VMNS_NW,
                                                &(tsc_params_p->vlan_id_in),vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                next_table_id);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      
        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_ARP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DNS_UDP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DHCP_UDP,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
        
        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                3,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST_DHCP_UDP_67,
                                                &vlan_id,vlan_mask_req,&vlan_id_mask,
                                                tsc_params_p->metadata,
                                                tsc_params_p->metadata_mask,
                                                TSC_APP_UNICAST_TABLE_ID_3);
        if(retval != OF_SUCCESS)
        {
            status = OF_FAILURE;
            break;
        }



        retval = tsc_add_flwmod_1_3_msg_table_0(dp_handle,
                                               msg_len,
                                               1,
                                               &(tsc_params_p->in_port_id),
                                               TSC_PKT_BROADCAST,
                                               &(tsc_params_p->vlan_id_out),vlan_mask_req,&vlan_id_mask,
                                               tsc_params_p->metadata,
                                               tsc_params_p->metadata_mask,
                                               TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4);
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      }
      else if(flow_mod_operation ==  OFPFC_DELETE)
      {
        retval = tsc_del_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_UNICAST
                                               );
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }

        retval = tsc_del_flwmod_1_3_msg_table_0(dp_handle,
                                                msg_len,
                                                &(tsc_params_p->in_port_id),
                                                TSC_PKT_BROADCAST
                                               );
        if(retval != OF_SUCCESS)
        {
          status = OF_FAILURE;
          break;
        }
      }
      else
       status = OF_FAILURE;
    }while(0);
  } 
  else
   status = OF_FAILURE;
 
  if(status == OF_FAILURE)
  {
    /* TBD Free messages that are successfully created */
  }
  return status;  
}
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_del_flows_from_table_4_5
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        tsc_params_p      : Parameters required to delete  flows.
        table_id          : Identifies Table 4 or 5.
Output Parameters:
        NONE
Description:This function deletes the matching flow entries from Table 4 or Table 5. Match is performed using metadata.
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_del_flows_from_table_4_5(uint64_t dp_handle,struct tsc_ofver_plugin_iface* tsc_params_p,uint8_t table_id)
{
  struct   of_msg *msg;
  uint16_t flags;
  uint8_t* match_start_loc_p = NULL;
  uint16_t match_fd_len = 0,msg_len;
  void*    conn_info_p;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;


  msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
              OFU_META_DATA_FIELD_LEN+
              OFU_ETH_TYPE_FIELD_LEN+
              OFU_APPLY_ACTION_INSTRUCTION_LEN+
              OFU_GOTO_TABLE_INSTRUCTION_LEN
            );

  do
  {
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }
    flags = 0;

    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);
    retval = of_create_delete_flow_entries_msg_of_table(msg,
                                                        dp_handle,
                                                        table_id,
                                                        flags,
                                                        0, /* cookie */
                                                        0, /* cookie_mask */
                                                        OFPP_NONE, /* out_port    */
                                                        OFPP_NONE, /* out_group   */
                                                        &conn_info_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error deleting flow entry in table 4 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);


    /* MF1: set match field Metadata */

    retval = of_set_meta_data(msg,&(tsc_params_p->metadata),TRUE,&(tsc_params_p->metadata_mask));
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in Metadata field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* match_start_loc_p points to the first match field metadata */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Del Flow Mod message to Table 4 = %d",retval);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Del Flow Mod message to Table 4 successfully");

  }while(0);
  return status;
}
/***************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_del_table_1_2_cnbind_flow_entries(struct nschain_repository_entry* nschain_repository_node_p,
                                                            uint8_t table_id)
{
  struct   of_msg *msg;
  uint16_t flags;
  uint8_t* match_start_loc_p = NULL;
  uint16_t match_fd_len = 0,msg_len;
  void*    conn_info_p;
  uint16_t eth_type = 0x0800; /*Eth type is IP*/
  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
             OFU_IN_PORT_FIELD_LEN+
             OFU_ETH_TYPE_FIELD_LEN+
             OFU_IPV4_SRC_MASK_LEN+
             OFU_IPV4_DST_MASK_LEN+
             OFU_IP_PROTO_FIELD_LEN+
             OFU_TCP_SRC_FIELD_LEN+
             OFU_TCP_DST_FIELD_LEN+
             OFU_OUTPUT_ACTION_LEN+
             OFU_APPLY_ACTION_INSTRUCTION_LEN+
             OFU_GOTO_TABLE_INSTRUCTION_LEN
            );

  do
  {
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      printf("\r\n Failed to alloc msg for del_flow_mod_msg for  Table = %d",table_id);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }
    flags = 0;
    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);

    retval = of_create_delete_flow_entries_msg_of_table(msg,
                                                        nschain_repository_node_p->dp_handle,
                                                        table_id,
                                                        flags,
                                                        0, 
                                                        0,
                                                        OFPP_NONE, /* out_port    */
                                                        OFPP_NONE, /* out_group   */
                                                        &conn_info_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error deleting flow entry in table 1 or 2 = %d",retval);
      printf("\r\n Failed to create del_flow_entry_msg for Table = %d",table_id);
      status = OF_FAILURE;
      break;
    }
   
    ofu_start_setting_match_field_values(msg);

    retval = of_set_ether_type(msg,&eth_type);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set IPv4 src field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* Set packet fields as match fields */
    retval = of_set_ipv4_destination(msg,&nschain_repository_node_p->selector.dst_ip,FALSE,0);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set destination ip match field");
      status = OF_FAILURE;
      break;
    }

    retval = of_set_ipv4_source(msg,&nschain_repository_node_p->selector.src_ip,FALSE,0);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set source ip match field");
      status = OF_FAILURE;
      break;
    }

    retval = of_set_protocol(msg,&nschain_repository_node_p->selector.protocol);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ip protocol match field ");
      status = OF_FAILURE;
      break;
    }
    
    if(nschain_repository_node_p->selector.protocol == OF_IPPROTO_TCP)
    {
      retval = of_set_tcp_source_port(msg,&nschain_repository_node_p->selector.src_port);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp source port match field ");
        status = OF_FAILURE;
        break;
      }
      retval = of_set_tcp_destination_port(msg,&nschain_repository_node_p->selector.dst_port);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp destination port match field ");
        status = OF_FAILURE;
        break;
      }
    }                                          
    else if (nschain_repository_node_p->selector.protocol == OF_IPPROTO_UDP)
    {
      retval = of_set_udp_source_port(msg,&nschain_repository_node_p->selector.src_port);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp source port match field ");
        status = OF_FAILURE;
        break;
      }

      retval = of_set_udp_destination_port(msg,&nschain_repository_node_p->selector.dst_port);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set udp destination port match field ");
        status = OF_FAILURE;
        break;
      }
    }
    else if(nschain_repository_node_p->selector.protocol == OF_IPPROTO_ICMP)
    {
      retval = of_set_icmpv4_type(msg,&(nschain_repository_node_p->selector.icmp_type));
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set icmp type match field");
        status = OF_FAILURE;
        break;
      }
      retval = of_set_icmpv4_code(msg,&(nschain_repository_node_p->selector.icmp_code));
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set icmp code match field");
        status = OF_FAILURE;
        break;
      }
    }
    
    /* match_start_loc_p points to the first match field metadata */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    retval = of_cntrlr_send_request_message(msg,nschain_repository_node_p->dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Del Flow Mod message to Table  = %d",table_id);
      printf("\r\n Failed to send Del Flow Mod message to Table = %d",table_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Del Flow Mod message to Table successfully = %d",table_id);
    //printf("\r\n Sent Del Flow Mod message to Table successfully = %d",table_id);
  }while(0);

  return status;
}
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_del_flows_from_table_1_2
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        port_id_p         : Input port on which packets are received.
        table_id          : Identifies Table 1 or 2.
Output Parameters:
        NONE
Description:This function deletes the matching flow entries from Table 1 or Table 2. Match is performed using in port.
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_del_flows_from_table_1_2(uint64_t dp_handle,uint32_t *port_id_p,uint8_t table_id)
{
  struct   of_msg *msg;
  uint16_t flags;
  uint8_t* match_start_loc_p = NULL;
  uint16_t match_fd_len = 0,msg_len;
  void*    conn_info_p;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
              OFU_IN_PORT_FIELD_LEN+
              OFU_ETH_TYPE_FIELD_LEN+
              OFU_APPLY_ACTION_INSTRUCTION_LEN+
              OFU_GOTO_TABLE_INSTRUCTION_LEN
            );
  
  do
  { 
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }
    flags = 0;

    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);
    retval = of_create_delete_flow_entries_msg_of_table(msg,
                                                        dp_handle,
                                                        table_id,
                                                        flags,
                                                        0, /* cookie */
                                                        0, /* cookie_mask */
                                                        OFPP_NONE, /* out_port    */
                                                        OFPP_NONE, /* out_group   */
                                                        &conn_info_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error deleting flow entry in table 1 or 2 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);

    retval = of_set_in_port(msg,port_id_p);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set port match field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    /* Send the message to DP */
    
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," table_1_2 port_id = %x",*port_id_p);

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," table_1_2 dp_handle = %llx",dp_handle);
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Del Flow Mod message to Table  = %d",table_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Del Flow Mod message to Table successfully = %d",table_id);
  }while(0);

  return status;
}
/****************************************************************************************************************************
Function Name:tsc_ofplugin_v1_3_del_flows_from_table_3
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        port_macaddr      : MAC Address identifying the destination VM.
        table_id          : Identifies Table 3.
Output Parameters:
        NONE
Description:This function deletes the matching flow entries from Table 3. Match is performed using in Destination MAC.
****************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_del_flows_from_table_3(uint64_t dp_handle,uint8_t *port_macaddr)
{
  struct   of_msg *msg;
  uint16_t flags;
  uint8_t* match_start_loc_p = NULL;
  uint16_t match_fd_len = 0,msg_len;
  void*    conn_info_p;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;
  uint8_t  ii;

  msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
              OFU_META_DATA_FIELD_LEN+
              OFU_ETH_TYPE_FIELD_LEN+
              OFU_APPLY_ACTION_INSTRUCTION_LEN+
              OFU_GOTO_TABLE_INSTRUCTION_LEN
            );
  do
  {
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }
    flags = 0;

    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);
    retval = of_create_delete_flow_entries_msg_of_table(msg,
                                                        dp_handle,
                                                        TSC_APP_UNICAST_TABLE_ID_3,
                                                        flags,
                                                        0, /* cookie */
                                                        0, /* cookie_mask */
                                                        OFPP_NONE, /* out_port    */
                                                        OFPP_NONE, /* out_group   */
                                                        &conn_info_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error deleting flow entry in table 3 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);

    retval = of_set_destination_mac(msg,port_macaddr,FALSE,0);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set Destination mac field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sending Del Flow Mod message to Table 3");

    for(ii=0;ii<6;ii++)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," table_3 MAC Address = %x",port_macaddr[ii]);
    }    

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," table_3 dp_handle = %llx",dp_handle);

    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Del Flow Mod message to Table 3 = %d",retval);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Del Flow Mod message to Table 3 successfully.");

  }while(0);

  return status;
}
/****************************************************************************************************************************
Delete All Flows in a given Table using dp_handle 
****************************************************************************************************************************/  
int32_t tsc_del_flowmod_1_3_all_flows_in_a_table(uint64_t dp_handle,uint8_t table_id)
{
  struct   of_msg *msg;
  uint16_t flags,msg_len;
  void*    conn_info_p;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  do
  {
    msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
               OFU_META_DATA_FIELD_LEN+
               OFU_ETH_TYPE_FIELD_LEN+
               OFU_APPLY_ACTION_INSTRUCTION_LEN+
               OFU_GOTO_TABLE_INSTRUCTION_LEN
              );

    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }
   
    flags = 0;

    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);
    retval = of_create_delete_flow_entries_msg_of_table(msg,
                                                        dp_handle,
                                                        table_id,
                                                        flags,
                                                        0, /* cookie */
                                                        0, /* cookie_mask */
                                                        OFPP_NONE, /* out_port    */
                                                        OFPP_NONE, /* out_group   */
                                                        &conn_info_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error deleting flow entry in table 0 = %d",retval);
      status = OF_FAILURE;
      break;
    }
    
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Del All Flows Mod message to Table  = %d",table_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Del All Flow Mod message to Table = %d",table_id);

  }while(0);
  return status;
}
/****************************************************************************************************************************
Function Name:tsc_del_flwmod_1_3_msg_table_0
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        input_port_id_p   : Input port to be usesd as match field to delete flows.
        pkt_type          : Unicast or Broadcast packet flows.
Output Parameters:
        NONE
Description:This function deletes the matching flow entries from classification Table 0.Match is performed using in port id.
****************************************************************************************************************************/
int32_t tsc_del_flwmod_1_3_msg_table_0(uint64_t dp_handle,
                                       uint16_t msg_len,
                                       uint32_t *input_port_id_p,
                                       uint8_t  pkt_type
                                      )
{
  struct   of_msg *msg;
  uint16_t flags;
  uint8_t* match_start_loc_p = NULL;
  uint16_t match_fd_len = 0;
  void*    conn_info_p;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;
 
  do
  {
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }
    flags = 0;
  
    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);  
    retval = of_create_delete_flow_entries_msg_of_table(msg,
                                                        dp_handle,
                                                        TSC_APP_CLASSIFY_TABLE_ID_0,
                                                        flags,
                                                        0, /* cookie */
                                                        0, /* cookie_mask */ 
                                                        OFPP_NONE, /* out_port    */
                                                        OFPP_NONE, /* out_group   */
                                                        &conn_info_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error deleting flow entry in table 0 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);

 /* MF1: set match field Input Port id */

    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"flow mod delete  - port id is %d",*input_port_id_p);
  
    retval = of_set_in_port(msg,input_port_id_p);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in port field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    if(pkt_type == TSC_PKT_UNICAST)
    {
 /* MF2: set match field DMAC = 00:00:00:00:00, Mask = 01:00:00:00:00:00 */
      retval = of_set_destination_mac(msg,ucast_mac_addr_chk,TRUE,ucast_mac_mask);
    }
    else /* Broadcast */
    {
 /* MF2A: set match field DMAC = ff:ff:ff:ff:ff:ff Address, Mask = All 1â*/
      retval = of_set_destination_mac(msg,bcast_mac_addr_chk,TRUE,bcast_mac_mask);
    }

    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set Destination mac field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);
    
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 0 = %d",retval);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Del Flow Mod message to Table 0 successfully.")

  }while(0);

  return status;
}
/****************************************************************************************************************************
Function Name:tsc_add_flwmod_1_3_msg_table_0
Input Parameters:
        dp_handle         : Identifies the logical switch that is connected to controller.
        input_port_id_p   : Input port to be usesd as match field to add flows.
        flow_priority     : Priority to be set for the flow to be added.
        pkt_type          : Unicast or Broadcast Packet.
Output Parameters:
        NONE
Description:This function adds one flow entries to classification Table 0. 
****************************************************************************************************************************/
int32_t tsc_add_flwmod_1_3_msg_table_0(uint64_t  dp_handle, 
                                       uint16_t  msg_len,
                                       uint16_t  flow_priority,
                                       uint32_t  *input_port_id_p,
                                       uint8_t   pkt_type,
                                       uint16_t  *vlan_id_p,
                                       uint8_t   vlan_mask_req,
                                       uint16_t  *vlan_id_mask_p,
                                       uint64_t  metadata,
                                       uint64_t  metadata_mask, 
                                       uint8_t   goto_table_id
                                      )

{
  uint16_t  ethernet_type;

  struct of_msg *msg;
  uint8_t* match_start_loc_p = NULL;
  uint8_t* inst_start_loc_p  = NULL;
  uint16_t match_fd_len = 0;
  uint16_t instruction_len;
  void     *conn_info_p;
  uint64_t tunnel_id;
  uint8_t  nwport_type;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  uint16_t eth_type = 0x0800; /*Eth type is IP*/
  uint8_t  protocol_udp;
  uint16_t dnsport,dhcpport;  

  do
  {
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }

    flags = 0;
    flags |= OFPFF_RESET_COUNTS;

    /* zero idle and hard timeout makes entry permanet */
    retval = of_create_add_flow_entry_msg(msg,dp_handle,
                                          TSC_APP_CLASSIFY_TABLE_ID_0, 
                                          flow_priority, 
                                          OFPFC_ADD,
                                          0, /* cookie */
                                          0, /* Cookie mask */
                                          OFP_NO_BUFFER,
                                          flags,
                                          0, /*No Hard timeout*/
                                          0, /*No Idle timeout*/
                                          &conn_info_p
                                          );

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error adding flow entry in table 0 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);
    
 /* MF1: set match field Input Port id */ 

    retval = of_set_in_port(msg,input_port_id_p);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in port field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port_id = %d",*input_port_id_p);
    if((pkt_type == TSC_PKT_UNICAST)         || (pkt_type == TSC_PKT_UNICAST_VMNS_NW)  || 
       (pkt_type == TSC_PKT_UNICAST_ARP)     || (pkt_type == TSC_PKT_UNICAST_DNS_UDP)  ||
       (pkt_type == TSC_PKT_UNICAST_DHCP_UDP)|| (pkt_type == TSC_PKT_UNICAST_DHCP_UDP_67))
    {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSSM");
 /* MF2: set match field DMAC = 00:00:00:00:00, Mask = 01:00:00:00:00:00 */
      retval = of_set_destination_mac(msg,ucast_mac_addr_chk,TRUE,ucast_mac_mask);
    } 
    else /* Broadcast */
    {
 /* MF2A: set match field DMAC = ff:ff:ff:ff:ff:ff Address, Mask = All 1’s */
      retval = of_set_destination_mac(msg,bcast_mac_addr_chk,TRUE,bcast_mac_mask);
    }   

    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set Destination mac field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    } 

/* MF3: set match field - VLAN id */

    if( (pkt_type == TSC_PKT_UNICAST_VMNS_NW) && (*vlan_id_p != 0)) 
    {
      *vlan_id_p = (*vlan_id_p | 0x1000);

      of_set_vlan_id(msg,vlan_id_p,FALSE,0 /* vlan_mask_req,vlan_id_mask_p */);
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set vlan id field failed,err = %d",retval);
        status = OF_FAILURE; 
        break;
      }
    }
    
    if(pkt_type == TSC_PKT_UNICAST_ARP) 
    {
      ethernet_type = 0x806;
      retval = of_set_ether_type(msg,&ethernet_type);
     
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ether type match field ");
        status = OF_FAILURE;
        break;
      }
    }
    if(pkt_type == TSC_PKT_UNICAST_DNS_UDP) 
    {
      retval = of_set_ether_type(msg,&eth_type);
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set IPv4 src field failed,err = %d",retval);
        status = OF_FAILURE;
        break;
      }
      protocol_udp = OF_IPPROTO_UDP;

      retval = of_set_protocol(msg,&protocol_udp);
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ip protocol match field ");
        status = OF_FAILURE;
        break;
      }
      dnsport = 53;
      retval = of_set_udp_destination_port(msg,&dnsport);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set udp destination port match field ");
        status = OF_FAILURE;
        break;
      }
    }
          
    if(pkt_type == TSC_PKT_UNICAST_DHCP_UDP) 
    {
      retval = of_set_ether_type(msg,&eth_type);
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set IPv4 src field failed,err = %d",retval);
        status = OF_FAILURE;
        break;
      }
      protocol_udp = OF_IPPROTO_UDP;

      retval = of_set_protocol(msg,&protocol_udp);
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ip protocol match field ");
        status = OF_FAILURE;
        break;
      }
      dhcpport = 68;
      retval = of_set_udp_destination_port(msg,&dhcpport);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set udp destination port match field ");
        status = OF_FAILURE;
        break;
      }
    }

    if(pkt_type == TSC_PKT_UNICAST_DHCP_UDP_67)
    {
        retval = of_set_ether_type(msg,&eth_type);
        if(retval != OFU_SET_FIELD_SUCCESS)
        {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set IPv4 src field failed,err = %d",retval);
            status = OF_FAILURE;
            break;
        }
        protocol_udp = OF_IPPROTO_UDP;

        retval = of_set_protocol(msg,&protocol_udp);
        if(retval != OFU_SET_FIELD_SUCCESS)
        {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ip protocol match field ");
            status = OF_FAILURE;
            break;
        }
        dhcpport = 67;
        retval = of_set_udp_destination_port(msg,&dhcpport);
        if(retval == OF_FAILURE)
        {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set udp destination port match field ");
            status = OF_FAILURE;
            break;
        }
    }

 /* MF4: set match field - VNI */

    nwport_type  = ((metadata >> PKT_ORIGIN_OFFSET) & 0x0f);

    if((nwport_type == NW_UNICAST_PORT) || (nwport_type == NW_BROADCAST_PORT))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port_id = %d",*input_port_id_p); 
      tunnel_id    = (metadata & 0xffffffff);
      
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Tunnel-ID = %lx",tunnel_id); 
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwport_type = %d",nwport_type);
   
      retval = of_set_logical_port_meta_data(msg,&tunnel_id,FALSE,0);
   
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set tunnel id field failed,err = %d",retval);
        status = OF_FAILURE;
        break;
      }
    }

    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);
    ofu_start_pushing_instructions(msg,match_fd_len);

    retval = ofu_push_write_meta_data_instruction(msg,metadata,metadata_mask);
    if (retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF write meta data instruction failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* Add Go to Instruction */
    retval = ofu_push_go_to_table_instruction(msg,goto_table_id);
    if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"go to table instruction failed err = %d",retval);
      status = OF_FAILURE;
      break;
    }
    ofu_end_pushing_instructions(msg,inst_start_loc_p,&instruction_len);
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 0 = %d",retval);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Sent Flow Mod message to Table 0 successfully.")
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
  }

  return status;
}  

int32_t
tsc_ofplugin_v1_3_table_0_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                            uint64_t  domain_handle,
                                            uint64_t  datapath_handle,
                                            struct    of_msg *msg,
                                            struct    ofl_packet_in *pkt_in,
                                            void*     cbk_arg1,
                                            void*     cbk_arg2)
{
  return OF_SUCCESS;
}
/*************************************TABLE_1**************************************************/
/* Handling miss_entry for Table 1 TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1 */
int32_t
tsc_ofplugin_v1_3_table_1_miss_entry_pkt_rcvd(int64_t controller_handle,
                                            uint64_t  domain_handle,
                                            uint64_t  datapath_handle,
                                            struct    of_msg *msg,
                                            struct    ofl_packet_in *pkt_in,
                                            void*     cbk_arg1,
                                            void*     cbk_arg2)
{
  uint32_t in_port_id;
  uint64_t metadata;

  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  printf("\r\nT1-MP");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Table_1 miss packet received");

  do
  {
    /* Get input_port_id */
    retval = ofu_get_in_port_field(msg,(void *)pkt_in->match_fields,
                                   pkt_in->match_fields_length,
                                   &in_port_id
                                  );

    if(retval != OFU_IN_PORT_FIELD_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get inport ",__FUNCTION__,in_port_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN IN PORT =%d",in_port_id);

    /* Get metadata */
    retval = ofu_get_metadata_field(msg,(void *)pkt_in->match_fields,
                                     pkt_in->match_fields_length,
                                     &metadata
                                    );

    if(retval != OFU_IN_META_DATA_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"failed to get metadata");
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA =%llx",metadata);

    retval = tsc_outbound_ns_chain_table_1_miss_entry_pkt_rcvd(datapath_handle,
                                                                pkt_in,
                                                                in_port_id,
                                                                metadata);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:table 1 processing failed ",__FUNCTION__);
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
       msg->desc.free_cbk(msg);
    return OF_ASYNC_MSG_CONSUMED;
  }

  return status;
}
int32_t tsc_add_flwmod_1_3_msg_table_1(uint64_t dp_handle,
                                       struct   nschain_repository_entry* out_bound_nschain_repository_entry_p
                                      )
{
  struct   of_msg *msg;
  uint16_t msg_len;
  uint8_t* action_start_loc = NULL;
  uint8_t* match_start_loc_p = NULL;
  uint8_t* inst_start_loc_p  = NULL;
  uint16_t match_fd_len = 0;
  uint16_t action_len;
  uint16_t instruction_len;
  void*    conn_info_p;
  uint64_t tunnel_id;
  int32_t  retval;
  uint8_t  ii;
  int32_t  status = OF_SUCCESS;
  uint16_t eth_type = 0x0800; /*Eth type is IP*/
 
  do
  {
    msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
                OFU_IN_PORT_FIELD_LEN+  
                OFU_META_DATA_FIELD_LEN+
                OFU_VLAN_VID_FIELD_LEN+
                OFU_ARP_OP_FIELD_LEN+
                OFU_ETH_TYPE_FIELD_LEN+
                OFU_IPV4_SRC_MASK_LEN+
                OFU_IPV4_DST_MASK_LEN+
                OFU_IP_PROTO_FIELD_LEN+
                OFU_TCP_SRC_FIELD_LEN+
                OFU_TCP_DST_FIELD_LEN+
                OFU_SET_FIELD_WITH_TUNNEL_ID_FIELD_ACTION_LEN+
                OFU_SET_FIELD_WITH_IPV4_DST_FIELD_ACTION_LEN+                
                OFU_APPLY_ACTION_INSTRUCTION_LEN+
                OFU_POP_VLAN_HEADER_ACTION_LEN+
                OFU_PUSH_VLAN_HEADER_ACTION_LEN+
                OFU_OUTPUT_ACTION_LEN+
                OFU_GOTO_TABLE_INSTRUCTION_LEN
              );

    /* Eventhough we allocate memory for tuple, we may use some of them, all of them or none of them */
    /* TCP,UDP or ICMP can be used as their lengths are same */

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," msg_len = %d",msg_len);
    
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      retval = OF_FAILURE;
      break;
    }

    flags = 0;
    flags |= OFPFF_RESET_COUNTS;
    flags |= OFPFF_SEND_FLOW_REM;

    /* zero idle and hard timeout makes entry permanet */
    retval = of_create_add_flow_entry_msg(msg,dp_handle,
                                          TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1,
                                          out_bound_nschain_repository_entry_p->flow_mod_priority, /*priority*/
                                          OFPFC_ADD,
                                          out_bound_nschain_repository_entry_p->safe_reference, /* Cookie */
                                          0xffffffffffffffff, /* Cookie mask */
                                          OFP_NO_BUFFER,
                                          flags,
                                          0,   /* No Hard timeout */
                                          300, /* 300 No Idle timeout */
                                          &conn_info_p
                                         );
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error adding flow entry in table 1 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);

    /* MF1: set match field Input Port id */
    retval = of_set_in_port(msg,&(out_bound_nschain_repository_entry_p->in_port_id));
#if 0    
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in port field failed,err = %d",retval);
      status = OF_FAILURE;
      break;      
    }
#endif
    /* MF2: set match field Metadata */
    retval = of_set_meta_data(msg,&(out_bound_nschain_repository_entry_p->metadata),TRUE,
                                  &(out_bound_nschain_repository_entry_p->metadata_mask));
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in Metadata field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif    

    if(out_bound_nschain_repository_entry_p->match_vlan_id != 0)
    {
      /* MF3: set match field - VLAN id for packet-origin = VM_NS and for nw ports */
      out_bound_nschain_repository_entry_p->match_vlan_id = ((out_bound_nschain_repository_entry_p->match_vlan_id) | 0x1000); 
   
      retval = of_set_vlan_id(msg,&(out_bound_nschain_repository_entry_p->match_vlan_id),FALSE,0);
#if 0
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set vlan id field failed,err = %d",retval);
        status = OF_FAILURE;
        break;  
      }
#endif      
    }

    retval = of_set_ether_type(msg,&eth_type);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)   
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set IPv4 src field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif
    /* Set packet fields as match fields */    
    retval = of_set_ipv4_destination(msg,&out_bound_nschain_repository_entry_p->selector.dst_ip,FALSE,0);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set destination ip match field");
      status = OF_FAILURE;
      break;
    }
#endif    

    retval = of_set_ipv4_source(msg,&out_bound_nschain_repository_entry_p->selector.src_ip,FALSE,0);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set source ip match field");
      status = OF_FAILURE;
      break;
    }
#endif    

    retval = of_set_protocol(msg,&out_bound_nschain_repository_entry_p->selector.protocol);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ip protocol match field ");
      status = OF_FAILURE;
      break;
    }
#endif
    if(out_bound_nschain_repository_entry_p->selector.protocol == OF_IPPROTO_TCP)
    {
      retval = of_set_tcp_source_port(msg,&out_bound_nschain_repository_entry_p->selector.src_port);
#if 0     
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp source port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      
      retval = of_set_tcp_destination_port(msg,&out_bound_nschain_repository_entry_p->selector.dst_port);
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp destination port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      
    }
    else if (out_bound_nschain_repository_entry_p->selector.protocol == OF_IPPROTO_UDP)
    {
      retval = of_set_udp_source_port(msg,&out_bound_nschain_repository_entry_p->selector.src_port);
#if 0      
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp source port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      

      retval = of_set_udp_destination_port(msg,&out_bound_nschain_repository_entry_p->selector.dst_port);
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set udp destination port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      
    }
    else if(out_bound_nschain_repository_entry_p->selector.protocol == OF_IPPROTO_ICMP)
    {
      retval = of_set_icmpv4_type(msg,&(out_bound_nschain_repository_entry_p->selector.icmp_type));
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set icmp type match field");
        status = OF_FAILURE;
        break;
      }
#endif      
      
      retval = of_set_icmpv4_code(msg,&(out_bound_nschain_repository_entry_p->selector.icmp_code));
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set icmp code match field");
        status = OF_FAILURE;
        break;
      }
#endif      
    }
    
    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    ofu_start_pushing_instructions(msg,match_fd_len);
   
    if(out_bound_nschain_repository_entry_p->ns_chain_b == TRUE)  
    {
      retval = ofu_start_pushing_apply_action_instruction(msg);
      if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding apply action instruction err = %d",retval);
        status = OF_FAILURE;
        break;
      }
    
      if(out_bound_nschain_repository_entry_p->copy_local_mac_addresses_b == TRUE)
      {
        for(ii=0;ii<6;ii++)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_mac = %x",out_bound_nschain_repository_entry_p->local_out_mac[ii]); 
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_mac = %x",out_bound_nschain_repository_entry_p->local_in_mac[ii]); 
        }
        retval = ofu_push_set_destination_mac_in_set_field_action(msg, (&out_bound_nschain_repository_entry_p->local_out_mac[0]));
#if 0        
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Error in setting local destination mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        

        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Set local destination mac address");
 
        retval = ofu_push_set_source_mac_in_set_field_action(msg, &out_bound_nschain_repository_entry_p->local_in_mac[0]);
#if 0      
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting local source mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
      
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Set local source mac address");
      }  
    
      if(out_bound_nschain_repository_entry_p->copy_original_mac_addresses_b == TRUE)
      {
        retval = ofu_push_set_destination_mac_in_set_field_action(msg, &out_bound_nschain_repository_entry_p->selector.dst_mac[0]);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting original destination mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Set original destination mac address");

        retval = ofu_push_set_source_mac_in_set_field_action(msg, &out_bound_nschain_repository_entry_p->selector.src_mac[0]);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting original source mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Set original source mac address");    
      }
    }

    if(out_bound_nschain_repository_entry_p->more_nf_b == TRUE) 
    {
      if(out_bound_nschain_repository_entry_p->pkt_origin == VM_NS) 
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Packet origin is VM_NS");
 
        retval = ofu_push_pop_vlan_header_action(msg);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in pop vlan err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
      }
      retval = ofu_push_push_vlan_header_action(msg,0x8100);
#if 0
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in push vlan header err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
      out_bound_nschain_repository_entry_p->next_vlan_id = ((out_bound_nschain_repository_entry_p->next_vlan_id) | 0x1000); 

      retval = ofu_push_set_vlan_id_in_set_field_action(msg,&(out_bound_nschain_repository_entry_p->next_vlan_id));
#if 0
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in push vlan tag err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
    }  
    if(out_bound_nschain_repository_entry_p->nw_port_b == TRUE)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Adding tun dest ip to flow:tun_dest ip = %x",out_bound_nschain_repository_entry_p->remote_switch_ip);
      retval = ofu_push_set_ipv4_tun_dst_addr_in_set_field_action(msg,&(out_bound_nschain_repository_entry_p->remote_switch_ip));
#if 0      
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting Tun Dest IP err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      

      tunnel_id    = (out_bound_nschain_repository_entry_p->metadata & 0xffffffff);
      retval = ofu_push_set_logical_port_meta_data_in_set_field_action(msg,&tunnel_id /* &(out_bound_nschain_repository_entry_p->nid)*/);
#if 0
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting tun id err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
    }
    
    if((out_bound_nschain_repository_entry_p->ns_chain_b == TRUE) && (out_bound_nschain_repository_entry_p->more_nf_b == FALSE ))
    {
      if(out_bound_nschain_repository_entry_p->pkt_origin == VM_NS)
      {
        retval = ofu_push_pop_vlan_header_action(msg);
#if 0     
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in pop vlan err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
      }
    }

    if((out_bound_nschain_repository_entry_p->more_nf_b == TRUE) || (out_bound_nschain_repository_entry_p->ns_port_b == TRUE))
    {    
      ofu_push_output_action(msg,out_bound_nschain_repository_entry_p->out_port_no,OFPCML_NO_BUFFER);
      
#if 0
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding output to port action  err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
    }

    if(out_bound_nschain_repository_entry_p->ns_chain_b == TRUE)
    {
      ofu_end_pushing_actions(msg,action_start_loc,&action_len);
      ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);
    } 

#if 1
    if((out_bound_nschain_repository_entry_p->more_nf_b == FALSE) && (out_bound_nschain_repository_entry_p->ns_port_b == FALSE))
    {
      retval = ofu_push_go_to_table_instruction(msg,out_bound_nschain_repository_entry_p->next_table_id);
#if 0    
      if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"go to table instruction failed err = %d",retval);
        status = OF_FAILURE;
        break;
      } 
#endif      
    }
#endif
 
    ofu_end_pushing_instructions(msg,inst_start_loc_p,&instruction_len);
   
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 1 = %d",retval);
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
  }

  return status;
}
/**********************************************TABLE_2**********************************/
/* Handling miss_entry for Table 2 TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2 */
int32_t
tsc_ofplugin_v1_3_table_2_miss_entry_pkt_rcvd(int64_t   controller_handle,
                                              uint64_t  domain_handle,
                                              uint64_t  datapath_handle,
                                              struct    of_msg *msg,
                                              struct    ofl_packet_in *pkt_in,
                                              void*     cbk_arg1,
                                              void*     cbk_arg2)
{
  uint32_t in_port_id;
  uint64_t metadata;

  int32_t retval;
  int32_t status = OF_SUCCESS;

  printf("\r\nT2-MP");

  do
  {
    /* Get input_port_id */
    retval = ofu_get_in_port_field(msg, pkt_in->match_fields,
                                   pkt_in->match_fields_length,
                                   &in_port_id
                                  );

    if(retval != OFU_IN_PORT_FIELD_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get inport ",__FUNCTION__,in_port_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN IN PORT =%d",in_port_id);

    /* Get metadata */
    retval = ofu_get_metadata_field(msg, pkt_in->match_fields,
                                     pkt_in->match_fields_length,
                                     &metadata
                                    );

    if(retval != OFU_IN_META_DATA_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get metadata ",__FUNCTION__,metadata);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA = %llx",metadata);

    retval = tsc_inbound_ns_chain_table_2_miss_entry_pkt_rcvd(datapath_handle,pkt_in,in_port_id,metadata);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"table 2 processing failed ");
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
        msg->desc.free_cbk(msg);    
    return OF_ASYNC_MSG_CONSUMED;
  }

  return status;
}

int32_t tsc_add_flwmod_1_3_msg_table_2(uint64_t  dp_handle,
                                       struct    nschain_repository_entry* in_bound_nschain_repository_entry_p
                                      )
{
  struct of_msg *msg;
  uint16_t msg_len;

  uint8_t* action_start_loc = NULL;
  uint8_t* match_start_loc_p = NULL;
  uint8_t* inst_start_loc_p  = NULL;
  uint16_t match_fd_len = 0;
  uint16_t action_len;
  uint16_t instruction_len;
  void*    conn_info_p;
  uint64_t tunnel_id;
  int32_t  retval; 
  int32_t  status = OF_SUCCESS;

  uint16_t eth_type = 0x0800; /*Eth type is IP*/

  do
  {
    msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
                OFU_IN_PORT_FIELD_LEN+    
                OFU_META_DATA_FIELD_LEN+
                OFU_VLAN_VID_FIELD_LEN+
                OFU_ETH_TYPE_FIELD_LEN+
                OFU_IPV4_SRC_MASK_LEN+
                OFU_IPV4_DST_MASK_LEN+
                OFU_IP_PROTO_FIELD_LEN+
                OFU_TCP_SRC_FIELD_LEN+
                OFU_TCP_DST_FIELD_LEN+
                OFU_TCP_DST_FIELD_LEN+
                OFU_TCP_DST_FIELD_LEN+
                OFU_TCP_DST_FIELD_LEN+
                OFU_TCP_DST_FIELD_LEN+
                OFU_APPLY_ACTION_INSTRUCTION_LEN+
                OFU_POP_VLAN_HEADER_ACTION_LEN+               
                OFU_PUSH_VLAN_HEADER_ACTION_LEN+
                OFU_OUTPUT_ACTION_LEN+
                OFU_GOTO_TABLE_INSTRUCTION_LEN
              );
   

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow_mod_priority = %d",in_bound_nschain_repository_entry_p->flow_mod_priority);

    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      retval = OF_FAILURE;
      break;
    }

    flags = 0;
    flags |= OFPFF_RESET_COUNTS;

    flags |= OFPFF_SEND_FLOW_REM;
    /* zero idle and hard timeout makes entry permanet */
    retval = of_create_add_flow_entry_msg(msg,dp_handle,
                                          TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2,
                                          in_bound_nschain_repository_entry_p->flow_mod_priority, 
                                          OFPFC_ADD,
                                          in_bound_nschain_repository_entry_p->safe_reference, /* Cookie */
                                          0xffffffffffffffff, /* Cookie mask */
                                          OFP_NO_BUFFER,
                                          flags,
                                          0, /*No Hard timeout*/
                                          300, /*300 No Idle timeout*/
                                          &conn_info_p
                                         );
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error adding flow entry in table 2 = %d",retval);
      status = OF_FAILURE;
      break;
    }
    
    ofu_start_setting_match_field_values(msg);

    /* MF1: set match field Input Port id */
    retval = of_set_in_port(msg,&(in_bound_nschain_repository_entry_p->in_port_id));
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in port field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif    

    /* MF2: set match field Metadata */
    retval = of_set_meta_data(msg,&(in_bound_nschain_repository_entry_p->metadata),TRUE,
                                  &(in_bound_nschain_repository_entry_p->metadata_mask));
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in Metadata field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif    

    if(in_bound_nschain_repository_entry_p->match_vlan_id != 0)
    {
      /* MF3: set match field - VLAN id for packet-origin = VM_NS and nw ports */
      in_bound_nschain_repository_entry_p->match_vlan_id = (in_bound_nschain_repository_entry_p->match_vlan_id) | 0x1000;
 
      retval = of_set_vlan_id(msg,&(in_bound_nschain_repository_entry_p->match_vlan_id),FALSE,0);
#if 0
      if(retval != OFU_SET_FIELD_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set vlan id field failed,err = %d",retval);
        status = OF_FAILURE;
        break; 
      }
#endif      
    }
    
    retval = of_set_ether_type(msg,&eth_type);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set IPv4 src field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif    

    /* Set packet fields as match fields */
    retval = of_set_ipv4_destination(msg,&in_bound_nschain_repository_entry_p->selector.dst_ip,FALSE,0);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set destination ip match field");
      status = OF_FAILURE;
      break;
    }
#endif    

    retval = of_set_ipv4_source(msg,&in_bound_nschain_repository_entry_p->selector.src_ip,FALSE,0);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set source ip match field");
      status = OF_FAILURE;
      break;
    }
#endif    
   
    retval = of_set_protocol(msg,&in_bound_nschain_repository_entry_p->selector.protocol);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set ip protocol match field ");
      status = OF_FAILURE;
      break;
    }
#endif    

    if(in_bound_nschain_repository_entry_p->selector.protocol == OF_IPPROTO_TCP)
    {
      retval = of_set_tcp_source_port(msg,&in_bound_nschain_repository_entry_p->selector.src_port);
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp source port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      
      retval = of_set_tcp_destination_port(msg,&in_bound_nschain_repository_entry_p->selector.dst_port);
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp destination port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      
    }
    else if(in_bound_nschain_repository_entry_p->selector.protocol == OF_IPPROTO_UDP)
    {
      retval = of_set_udp_source_port(msg,&in_bound_nschain_repository_entry_p->selector.src_port);
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set tcp source port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      

      retval = of_set_udp_destination_port(msg,&in_bound_nschain_repository_entry_p->selector.dst_port);
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set udp destination port match field ");
        status = OF_FAILURE;
        break;
      }
#endif      
    }
    else if(in_bound_nschain_repository_entry_p->selector.protocol == OF_IPPROTO_ICMP)
    {
      retval = of_set_icmpv4_type(msg,&(in_bound_nschain_repository_entry_p->selector.icmp_type));
#if 0
      if(retval == OF_FAILURE)
      { 
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set icmp type match field");
        status = OF_FAILURE;
        break;
      }
#endif
      retval = of_set_icmpv4_code(msg,&(in_bound_nschain_repository_entry_p->selector.icmp_code));
#if 0
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to set icmp code match field");
        status = OF_FAILURE;
        break;
      }
#endif      
    } 
    
    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);
 
    ofu_start_pushing_instructions(msg,match_fd_len);
    
    if(in_bound_nschain_repository_entry_p->ns_chain_b == TRUE)
    {
      retval = ofu_start_pushing_apply_action_instruction(msg);
      if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding apply action instruction err = %d",retval);
        status = OF_FAILURE;
        break;
      }

      if(in_bound_nschain_repository_entry_p->copy_local_mac_addresses_b == TRUE)
      {
        retval = ofu_push_set_destination_mac_in_set_field_action(msg, &in_bound_nschain_repository_entry_p->local_out_mac[0]);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Error in setting local destination mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif
        retval = ofu_push_set_source_mac_in_set_field_action(msg, &in_bound_nschain_repository_entry_p->local_in_mac[0]);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting local source mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
      }

      if(in_bound_nschain_repository_entry_p->copy_original_mac_addresses_b == TRUE)
      {
        retval = ofu_push_set_destination_mac_in_set_field_action(msg, &in_bound_nschain_repository_entry_p->selector.dst_mac[0]);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting original destination mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif
        retval = ofu_push_set_source_mac_in_set_field_action(msg, &in_bound_nschain_repository_entry_p->selector.src_mac[0]);
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting original source mac address err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
      }
    }
    
    if(in_bound_nschain_repository_entry_p->more_nf_b == TRUE)
    {
      if(in_bound_nschain_repository_entry_p->nw_port_b == TRUE)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Adding tun dest ip to flow:tun_dest ip = %x",in_bound_nschain_repository_entry_p->remote_switch_ip);
        retval = ofu_push_set_ipv4_tun_dst_addr_in_set_field_action(msg,&(in_bound_nschain_repository_entry_p->remote_switch_ip));
#if 0
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting Tun Dest IP err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        

        tunnel_id = (in_bound_nschain_repository_entry_p->metadata & 0xffffffff); 
        retval = ofu_push_set_logical_port_meta_data_in_set_field_action(msg,&tunnel_id /* (in_bound_nschain_repository_entry_p->nid)*/);
#if 0      
        if(retval != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting tun id err = %d",retval);
          status = OF_FAILURE;
          break;
        }
#endif        
      }

      retval = ofu_push_output_action(msg,in_bound_nschain_repository_entry_p->out_port_no,OFPCML_NO_BUFFER);
#if 0
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding output to port action  err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
    }

    if(in_bound_nschain_repository_entry_p->ns_chain_b == TRUE)
    {
      ofu_end_pushing_actions(msg,action_start_loc,&action_len);
      ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);
    }
    
    if(in_bound_nschain_repository_entry_p->more_nf_b == FALSE)
    {
      /* This part of the code is not normally  reached */
      /* End of NS Chain is reached or No Chain is configured, Go to Instruction Table 3 */
      
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Table 2 No service chaining required.");
      
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Table_id to send the packet = %d",in_bound_nschain_repository_entry_p->next_table_id);

      retval = ofu_push_go_to_table_instruction(msg,in_bound_nschain_repository_entry_p->next_table_id);
#if 0
      if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"go to table instruction failed err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
    } 
    
    ofu_end_pushing_instructions(msg,inst_start_loc_p,&instruction_len);
 
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 2 = %d",retval);
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
  }
  return status;
}
/***************************************TABLE_3**************************************/
/* Handling miss_entry for Table 3 TSC_APP_UNICAST_OUTBOUND_TABLE_ID_3 */
int32_t
tsc_ofplugin_v1_3_table_3_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                             uint64_t  domain_handle,
                                             uint64_t  datapath_handle,
                                             struct    of_msg *msg,
                                             struct    ofl_packet_in *pkt_in,
                                             void*     cbk_arg1,
  					     void*     cbk_arg2)
{
  uint32_t in_port_id;
  uint64_t metadata;
  uint32_t tun_src_ip = 0;  

  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  printf("\r\nT3_MP");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Table_3 Miss packet is received");

  do
  {
    /* Get input_port_id */
    retval = ofu_get_in_port_field(msg, pkt_in->match_fields,
                                   pkt_in->match_fields_length,
                                   &in_port_id
                                  );

    if(retval != OFU_IN_PORT_FIELD_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get inport ",__FUNCTION__,in_port_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN IN PORT =%d",in_port_id);

    /* Get metadata */
    retval = ofu_get_metadata_field(msg, pkt_in->match_fields,
                                     pkt_in->match_fields_length,
                                     &metadata
                                    );

    if(retval != OFU_IN_META_DATA_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get metadata ",__FUNCTION__,metadata);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA =%llx",metadata);

    retval = tsc_unicast_table_3_miss_entry_pkt_rcvd(datapath_handle,pkt_in,metadata,in_port_id,tun_src_ip);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table 3 processing failed ");
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
        msg->desc.free_cbk(msg);
    return OF_ASYNC_MSG_CONSUMED;
  }

  return status;
}

int32_t tsc_add_flwmod_1_3_msg_table_3(uint64_t dp_handle,
                                       struct   ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p
                                      )
{
  struct   of_msg *msg;
  uint16_t msg_len; 
 
  uint8_t* action_start_loc = NULL;
  uint8_t* match_start_loc_p = NULL;
  uint8_t* inst_start_loc_p  = NULL;
  uint16_t match_fd_len = 0;
  uint16_t action_len;
  uint16_t instruction_len;
  void*    conn_info_p;
  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  do
  {
    msg_len  = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
                OFU_META_DATA_MASK_LEN+
                OFU_ETH_TYPE_FIELD_LEN+
                OFU_ETH_DST_MAC_ADDR_FIELD_LEN+
                OFU_APPLY_ACTION_INSTRUCTION_LEN+
                OFU_OUTPUT_ACTION_LEN+
                OFU_SET_FIELD_WITH_TUNNEL_ID_FIELD_ACTION_LEN+
                OFU_SET_FIELD_WITH_IPV4_DST_FIELD_ACTION_LEN
               );

    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      printf("\r\n TABLE_3 OF message alloc error"); 
      break;
    }
 
    memset(msg->desc.start_of_data,0,msg->desc.buffer_len);

    flags = 0;
    flags |= OFPFF_RESET_COUNTS;
    flags |= OFPFF_SEND_FLOW_REM;

    /* zero idle and hard timeout makes entry permanet */
    retval = of_create_add_flow_entry_msg(msg,dp_handle,
                                          TSC_APP_UNICAST_TABLE_ID_3,
                                          ucastpkt_outport_repository_entry_p->priority, /* priority */
                                          OFPFC_ADD,
                                          ucastpkt_outport_repository_entry_p->safe_reference,/*cookie */
                                          0xffffffffffffffff, /* Cookie mask */
                                          OFP_NO_BUFFER,
                                          flags,
                                          0,  /* No Hard timeout */
                                          ucastpkt_outport_repository_entry_p->idle_time, /* 300 No Idle timeout */
                                          &conn_info_p
                                         );
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error adding flow entry in table 3 = %d",retval);
      status = OF_FAILURE;
      printf("\r\n TABLE_3 Error in of_create_add_flow_entry_msg");
      break;
    }

    ofu_start_setting_match_field_values(msg);

    /* MF1: set match field Metadata */
    retval = of_set_meta_data(msg,&(ucastpkt_outport_repository_entry_p->metadata),TRUE,
                                   &(ucastpkt_outport_repository_entry_p->metadata_mask));
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in Metadata field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif

    /* MF2: set match field DMAC with no mask */
    retval = of_set_destination_mac(msg,ucastpkt_outport_repository_entry_p->dst_mac,FALSE,NULL);
#if 0
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set Destination mac field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif    

    /* match_start_loc_p points to the first match field metadata */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    ofu_start_pushing_instructions(msg,match_fd_len);

    retval = ofu_start_pushing_apply_action_instruction(msg);
    if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding apply action instruction err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    if(ucastpkt_outport_repository_entry_p->nw_port_b == TRUE)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Adding tun dest ip to flow:tun_dest ip = %x",ucastpkt_outport_repository_entry_p->tun_dest_ip);
      retval = ofu_push_set_ipv4_tun_dst_addr_in_set_field_action(msg,
                                &(ucastpkt_outport_repository_entry_p->tun_dest_ip));
#if 0
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting Tun Dest IP err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif      
      
      retval = ofu_push_set_logical_port_meta_data_in_set_field_action(msg,
                                       &(ucastpkt_outport_repository_entry_p->tun_id));
#if 0
    if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting tun id err = %d",retval);
        status = OF_FAILURE;
        break;
      }
#endif    
    } 

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Table 3 port no = %d",ucastpkt_outport_repository_entry_p->out_port_no);
    retval = ofu_push_output_action(msg,ucastpkt_outport_repository_entry_p->out_port_no,OFPCML_NO_BUFFER);
#if 0
    if(retval != OFU_ACTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding output to port action  err = %d",retval);
      status = OF_FAILURE;
      break;
    }
#endif    
    
    ofu_end_pushing_actions(msg,action_start_loc,&action_len);
    ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);
    ofu_end_pushing_instructions(msg,inst_start_loc_p,&instruction_len);
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 3 = %d",retval);
      status = OF_FAILURE;
      printf("\r\n TABLE_3 Error Sending Flow Mod message to Table 3");
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
  }

  return status;
}
/**********************************************************************************************/
int32_t tsc_send_pktout_1_3_msg(uint64_t datapath_handle,uint32_t in_port,struct ofl_packet_in *pkt_in)
{
  uint8_t*  action_start_loc=NULL;
  uint16_t  action_len;
  uint16_t  msg_len;
  struct    of_msg *msg;
  uint8_t*  pkt_data = pkt_in->packet;
  void*     conn_info_p;
//  int32_t   flags=0;
//  struct    pkt_mbuf *mbuf = NULL;

  int32_t   retval;
  int32_t   status = OF_SUCCESS;

  msg_len = (OFU_PACKET_OUT_MESSAGE_LEN+16+OFU_OUTPUT_ACTION_LEN+pkt_in->packet_length);

  do
  {
    msg = ofu_alloc_message(OFPT_PACKET_OUT, msg_len);
//    of_alloc_pkt_mbuf_and_set_of_msg(mbuf, msg, OFPT_PACKET_OUT, msg_len, flags, status);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:%d OF message alloc error \r\n",__FUNCTION__,__LINE__);
      status = OF_FAILURE;
      break;
    }

    retval = of_frame_pktout_msg(msg,datapath_handle,TRUE,OFP_NO_BUFFER,
                                 in_port,0,&conn_info_p);
 
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in framing pkt out");
      status = OF_FAILURE;
      break;
    }
    ofu_start_pushing_actions(msg);

    retval = ofu_push_output_action(msg,OFPP_TABLE,OFPCML_NO_BUFFER);
  
    if(retval != OFU_ACTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:%d  Error in adding output action  err = %d\r\n",
                  __FUNCTION__,__LINE__,retval);
      status = OF_FAILURE;
      break;
    }
  
    ofu_end_pushing_actions(msg,action_start_loc,&action_len);
    ((struct ofp_packet_out*)(msg->desc.start_of_data))->actions_len=htons(action_len);

    retval = of_add_data_to_pkt_and_send_to_dp(msg,datapath_handle,conn_info_p,
                                             pkt_in->packet_length,pkt_data,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in sending packet err = %d",retval);
      status = OF_FAILURE;
      break;
    }
  }while(0);
 
  if((status == OF_FAILURE) && (msg != NULL))
     msg->desc.free_cbk(msg);
    
  return status;
}
/***************s***************************Table 4 *******************************************/
int32_t tsc_add_flwmod_1_3_msg_table_4(uint64_t  dp_handle,
                                       struct    vmports_repository_entry* vmports_repository_entry_p,
                                       struct    nwports_brdcast_repository_entry* nwports_brdcast_repository_entry_p
                                      )
{
  struct   of_msg *msg;
  uint16_t msg_len;
  
  uint8_t* action_start_loc = NULL;
  uint8_t* match_start_loc_p = NULL;
  uint8_t* inst_start_loc_p  = NULL;
  uint16_t match_fd_len = 0;
  uint8_t  no_of_ports;
  uint16_t action_len;
  uint16_t instruction_len;
  uint8_t  table_4_entry_priority = 1;
  void     *conn_info_p;
  uint64_t tun_id; 
 
  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  do
  {
    msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
                OFU_META_DATA_MASK_LEN+
                OFU_APPLY_ACTION_INSTRUCTION_LEN+
                OFU_SET_FIELD_WITH_TUNNEL_ID_FIELD_ACTION_LEN+
                ((OFU_OUTPUT_ACTION_LEN) * (vmports_repository_entry_p->no_of_vmside_ports))+
                ((OFU_OUTPUT_ACTION_LEN) * (nwports_brdcast_repository_entry_p->no_of_nwside_br_ports))
              );

    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
      break;
    }

    flags = 0;
    flags |= OFPFF_RESET_COUNTS;

    /* zero idle and hard timeout makes entry permanet */
    retval = of_create_add_flow_entry_msg(msg,dp_handle,
                                          TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4,
                                          table_4_entry_priority, /*priority*/
                                          OFPFC_ADD,
                                          0, /* Cookie */
                                          0, /* Cookie mask */
                                          OFP_NO_BUFFER,
                                          flags,
                                          0, /*No Hard timeout*/
                                          300, /*Idle timeout in seconds*/
                                          &conn_info_p
                                        );
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error adding flow entry in table 4 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);

    /* MF1: set match field Metadata */
    retval = of_set_meta_data(msg,&(nwports_brdcast_repository_entry_p->metadata),TRUE,
                                  &(nwports_brdcast_repository_entry_p->metadata_mask));
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in Metadata field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* match_start_loc_p points to the first match field input_port_id */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    ofu_start_pushing_instructions(msg,match_fd_len);

    retval = ofu_start_pushing_apply_action_instruction(msg);
    if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding apply action instruction err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    tun_id = (nwports_brdcast_repository_entry_p->metadata & 0xffffffff);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Table 4 tun_id =  %llx",tun_id);
    
    retval = ofu_push_set_logical_port_meta_data_in_set_field_action(msg,&tun_id);
    if(retval != OFU_ACTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in setting table 4 tun id err = %d",retval);
      status = OF_FAILURE;
      break;
    }    
 
    for(no_of_ports = 0;(no_of_ports < (vmports_repository_entry_p->no_of_vmside_ports));no_of_ports++)
    {
      retval = ofu_push_output_action(msg,
                 vmports_repository_entry_p->vmside_ports_p[no_of_ports],OFPCML_NO_BUFFER);
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error adding output to local port action err = %d",retval);
        status = OF_FAILURE;
        break; 
      }
    }

    for(no_of_ports = 0;(no_of_ports < (nwports_brdcast_repository_entry_p->no_of_nwside_br_ports));
                                          no_of_ports++)
    {
      retval = ofu_push_output_action(msg,
                nwports_brdcast_repository_entry_p->nwside_br_ports_p[no_of_ports],OFPCML_NO_BUFFER);
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR," Err in adding output to nw br port action err = %d",retval);
        status = OF_FAILURE;
        break;
      }
    }

    ofu_end_pushing_actions(msg,action_start_loc,&action_len);
    ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);
    ofu_end_pushing_instructions(msg,inst_start_loc_p,&instruction_len);
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 4 = %d",retval);
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
  }

  return status;
}
/* Handling miss_entry for Table 4 TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4 */
int32_t
tsc_ofplugin_v1_3_table_4_miss_entry_pkt_rcvd(int64_t   controller_handle,
                                             uint64_t  domain_handle,
                                             uint64_t  datapath_handle,
                                             struct    of_msg *msg,
                                             struct    ofl_packet_in *pkt_in,
                                             void*     cbk_arg1,
                                             void*     cbk_arg2)
{
  uint32_t in_port_id;
  uint64_t metadata;

  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  printf("\r\nT4-MP");

  do
  {
    /* Get input_port_id */
    retval = ofu_get_in_port_field(msg, pkt_in->match_fields,
                                   pkt_in->match_fields_length,
                                   &in_port_id
                                  );

    if(retval != OFU_IN_PORT_FIELD_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get inport ",__FUNCTION__,in_port_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN IN PORT =%d",in_port_id);

    /* Get metadata */
    retval = ofu_get_metadata_field(msg,pkt_in->match_fields,
                                    pkt_in->match_fields_length,
                                    &metadata
                                    );

    if(retval != OFU_IN_META_DATA_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get metadata ",__FUNCTION__);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA2 =%llx",metadata);

    retval = tsc_broadcast_outbound_table_4_miss_entry_pkt_rcvd(datapath_handle,pkt_in,metadata,in_port_id);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:table 4 processing failed ",__FUNCTION__);
      status = OF_FAILURE;
      break;
    }
  }while(0);
  
  if(status == OF_FAILURE)
  {
    if(msg != NULL)
       msg->desc.free_cbk(msg);
    return OF_ASYNC_MSG_CONSUMED;
  }
  
  return status;
}
/******************************************Table_5**********************************************/
int32_t tsc_add_flwmod_1_3_msg_table_5(uint64_t  dp_handle,
                                       struct    vmports_repository_entry* vmports_repository_entry_p
                                      )
{
  struct   of_msg *msg;
  uint16_t msg_len;
  
  uint8_t* action_start_loc = NULL;
  uint8_t* match_start_loc_p = NULL;
  uint8_t* inst_start_loc_p  = NULL;
  uint16_t match_fd_len = 0;
  uint8_t  no_of_ports;
  uint16_t action_len;
  uint16_t instruction_len;
  uint8_t  table_5_entry_priority = 1;
  void*    conn_info_p;

  int32_t  retval;
  int32_t  status = OF_SUCCESS;

  do
  {
    msg_len = ( OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+ 
                OFU_META_DATA_MASK_LEN+ 
                OFU_APPLY_ACTION_INSTRUCTION_LEN+ 
                (OFU_OUTPUT_ACTION_LEN * vmports_repository_entry_p->no_of_vmside_ports)
              );
    
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error - Table 5 ");
      status = OF_FAILURE;
      break;
    }

    flags = 0;
    flags |= OFPFF_RESET_COUNTS;

    /* zero idle and hard timeout makes entry permanet */
    retval = of_create_add_flow_entry_msg(msg,dp_handle,
                                          TSC_APP_BROADCAST_INBOUND_TABLE_ID_5,
                                          table_5_entry_priority, /*priority*/
                                          OFPFC_ADD,
                                          0, /* cookie */
                                          0, /* Cookie mask */
                                          OFP_NO_BUFFER,
                                          flags,
                                          0,  /*No Hard timeout*/
                                          300, /*No Idle timeout*/
                                          &conn_info_p
                                         );
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in creating flow entry - table 5 = %d",retval);
      status = OF_FAILURE;
      break;
    }

    ofu_start_setting_match_field_values(msg);

    /* MF1: set match field Metadata */
    retval = of_set_meta_data(msg,&(vmports_repository_entry_p->metadata),TRUE,
                                  &(vmports_repository_entry_p->metadata_mask));

    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF Set in Metadata field failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    /* match_start_loc_p points to the first match field Metadata */
    ofu_end_setting_match_field_values(msg,match_start_loc_p,&match_fd_len);

    ofu_start_pushing_instructions(msg,match_fd_len);

    retval = ofu_start_pushing_apply_action_instruction(msg);
    if(retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding apply action instruction err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    for(no_of_ports = 0;(no_of_ports < (vmports_repository_entry_p->no_of_vmside_ports)); no_of_ports++)
    {
      retval = ofu_push_output_action(msg,vmports_repository_entry_p->vmside_ports_p[no_of_ports],OFPCML_NO_BUFFER);
      if(retval != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in adding output to port action  err = %d",retval);
        status = OF_FAILURE;
        break;
      }
    }
    ofu_end_pushing_actions(msg,action_start_loc,&action_len);
    ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);

    ofu_end_pushing_instructions(msg,inst_start_loc_p,&instruction_len);
    /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,dp_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Flow Mod message to Table 5 = %d",retval);
      status = OF_FAILURE;
      break;
    }
  }while(0);
  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
  }   
 
  return status;
}
/* Handling miss_entry for Table 5 */
int32_t
tsc_ofplugin_v1_3_table_5_miss_entry_pkt_rcvd(int64_t   controller_handle,
                                              uint64_t  domain_handle,
                                              uint64_t  datapath_handle,
                                              struct    of_msg *msg,
                                              struct    ofl_packet_in *pkt_in,
                                              void*     cbk_arg1,
                                              void*     cbk_arg2)
{
  uint32_t in_port_id;
  uint64_t metadata;  
  uint8_t  nw_type;
  uint32_t tun_src_ip;
  
  int32_t  retval;
  int32_t  status = OF_SUCCESS;
 
  do
  {
    /* Get input_port_id */
    retval = ofu_get_in_port_field(msg, pkt_in->match_fields,
                                   pkt_in->match_fields_length,
                                   &in_port_id
                                  );

    if(retval != OFU_IN_PORT_FIELD_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get inport ",__FUNCTION__,in_port_id);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN IN PORT =%d",in_port_id);

    /* Get metadata */
    retval = ofu_get_metadata_field(msg,
                                   (void *)pkt_in->match_fields,
                                   pkt_in->match_fields_length,&metadata);

    if(retval != OFU_IN_META_DATA_PRESENT)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get metadata ",__FUNCTION__,metadata);
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA =%llx",metadata);

    nw_type = (uint8_t) ((( metadata >> NW_TYPE_OFFSET) & 0x0f));

    if(nw_type == VXLAN_TYPE)
    {
      /* Get Tun_src from pkt in */
      retval = ofu_get_tun_src_field(msg, pkt_in->match_fields,
                                     pkt_in->match_fields_length, &tun_src_ip);
      if(retval != OFU_TUN_SRC_FIELD_PRESENT)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:failed to get tunnel source ip");
        status = OF_FAILURE;
        break;
      }
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Tunnel Source IP = %x",tun_src_ip);
    }

    retval = tsc_broadcast_inbound_table_5_miss_entry_pkt_rcvd(datapath_handle,pkt_in,metadata,in_port_id,tun_src_ip);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"%s:table 5 processing failed ",__FUNCTION__);
      status = OF_FAILURE;
      break;
    }
  }while(0);

  if(status == OF_FAILURE)
  {
    if(msg != NULL)
      msg->desc.free_cbk(msg);
    return OF_ASYNC_MSG_CONSUMED;
  }

  return status;
}
/********************************************************************************************************************************/
int32_t tsc_ofplugin_v1_3_nschain_table_1_flow_entry_removed_rcvd(int64_t   controller_handle,
                                                                  uint64_t  domain_handle,
                                                                  uint64_t  dp_handle,
                                                                  struct    of_msg *msg,
                                                                  struct    ofl_flow_removed *flow,
                                                                  void*     cbk_arg1,
                                                                  void*     cbk_arg2)
{
  uint64_t metadata;
  int32_t  retval;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_1 flow removed event received");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow removed message received for table = %x",flow->table_id); 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow removed message cookie = %x",flow->cookie);

  /* Get metadata */
  retval = ofu_get_metadata_field(msg,flow->match_fields,flow->match_field_len,&metadata);
 
  if(retval != OFU_IN_META_DATA_PRESENT)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"failed to get metadata");
    return OF_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"metadata = %llx",metadata);

  tsc_flow_entry_removal_msg_recvd_nschain_tables(dp_handle,metadata,
                                                  flow->cookie,flow->reason,flow->table_id); 
  return OF_SUCCESS;
}

int32_t tsc_ofplugin_v1_3_nschain_table_2_flow_entry_removed_rcvd(int64_t   controller_handle,
                                                                  uint64_t  domain_handle,
                                                                  uint64_t  dp_handle,
                                                                  struct    of_msg *msg,
                                                                  struct    ofl_flow_removed *flow,
                                                                  void*     cbk_arg1,
                                                                  void*     cbk_arg2)
{
  uint64_t metadata;
  int32_t  retval; 
 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_2 flow removed event received");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow removed message received for table = %x",flow->table_id);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow removed message cookie = %x",flow->cookie);

  /* Get metadata */
  retval = ofu_get_metadata_field(msg,flow->match_fields,flow->match_field_len,&metadata);

  if(retval != OFU_IN_META_DATA_PRESENT)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"failed to get metadata");
    return OF_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"metadata = %llx",metadata);

  tsc_flow_entry_removal_msg_recvd_nschain_tables(dp_handle,metadata,
                                                  flow->cookie,flow->reason,flow->table_id);
  return OF_SUCCESS;
}

int32_t tsc_ofplugin_v1_3_fc_table_flow_entry_removed_rcvd(int64_t   controller_handle,
                                                           uint64_t  domain_handle,
                                                           uint64_t  dp_handle,
                                                           struct    of_msg *msg,
                                                           struct    ofl_flow_removed *flow,
                                                           void*     cbk_arg1,
                                                           void*     cbk_arg2)
{
  int32_t  retval;
  uint64_t metadata;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow removed event received forfor table_3 ");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow removed message received for table = %x",flow->table_id);

  /* Get metadata */
  retval = ofu_get_metadata_field(msg,flow->match_fields,flow->match_field_len,&metadata);

  if(retval != OFU_IN_META_DATA_PRESENT)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"failed to get metadata");
    return OF_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"metadata = %llx",metadata);
      
  tsc_flow_entry_removal_msg_recvd_table_3(dp_handle,metadata,
                                           flow->cookie,flow->reason);
      
  return OF_SUCCESS;
} 
int32_t
tsc_ofplugin_v1_3_dp_detach_event(uint64_t  controller_handle,
                      uint64_t  domain_handle,
                      uint64_t  datapath_handle,
                      void      *cbk_arg1,
                      void      *cbk_arg2)
{
  uint8_t table_id;
  struct dprm_datapath_entry *pdatapath_info;
  int32_t status = OF_SUCCESS;

  do
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"DATA PATH DETACH EVENT RECEIVED");
    if(get_datapath_byhandle(datapath_handle, &pdatapath_info) != DPRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_WARN, "No DP Entry with given datapath_handle=%llu",datapath_handle);
      status = OF_FAILURE;
      break;
  }
    /* Add miss entries to the OF tables 1,2,3,4,5 */
    for(table_id = 1; table_id < 6; table_id++)
    {
      if(tsc_ofplugin_v1_3_del_table_miss_entry(datapath_handle,table_id) != OF_SUCCESS)
      {
        status = OF_FAILURE;
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Adding miss entries failed for table %d ",table_id);
        break;
}
    }
  }while(0);
  return status;
}



int32_t tsc_ofplugin_v1_3_del_table_miss_entry(uint64_t datapath_handle,uint8_t table_id)
{
  struct   of_msg *msg;
  uint16_t msg_len,flags; 
  uint16_t match_fd_len=0;
  uint8_t* match_start_loc = NULL;
  int32_t  retval = OF_SUCCESS;
  int32_t  status = OF_SUCCESS;
  void*    conn_info_p;  
                                                
  msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16);
 
  do                  
  {                   
    msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
    if(msg == NULL)  
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"OF message alloc error ");
      status = OF_FAILURE;
       break;
    }
    flags = 0;
    flags |= OFPFF_SEND_FLOW_REM;
    retval = of_create_delete_flow_entries_msg_of_table(msg,datapath_handle,table_id, 
                                          flags, /* flags */
                                          0, /*cookie*/
                                          0, /*cookie mask*/
                                          OFPP_NONE, /* outport */
                                          OFPP_NONE, /* outgroup */
                                          &conn_info_p
                                          );

   if(retval != OFU_DELETE_FLOW_ENTRY_TO_TABLE_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error in deleting miss entry arp table = %d",retval);
     status = OF_FAILURE;
     break;
   }
   ofu_start_setting_match_field_values(msg);
   ofu_end_setting_match_field_values(msg,match_start_loc,&match_fd_len);

   /* Send the message to DP */
    retval = of_cntrlr_send_request_message(msg,datapath_handle,conn_info_p,NULL,NULL);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Error Sending Del Flow Mod message to miss Table = %d",retval);
      status = OF_FAILURE;   
      break;
    } 

   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Miss entries deleted");
 }while(0);
   return status;
}

/*****************************************************************************************************************************/
