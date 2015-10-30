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

/* File: tsc_controller.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains OF Flow Manager functionality.
 */

/* INCLUDE_START *************************************************************************************/
#include "controller.h"
#include "of_utilities.h"
#include "of_multipart.h"
#include "of_msgapi.h"
#include "cntrl_queue.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "dprmldef.h"
#include "dprm.h"

#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_controller.h"
#include "tsc_ofv1_3_plugin.h"
#include "tsc_nvm_modules.h"
#include "tsc_nvm_vxlan.h"
#include "tsc_nsc_iface.h"
#include "tsc_vxn_views.h"
/* INCLUDE_END *****************************************************************************************/

extern   uint32_t  vn_nsc_info_offset_g;
extern   uint32_t  nsc_no_of_repository_table_1_buckets_g;
extern   uint32_t  nsc_no_of_repository_table_2_buckets_g;
extern   uint32_t  nsc_no_of_repository_table_3_buckets_g;

struct tsc_nvm_module_interface tsc_nvm_modules[SUPPORTED_NW_TYPE_MAX + 1];

int32_t tsc_add_unicast_nwport_flows_to_classify_table(uint64_t vn_handle, 
                                                       uint8_t  nw_type,
                                                       char*    swname_p,
                                                       uint16_t vlan_id_in,
                                                       uint16_t vlan_id_out,
                                                       uint64_t dp_handle);

int32_t tsc_add_broadcast_nwport_flows_to_classify_table(uint64_t vn_handle, 
                                                         uint8_t  nw_type,
                                                         char*    swname_p,
                                                         uint16_t vlan_id_in,  
                                                         uint16_t vlan_id_out,  
                                                         uint64_t dp_handle);

void tsc_barrier_reply_notification_fn(uint64_t  controller_handle,
                                                 uint64_t  domain_handle,
                                                 uint64_t  datapath_handle,
                                                 struct of_msg *msg,
                                                 void     *cbk_arg1,
                                                 void     *cbk_arg2,
                                                 uint8_t   status);


struct table_barrier_msg
{
  uint64_t dp_handle;
  int32_t  in_port;
  struct   of_msg* msg;
  uint8_t* packet; 
  uint16_t packet_length;
};
/*******************************************************************************************************
Function Name    :tsc_delete_vmport_flows_from_classify_table
Input  Parameters:
	crm_port_type:VM or VM_NS.
	inport_id    :ID of the port deleted.
        dp_handle    :datapath handle of the Logical switch
Output Parameters:NONE
Description:Deletes proactive flows added to classify table 0 received on the port deleted.
*******************************************************************************************************/
int32_t tsc_delete_vmport_flows_from_classify_table(uint8_t  crm_port_type,
                                                    uint32_t inport_id,
                                                    uint64_t dp_handle)
{
  int32_t  retval;
  struct tsc_ofver_plugin_iface* tsc_params_vm_p;

  tsc_params_vm_p = (struct tsc_ofver_plugin_iface *) (malloc(sizeof (struct tsc_ofver_plugin_iface )));
  if(tsc_params_vm_p == NULL)
    return OF_FAILURE;

  tsc_params_vm_p->in_port_id = inport_id;

  /* crm_port_type: Packet Origin which can be one of VM_APPLN_PORT,VM_NS_PORT */
  tsc_params_vm_p->pkt_origin_e = crm_port_type;

  /* Delete Flows for packets for the VM and VM_NS crm port */
  retval = tsc_ofplugin_v1_3_send_flow_mod_classify_table_vmports(dp_handle,
                                                                  tsc_params_vm_p,
                                                                  OFPFC_DELETE);
  return retval;
}
/****************************************************************************************************
Function Name    :tsc_add_vmport_flows_to_classify_table
Input  Parameters:
        vn_handle    :Handle to the Virtual Network.
        swname_p     :Name of the Compute Node.  
        crm_port_type:VM or VM_NS.
        inport_id    :ID of the port added.
        dp_handle    :datapath handle of the Logical switch
Output Parameters:NONE
Description: Add Flows proactively to classify table for VM and VM_NS ports.
             GPSYS: crm_port_type is now checked with the new VM_NS port types.  
****************************************************************************************************/
int32_t tsc_add_vmport_flows_to_classify_table(uint64_t vn_handle,
                                               char*    swname_p,
                                               uint8_t  crm_port_type,
                                               uint32_t inport_id,            
                                               uint16_t vlan_id_in,
                                               uint16_t vlan_id_out,
                                               uint64_t dp_handle)
{
  int32_t  retval;
  struct   crm_virtual_network* vn_entry_p;
  struct   tsc_ofver_plugin_iface* tsc_params_vm_p;
  uint8_t  nw_type;

  /* Get metadata from NVM module */
  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"tsc_add_flows_classify_table - Unable to fetch vn_entry");
    return OF_FAILURE;
  }  
  nw_type = vn_entry_p->nw_type;

  tsc_params_vm_p = (struct tsc_ofver_plugin_iface *) (malloc(sizeof (struct tsc_ofver_plugin_iface )));
  if(tsc_params_vm_p == NULL)
    return OF_FAILURE;

  tsc_params_vm_p->in_port_id = inport_id;
  strcpy(tsc_params_vm_p->local_switch_name,swname_p);

  /* crm_port_type: Packet Origin which can be one of VM_APPLN_PORT,VM_NS_PORT */
  tsc_params_vm_p->pkt_origin_e = crm_port_type;

  tsc_nvm_modules[nw_type].nvm_module_construct_metadata(tsc_params_vm_p,vn_handle,NULL);  
  
  tsc_params_vm_p->metadata = (tsc_params_vm_p->metadata |
                                (((uint64_t)(crm_port_type)) << PKT_ORIGIN_OFFSET)); /* VM */  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA1 =%llx",tsc_params_vm_p->metadata); 
  tsc_params_vm_p->metadata_mask = 0xFFFFFFFFFFFFFFFF;    

  if((crm_port_type == VM_NS) || (crm_port_type == VMNS_IN_PORT) || (crm_port_type == VMNS_OUT_PORT))
  {
    tsc_params_vm_p->vlan_id_in  = vlan_id_in;
    tsc_params_vm_p->vlan_id_out = vlan_id_out;
  }
  else
  {
    tsc_params_vm_p->vlan_id_in  = 0;
    tsc_params_vm_p->vlan_id_out = 0;
  }

  /* Add Flows for both unicast and broadcast packets for the VM and VM_NS crm port */
  retval = tsc_ofplugin_v1_3_send_flow_mod_classify_table_vmports(dp_handle,
                                                                  tsc_params_vm_p,
                                                                  OFPFC_ADD);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, " Failed to add flows for vm port");
    return retval;
  }

  /* Add one flow here for unicast vxlan port for each combination of VNI and SP of VM and VM-NS Port */
  /* Add flow for NW broadcast port also here if possible as we want VNI to be a flow parameter. TBD */

  retval = tsc_add_unicast_nwport_flows_to_classify_table(vn_handle, 
                                                          nw_type, 
                                                          swname_p,
                                                          vlan_id_in,  
                                                          vlan_id_out,
                                                          dp_handle);

  
  /* We may need to enable this code as we want VNI to be a flow parameter even for Broadcast ports */
  retval = tsc_add_broadcast_nwport_flows_to_classify_table(vn_handle, 
                                                            nw_type,
                                                            swname_p,
                                                            vlan_id_in,  
                                                            vlan_id_out, 
                                                            dp_handle);
  
  return retval;
}
/*******************************************************************************************************
Function Name    :tsc_add_unicast_nwport_flows_to_classify_table
Input  Parameters:
        vn_handle    :Handle to the Virtual Network.
        nw_type_p    :VXLAN or NV_GRE
        swname_p     :Name of the Compute Node.
        dp_handle    :datapath handle of the Logical switch
Output Parameters:NONE
Description: Add Flows proactively to classify table 0 for Unicast Network Ports.
*******************************************************************************************************/
int32_t tsc_add_unicast_nwport_flows_to_classify_table(uint64_t vn_handle, 
                                                       uint8_t  nw_type,
                                                       char*    swname_p,
                                                       uint16_t vlan_id_in,
                                                       uint16_t vlan_id_out, 
                                                       uint64_t dp_handle)
{
  struct     tsc_ofver_plugin_iface* tsc_params_p;
  int32_t    retval = OF_SUCCESS; 

  uint8_t crm_port_type = NW_UNICAST_PORT;
  {
    /* Get the VxLan unicast port in the switch for the service port for adding flows. */
    /* Vxlan Unicast Port id in (tsc_params_p->out_port_no) */

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"switch name = %s",swname_p);

    tsc_params_p = (struct tsc_ofver_plugin_iface *) malloc(sizeof (struct tsc_ofver_plugin_iface));

    tsc_nvm_modules[nw_type].nvm_module_construct_metadata(tsc_params_p,vn_handle,NULL);
    tsc_params_p->metadata = (tsc_params_p->metadata | (((uint64_t)(crm_port_type)) << PKT_ORIGIN_OFFSET));

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Calling nvm_module_get_unicast_nwport");
    CNTLR_RCU_READ_LOCK_TAKE();
    retval = tsc_nvm_modules[nw_type].nvm_module_get_unicast_nwport(swname_p,
                                                                    tsc_params_p->serviceport,
                                                                    &(tsc_params_p->in_port_id)
                                                                    );
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"VxLan Unicast port not found");
      CNTLR_RCU_READ_LOCK_RELEASE();
      return OF_FAILURE;
    }
    CNTLR_RCU_READ_LOCK_RELEASE();

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"VxLan Unicast port found");
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"unicast port id =  %d",tsc_params_p->in_port_id);

    /* Add Network Unicast ports for a given combination of vni and service port */
    strcpy(tsc_params_p->local_switch_name,swname_p);
    tsc_params_p->pkt_origin_e  = NW_UNICAST_PORT;
    tsc_params_p->dp_handle     = dp_handle;
    tsc_params_p->metadata_mask = 0xffffffffffffffff;
    tsc_params_p->vlan_id_in    = vlan_id_in;
    tsc_params_p->vlan_id_out   = vlan_id_out;

    retval = tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports(dp_handle,tsc_params_p,OFPFC_ADD);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add flow for unicast port with port id =  %d",tsc_params_p->in_port_id);
    }
    return retval;
  }
}
/*******************************************************************************************************
Function Name    :tsc_add_broadcast_nwport_flows_to_classify_table
Input  Parameters:
        vn_handle    :Handle to the Virtual Network.
        nw_type_p    :VXLAN or NV_GRE
        swname_p     :Name of the Compute Node.
        dp_handle    :datapath handle of the Logical switch
Output Parameters:NONE
Description: Add Flows proactively to classify table 0 for Broadcast Network Ports.
*******************************************************************************************************/
int32_t tsc_add_broadcast_nwport_flows_to_classify_table(uint64_t vn_handle, 
                                                         uint8_t  nw_type,
                                                         char*    swname_p,
                                                         uint16_t vlan_id_in,
                                                         uint16_t vlan_id_out,
                                                         uint64_t dp_handle)
{
  struct     tsc_ofver_plugin_iface* tsc_params_p;
  uint32_t*  nwbr_ports_p;
  uint32_t   ii,no_of_nwbr_ports;
  int32_t    retval = OF_SUCCESS;

  uint8_t crm_port_type = NW_BROADCAST_PORT;
  
  tsc_params_p = (struct tsc_ofver_plugin_iface *) malloc(sizeof (struct tsc_ofver_plugin_iface));
  strcpy(tsc_params_p->local_switch_name,swname_p);
  tsc_nvm_modules[nw_type].nvm_module_construct_metadata(tsc_params_p,vn_handle,NULL);
  tsc_params_p->metadata = (tsc_params_p->metadata |
                               (((uint64_t)(crm_port_type)) << PKT_ORIGIN_OFFSET));
  tsc_params_p->pkt_origin_e  = NW_BROADCAST_PORT;
  tsc_params_p->dp_handle     = dp_handle;
  tsc_params_p->metadata_mask = 0xffffffffffffffff;

  no_of_nwbr_ports = 50; /* No of ports available may be less */
  nwbr_ports_p     = (uint32_t *)(malloc((sizeof (uint32_t) * no_of_nwbr_ports)));

  if(nwbr_ports_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to allocate memory for nwbr_ports_p");
    return OF_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Calling nvm_module_get_broadcast_nwports");
  CNTLR_RCU_READ_LOCK_TAKE();
  retval = tsc_nvm_modules[nw_type].nvm_module_get_broadcast_nwports(swname_p,
                                                                     tsc_params_p->serviceport,
                                                                     0,  /* nss vxlan_nw_p->nid */
                                                                     &no_of_nwbr_ports,
                                                                     nwbr_ports_p,
                                                                     0  /* no specific remote ip */ 
                                                                    );
  
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"VxLan Broadcast ports not found");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"VxLan Broadcast port found number of ports = %d",no_of_nwbr_ports);

  for(ii=0;ii< no_of_nwbr_ports;ii++)
  {
    tsc_params_p->in_port_id = *nwbr_ports_p;
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Broadcast port id =  %d",tsc_params_p->in_port_id);
    nwbr_ports_p++;
    tsc_params_p->vlan_id_in  = vlan_id_in;
    tsc_params_p->vlan_id_out = vlan_id_out;
    retval = tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports(dp_handle,tsc_params_p,OFPFC_ADD);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add flow for broadcast port with port id =  %d",tsc_params_p->in_port_id);
      break;
    }
  }
  return retval;
}
/*******************************************************************************************************
Function Name    :tsc_delete_nwport_flows_from_classify_table
Input  Parameters:
        crm_port_type:Unicast or Broadcast Network Type
        port_id      :ID of the Port deleted
        dp_handle    :datapath handle of the Logical switch
Output Parameters:NONE
Description: Delete Flows proactively from classify table 0 for Network Ports.
*******************************************************************************************************/
/* Delete flows for the nw port deleted. */
int32_t tsc_delete_nwport_flows_from_classify_table(uint8_t  crm_port_type,
                                                    uint32_t port_id,
                                                    uint64_t dp_handle)
{
  struct   tsc_ofver_plugin_iface* tsc_params_p; 
  int32_t  retval;

  tsc_params_p = (struct tsc_ofver_plugin_iface *) malloc(sizeof (struct tsc_ofver_plugin_iface));

  /* crm_port_type: nw port  */
  tsc_params_p->pkt_origin_e = crm_port_type;
  tsc_params_p->dp_handle    = dp_handle;
  tsc_params_p->in_port_id   = port_id; /* Deletion happens only based on port_id */

  /* TBD check whether it deletes multiple flow entries with different VNI */
  retval = tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports(dp_handle,tsc_params_p,OFPFC_DELETE);
  return retval;
}
/*******************************************************************************************************
Function Name: tsc_outbound_ns_chain_table_1_miss_entry_pkt_rcvd
Input  Parameters:
	dp_handle  :datapath handle of the Logical switch 
        pkt_in     :miss Packet received
        in_port_id :ID of the Port on which the packet is received.
        metadata   :metadata field in the pipeline   
Output Parameters: NONE
Description:Miss Packet received from OF Table 1 of a logical switch whose handle is received.
            Table_1 and Table_2 are used for service chaining purpose.   
*******************************************************************************************************/
int32_t tsc_outbound_ns_chain_table_1_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                          struct   ofl_packet_in* pkt_in,
                                                          struct   of_msg* msg,
                                                          uint32_t in_port_id,
                                                          uint64_t metadata
                                                         )
{
  struct    nschain_repository_entry  nschain_repository_entry;
  struct    nschain_repository_entry* out_bound_nschain_repository_entry_p;
  struct    table_barrier_msg*          table_1_barrier = NULL;         

  char*     local_switch_name_p;
  uint8_t   nw_type;
  uint8_t   pkt_origin,nsc_copy_mac_addresses_b;
  uint32_t  nid;
  uint32_t  retval = OF_FAILURE;
  uint8_t   selector_type,table_id,add_status = OF_FAILURE;
 
  uint32_t  hashkey,hashmask;
  struct    nsc_selector_node  selector;
  struct    nsc_selector_node* selector_p;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Miss Packet received by Table_1"); 

  out_bound_nschain_repository_entry_p = &nschain_repository_entry;

  /* Get Local Switch Name */
  CNTLR_RCU_READ_LOCK_TAKE();
  retval = get_swname_by_dphandle(dp_handle,&(local_switch_name_p));
  
  if(retval != DPRM_SUCCESS)
  {
    retval = OF_FAILURE;
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();

  pkt_origin = (uint8_t)((metadata & PKT_ORIGIN_MASK) >> PKT_ORIGIN_OFFSET); 
  nw_type    = (uint8_t)(metadata >> NW_TYPE_OFFSET);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %d",pkt_origin);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_type    = %d",nw_type);
 
  nid = (uint32_t)(metadata & TUN_ID_MASK);

  selector_p = &selector;

  nsc_copy_mac_addresses_b = TRUE;
  if(pkt_origin == VM_NS)
    nsc_copy_mac_addresses_b = FALSE;

  retval = nsc_extract_packet_fields(pkt_in,selector_p,PACKET_NORMAL,nsc_copy_mac_addresses_b);
  if(retval != OF_SUCCESS)
    return retval;

  hashmask = nsc_no_of_repository_table_1_buckets_g;
  hashkey  = nsc_compute_hash_repository_table_1_2(selector_p->src_ip,selector_p->dst_ip,
                                                   selector_p->src_port,selector_p->dst_port,
                                                   selector_p->protocol,
                                                   selector_p->ethernet_type,
                                                   in_port_id,
                                                   dp_handle,
                                                   selector_p->vlan_id_pkt,
                                                   hashmask);
  
  add_status = tsc_get_nschain_repository_entry(local_switch_name_p,
                                                dp_handle,
                                                nw_type,
                                                nid,
                                                in_port_id,
                                                metadata,
                                                pkt_in,
                                                pkt_origin,
                                                TSC_PKT_OUTBOUND,
                                                selector_p,
                                                hashkey,
                                                &out_bound_nschain_repository_entry_p
                                              );

  if(add_status != OF_FAILURE)
  {
    retval = tsc_add_flwmod_1_3_msg_table_1(dp_handle,out_bound_nschain_repository_entry_p);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to send Flow Mod message to Table 1");
      if(add_status == NSC_ADDED_REPOSITORY_ENTRY)   
      {
        /* If a flow is extracted from cn_bind_entry, successfully added it to repository but failed to send it to switch,delete it from repository */
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Deleting the nschain repository entry added just as failed to send the flow entry to switch");
        table_id = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_delete_nschain_repository_entry(out_bound_nschain_repository_entry_p,nw_type,nid,table_id); /* NSME */
      }
      return OF_FAILURE;
    }
    if(selector_p->nsc_entry_present == 0)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Sending Packet out message for Table 1");
      table_1_barrier = (struct table_barrier_msg*)calloc(1,sizeof(struct table_barrier_msg));
      table_1_barrier->dp_handle     = dp_handle;
      table_1_barrier->in_port       = in_port_id;
      table_1_barrier->msg           = msg;
      table_1_barrier->packet        = pkt_in->packet;
      table_1_barrier->packet_length = pkt_in->packet_length;

      retval = of_send_barrier_request_msg(dp_handle,
                                         tsc_barrier_reply_notification_fn,
                                         table_1_barrier,NULL);

      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"barrier request for table 1 failed");
        retval = tsc_send_pktout_1_3_msg(dp_handle,in_port_id,pkt_in->packet,pkt_in->packet_length);
        if(retval == OF_FAILURE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Packet out message for Table 1");
        } 
        return OF_FAILURE; 
      }
   }
   else
   {
     selector_p->nsc_entry_present = 0;
     if(msg != NULL)
       msg->desc.free_cbk(msg);  /* Normally it is released in barrier response callback */
   }
   /* proactive flow pushing */
#if 1
   if(out_bound_nschain_repository_entry_p->pkt_origin == VM_APPLN) 
   {
     selector_type = out_bound_nschain_repository_entry_p->selector.selector_type;
     
     printf("\r\n selector type = %d",selector_type);
     printf(" zone = %d", out_bound_nschain_repository_entry_p->zone);

     if(out_bound_nschain_repository_entry_p->zone == ZONE_LESS)
     {
       retval = tsc_send_all_flows_to_all_tsas(out_bound_nschain_repository_entry_p);
     }
     else if (out_bound_nschain_repository_entry_p->zone == ZONE_LEFT)
     {
       if(selector_type == SELECTOR_PRIMARY) 
       {
         retval = tsc_send_all_flows_to_all_tsas_zone_left(out_bound_nschain_repository_entry_p);
       }  
       else /* SELECTOR_SECONDARY */
       {
         retval = tsc_send_all_flows_to_all_tsas_zone_right(out_bound_nschain_repository_entry_p);   
       }  
     }
     else /* ZONE_RIGHT */
     {
       if(selector_type == SELECTOR_PRIMARY)
       {
         retval = tsc_send_all_flows_to_all_tsas_zone_right(out_bound_nschain_repository_entry_p);   
       }  
       else
       {
         retval = tsc_send_all_flows_to_all_tsas_zone_left(out_bound_nschain_repository_entry_p); 
       }  
     }

     OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Calling Proactive flow pushing to all switches routine.");

     if(retval == OF_SUCCESS)
     { 
       OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Proactive flow pushing is successful.");
     }
     else
     {
       OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Proactive flow pushing failed.");   
     }    
   }
#endif
  }
 else
 {
   return OF_FAILURE;
 }
 return OF_SUCCESS;
}
/*******************************************************************************************************
Function Name: tsc_inbound_ns_chain_table_2_miss_entry_pkt_rcvd
Input  Parameters:
        dp_handle  :datapath handle of the Logical switch
        pkt_in     :miss Packet received
        in_port_id :ID of the Port on which the packet is received.
        metadata   :metadata field in the pipeline
Output Parameters: NONE
Description:Miss Packet received from OF Table 2 of a logical switch whose handle is received.
            Table_1 and Table_2 are used for service chaining purpose.
*******************************************************************************************************/
int32_t tsc_inbound_ns_chain_table_2_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                         struct   ofl_packet_in* pkt_in,
                                                         struct   of_msg* msg,
                                                         uint32_t in_port_id,
                                                         uint64_t metadata
                                                        )
{
  struct    nschain_repository_entry nschain_repository_entry;
  struct    nschain_repository_entry* in_bound_nschain_repository_entry_p;   
  struct    table_barrier_msg*        table_2_barrier = NULL;

  char*     local_switch_name_p;
  uint8_t   nw_type;
  uint8_t   pkt_origin;
  uint32_t  hashkey,hashmask,nid;
  uint16_t  serviceport;
  uint32_t  retval = OF_FAILURE;

  struct    nsc_selector_node  selector;
  struct    nsc_selector_node* selector_p;
  uint8_t   table_id,add_status = OF_FAILURE; 

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Miss Packet received by Table_2");

  in_bound_nschain_repository_entry_p = &nschain_repository_entry;

  /* Get Local Switch Name */
  CNTLR_RCU_READ_LOCK_TAKE();
  retval = get_swname_by_dphandle(dp_handle,&(local_switch_name_p));

  if(retval != DPRM_SUCCESS)
  {
    retval = OF_FAILURE;
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();

  pkt_origin = (uint8_t)((metadata & PKT_ORIGIN_MASK) >> PKT_ORIGIN_OFFSET);
  nw_type = (uint8_t)(metadata >> NW_TYPE_OFFSET);

  pkt_origin = (uint8_t)((metadata & PKT_ORIGIN_MASK) >> PKT_ORIGIN_OFFSET);
  nw_type    = (uint8_t)(metadata >> NW_TYPE_OFFSET);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %d",pkt_origin);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_type    = %d",nw_type);

  nid = (uint32_t)(metadata & TUN_ID_MASK);
  serviceport  = (uint16_t)(metadata >> SERVICE_PORT_OFFSET);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nid = %d",nid);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"serviceport = %d",serviceport);

  selector_p = &selector;

  retval = nsc_extract_packet_fields(pkt_in,selector_p,PACKET_NORMAL,TRUE);
  if(retval != OF_SUCCESS)
    return retval;
 
  hashmask = nsc_no_of_repository_table_2_buckets_g;
  hashkey  = nsc_compute_hash_repository_table_1_2(selector_p->src_ip,selector_p->dst_ip,
                                                   selector_p->src_port,selector_p->dst_port,
                                                   selector_p->protocol,
                                                   selector_p->ethernet_type,
                                                   in_port_id,
                                                   dp_handle,
                                                   selector_p->vlan_id_pkt,
                                                   hashmask);

  add_status = tsc_get_nschain_repository_entry(local_switch_name_p,
                                                dp_handle,
                                                nw_type,
                                                nid,
                                                in_port_id,
                                                metadata,
                                                pkt_in,
                                                pkt_origin,
                                                TSC_PKT_INBOUND,
                                                selector_p,
                                                hashkey,
                                                &in_bound_nschain_repository_entry_p
                                              );
 
  if(add_status != OF_FAILURE)
  {
    retval = tsc_add_flwmod_1_3_msg_table_2(dp_handle,in_bound_nschain_repository_entry_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to send Flow Mod message to Table 2");
      if(add_status == NSC_ADDED_REPOSITORY_ENTRY)
      {
        /* Delete repository entry. If not deleted,it will stay forever unless we use timeouts. */
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Deleting the nschain repository entry added just as failed to send the flow entry to switch");
        table_id = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
        tsc_delete_nschain_repository_entry(in_bound_nschain_repository_entry_p,nw_type,nid,table_id); /* NSME */

      }
      /* Drop the packet. */
    }
    else
    {
      table_2_barrier = (struct    table_barrier_msg*)calloc(1,sizeof(struct    table_barrier_msg));
      table_2_barrier->dp_handle     = dp_handle;
      table_2_barrier->in_port       = in_port_id;
      table_2_barrier->msg           = msg;
      table_2_barrier->packet        = pkt_in->packet;
      table_2_barrier->packet_length = pkt_in->packet_length;

      retval = of_send_barrier_request_msg(dp_handle,
                                         tsc_barrier_reply_notification_fn,
                                         table_2_barrier,NULL);

      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"barrier request for table 2 failed");
        retval = tsc_send_pktout_1_3_msg(dp_handle,in_port_id,pkt_in->packet,pkt_in->packet_length);
        if(retval == OF_FAILURE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to send Packet out message for Table 2");
        }      
      }
    }
  } 
  else
  {
    /* Drop the packet. */
  }
  return retval;
}
/*****************************************************************************************************************
Function Name:tsc_delete_nschain_repository_entry
Input Parameters:
	nschain_repository_node_p: Repository entry to be deleted. 
	nw_type                  : VXLAN or NVGRE		
        nid			         : Network identifier,VNI for VXLAN 
   table_id                  : Table_id of the service chaining tables 1 or 2              
Output Parameters:NONE
Description:Deletes the given service chaining repository entry associated with the given Table.
****************************************************************************************************************/
int32_t tsc_delete_nschain_repository_entry(struct nschain_repository_entry* nschain_repository_node_p,
                                            uint8_t nw_type,uint32_t nid,uint8_t table_id)
{
  struct    mchash_table* nsc_repository_table_p;
  struct    vn_service_chaining_info* vn_nsc_info_p;
  struct    crm_virtual_network*      vn_entry_p;
  uint64_t  vn_handle;
  uint32_t  offset,index,magic;
  int32_t   retval;
  uint8_t   status_b = FALSE;
  //struct    nsc_selector_node* selector_p;

  retval  = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    return OF_FAILURE;
  }

  printf("\r\n Entered tsc_delete_nschain_repository_entry. This is unwanted");

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    return OF_FAILURE;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    return OF_FAILURE;
  }    
  if(table_id == TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1)
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_1_p;
  else
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_2_p;

  index = nschain_repository_node_p->selector.index;
  magic = nschain_repository_node_p->selector.magic;
 
  nschain_repository_node_p->skip_this_entry = 1; 
  
  CNTLR_RCU_READ_LOCK_TAKE();
  tsc_mark_cn_bind_entry_as_stale(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p,vn_nsc_info_p->nsc_cn_bind_mempool_g,index,magic);
  
  offset =  NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
  MCHASH_DELETE_NODE_BY_REF(nsc_repository_table_p,nschain_repository_node_p->index,nschain_repository_node_p->magic,
                            struct nschain_repository_entry *,offset,status_b);

  if(status_b == TRUE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Repository entry deleted successfully from table repository %d",table_id);
    //CNTLR_RCU_READ_LOCK_RELEASE(); 
    retval = OF_SUCCESS;
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Repository entry deletion failed from table repository %d",table_id);
    //CNTLR_RCU_READ_LOCK_RELEASE();
    retval = OF_FAILURE;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return retval;
}
/*******************************************************************************************************
Function Name:tsc_get_nschain_repository_entry
Description:Obtains the matching service chaining repository entry from the given Table of 1 or 2.
*******************************************************************************************************/
int32_t  tsc_get_nschain_repository_entry (char*    switch_name_p,
                                           uint64_t dp_handle,   
                                           uint8_t  nw_type,
                                           uint32_t nid,
                                           uint32_t in_port_id,
                                           uint64_t metadata,
                                           struct   ofl_packet_in* pkt_in,
                                           uint8_t  pkt_origin,      
                                           uint8_t  pkt_dir,
                                           struct   nsc_selector_node* selector_p,
                                           uint32_t hashkey, 
                                           struct   nschain_repository_entry** nschain_repository_node_p_p
                                          )
{
  struct  nschain_repository_entry* nsc_repository_entry_p = NULL;
  struct  nschain_repository_entry* nsc_repository_scan_node_p; 
  uint64_t vn_handle;
  uint32_t offset;
  uint32_t index,magic;
  int32_t  status = FALSE;
  uint8_t  heap_b;
  struct   mchash_table*  nsc_repository_table_p;
  void*    nsc_repository_mempool_g;

  uchar8_t* hashobj_p = NULL;
  struct    vn_service_chaining_info* vn_nsc_info_p;
  struct    crm_virtual_network*      vn_entry_p;

  int32_t  retval = OF_SUCCESS; 
             
  selector_p->nsc_entry_present = 0;

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    return OF_FAILURE;
  }
  
  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    return OF_FAILURE;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    return OF_FAILURE;
  }
  offset = NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;

  CNTLR_RCU_READ_LOCK_TAKE();
  if(pkt_dir == TSC_PKT_OUTBOUND)
  {
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_1_p;
    nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_1_mempool_g;
  }
  else
  {
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_2_p;
    nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_2_mempool_g;
  }
  MCHASH_BUCKET_SCAN(nsc_repository_table_p,hashkey,nsc_repository_scan_node_p,struct nschain_repository_entry *, offset)
  {
    if(nsc_repository_scan_node_p->skip_this_entry == 1)
      continue;

  #if 0
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%x,%x",selector_p->src_ip,nsc_repository_scan_node_p->selector.src_ip);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%x,%x",selector_p->dst_ip,nsc_repository_scan_node_p->selector.dst_ip);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%x,%x",selector_p->protocol,nsc_repository_scan_node_p->selector.protocol);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%d,%d",selector_p->src_port,nsc_repository_scan_node_p->selector.src_port);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%d,%d",selector_p->dst_port,nsc_repository_scan_node_p->selector.dst_port);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%x,%x",selector_p->ethernet_type,nsc_repository_scan_node_p->selector.ethernet_type);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%x,%x",in_port_id,nsc_repository_scan_node_p->in_port_id);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%llx,%llx",dp_handle, nsc_repository_scan_node_p->dp_handle);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"%x,%x",selector_p->vlan_id_pkt,nsc_repository_scan_node_p->vlan_id_pkt);
   #endif
   if(  (selector_p->src_ip   != nsc_repository_scan_node_p->selector.src_ip )  ||
        (selector_p->dst_ip   != nsc_repository_scan_node_p->selector.dst_ip )  ||
        (selector_p->protocol != nsc_repository_scan_node_p->selector.protocol) ||
        (selector_p->src_port != nsc_repository_scan_node_p->selector.src_port) ||
        (selector_p->dst_port != nsc_repository_scan_node_p->selector.dst_port)
                                       ||
        (selector_p->ethernet_type != nsc_repository_scan_node_p->selector.ethernet_type)
                                       ||
        ((in_port_id != nsc_repository_scan_node_p->in_port_id)) 
                                       ||
        ((dp_handle  != nsc_repository_scan_node_p->dp_handle))
                                       ||
        ((selector_p->vlan_id_pkt != nsc_repository_scan_node_p->vlan_id_pkt)) 
      )
      continue;
    
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nsc repository entry found");
      selector_p->nsc_entry_present = 1;
      *nschain_repository_node_p_p = nsc_repository_scan_node_p; 
      CNTLR_RCU_READ_LOCK_RELEASE();
      return OF_SUCCESS;
  }
 
  CNTLR_RCU_READ_LOCK_RELEASE(); 

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"get new mem block for nsc repository entry"); 
  retval = mempool_get_mem_block(nsc_repository_mempool_g,(uchar8_t **)&nsc_repository_entry_p,&heap_b);
  if(retval != MEMPOOL_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Memory block allocation for nsc repository entry failed");
    return OF_FAILURE;
  }

  nsc_repository_entry_p->heap_b = heap_b;
  strcpy(nsc_repository_entry_p->local_switch_name_p,switch_name_p);

  nsc_repository_entry_p->skip_this_entry = 0; 
  nsc_repository_entry_p->in_port_id    = in_port_id;
  nsc_repository_entry_p->metadata      = (metadata & 0xF0FFFFFFFFFFFFFF);
  nsc_repository_entry_p->serviceport   = (uint16_t)(metadata >> SERVICE_PORT_OFFSET);
  nsc_repository_entry_p->metadata_mask = 0xF0FFFFFFFFFFFFFF;
  nsc_repository_entry_p->pkt_origin    = pkt_origin;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %d",pkt_origin);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_type    = %d",nw_type);

  nsc_repository_entry_p->nw_type    = nw_type;
  nsc_repository_entry_p->nid        = nid;
  nsc_repository_entry_p->dp_handle  = dp_handle;   
 
  nsc_repository_entry_p->flow_mod_priority  =  1;
  nsc_repository_entry_p->next_table_id      =  3;  /* default  */
  
  /* Initialize to no service chaining for the tuple before searching */
  nsc_repository_entry_p->ns_chain_b = FALSE;
  nsc_repository_entry_p->more_nf_b  = FALSE;

  nsc_repository_entry_p->next_vlan_id  = 0;
  nsc_repository_entry_p->match_vlan_id = 0;
  nsc_repository_entry_p->next_table_id = 0;
  nsc_repository_entry_p->out_port_no   = 0;
 
  nsc_repository_entry_p->selector = *selector_p;  /* copy packet fields. */

  nsc_repository_entry_p->vlan_id_pkt = selector_p->vlan_id_pkt;

  /* If VN is not configured with service chaining, return success. */
  if(vn_nsc_info_p->service_chaining_type == SERVICE_CHAINING_NONE)
  {
    nsc_repository_entry_p->service_chaining_type = SERVICE_CHAINING_NONE; /* May subsequently be enabled */
    retval = OF_SUCCESS;
  }
  else
  {
    nsc_repository_entry_p->service_chaining_type = vn_nsc_info_p->service_chaining_type;
  }

  /* For 2 port S-VM vlan_id_pkt may be zero */
#if 0  
  if((pkt_origin != VM_APPLN) && (nsc_repository_entry_p->vlan_id_pkt == 0))
  {
     /* TBD drop the packet */
     OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG," A packet without a VLAN-ID reached NSCHAIN Tables 1 or 2"); 
     /* Assuming that such packets go from table 0 to Table 3 directly */
     retval = OF_FAILURE;
  }
#endif

  /* Get match_vlan_id, next_vlan_id, out_next_service_port using NSRM API */
  
  retval = nsc_get_l2_nschaining_info(nsc_repository_entry_p,pkt_in,pkt_origin);
  if(retval == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get service chaining information");
    retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                       (uchar8_t *)nsc_repository_entry_p,
                                       nsc_repository_entry_p->heap_b
                                      );
    return OF_FAILURE;
  }
  
  CNTLR_RCU_READ_LOCK_TAKE();

  /* Add to the inbound or outbound hash table based on pkt_dir */ 
  /* nsc_repository_entry to nsc_repository_table */
  offset = NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
  hashobj_p = (uchar8_t *)(nsc_repository_entry_p) + offset;
  
  MCHASH_APPEND_NODE(nsc_repository_table_p,hashkey,nsc_repository_entry_p,index,magic,hashobj_p,status);
  if(status == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to append repository entry");
    
    retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                       (uchar8_t *)nsc_repository_entry_p,
                                       nsc_repository_entry_p->heap_b
                                      );
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to append repository entry and free the repository entry memory");
    retval = OF_FAILURE;
  }
  else
  {
    retval = NSC_ADDED_REPOSITORY_ENTRY;
  
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully appended nsc repository entry to hash table");
    nsc_repository_entry_p->magic = magic;
    nsc_repository_entry_p->index = index;

    nsc_repository_entry_p->safe_reference = magic;
    (nsc_repository_entry_p->safe_reference) = (((nsc_repository_entry_p->safe_reference) << 32) | (index));

    CNTLR_LOCK_TAKE(nsc_repository_entry_p->selector.cnbind_node_p->cnbind_node_lock);
    ++(nsc_repository_entry_p->selector.cnbind_node_p->no_of_flow_entries_deduced);
    //printf("\r\n present no_of_flow_entries_deduced = %d",nsc_repository_entry_p->selector.cnbind_node_p->no_of_flow_entries_deduced);
    CNTLR_LOCK_RELEASE(nsc_repository_entry_p->selector.cnbind_node_p->cnbind_node_lock);

    #if 0
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan_id_pkt    = %d",nsc_repository_entry_p->vlan_id_pkt);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"match_vlan_id  = %d",nsc_repository_entry_p->match_vlan_id);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_vlan_id   = %d",nsc_repository_entry_p->next_vlan_id);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"out_port_no    = %d",nsc_repository_entry_p->out_port_no);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_table_id  = %d",nsc_repository_entry_p->next_table_id);
    #endif

    *(nschain_repository_node_p_p) = nsc_repository_entry_p;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return retval; 
} 
/*******************************************************************************************************
 *Function Name: tsc_add_t3_procactive_flow_for_vm
 *
 *
*******************************************************************************************************/
int32_t tsc_add_t3_proactive_flow_for_vm(uint64_t crm_port_handle,uint64_t crm_vn_handle,uint64_t dp_handle)
{
  struct   ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p = NULL;
  struct   crm_port* crm_port_p;
  uint32_t nid,offset,index,magic,hashkey,hashmask;
  uint8_t  heap_b,nw_type;
  struct   mchash_table*  nsc_repository_table_p;
  void*    nsc_repository_mempool_g;

  uchar8_t* hashobj_p = NULL;
  struct    vn_service_chaining_info* vn_nsc_info_p;
  struct    crm_virtual_network*      vn_entry_p;

  uint64_t metadata;
  uint16_t serviceport;
  int32_t  retval   = OF_SUCCESS;
  int32_t  status   = OF_SUCCESS;
  uint8_t  status_b = FALSE;

  CNTLR_RCU_READ_LOCK_TAKE();

  do
  {
    retval = crm_get_port_byhandle(crm_port_handle,&crm_port_p);
    if(retval == CRM_SUCCESS)
    {
      nw_type = crm_port_p->nw_type;  
      if(nw_type == VXLAN_TYPE)
      {
        serviceport = (crm_port_p->nw_params).vxlan_nw.service_port;
        nid         = (crm_port_p->nw_params).vxlan_nw.nid;
      }
      else if(nw_type == NVGRE_TYPE)
      {
        serviceport = (crm_port_p->nw_params).nvgre_nw.service_port;
        nid         = (crm_port_p->nw_params).nvgre_nw.nid;
      }
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get crm port"); 
      status = OF_FAILURE;
      break;
    }    

    retval = crm_get_vn_byhandle(crm_vn_handle,&vn_entry_p);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
      status = OF_FAILURE;
      break;
    }

    vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
    vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
    if(vn_nsc_info_p == NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
      status = OF_FAILURE;
      break;
    }
    offset = NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;

    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_3_p;
    nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_3_mempool_g;

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"get new mem block for ucastpkt outport repository entry");
    retval = mempool_get_mem_block(nsc_repository_mempool_g,(uchar8_t **)&ucastpkt_outport_repository_entry_p,&heap_b);
    if(retval != MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Memory block allocation for ucastpkt outport repository entry failed");
      status = OF_FAILURE;
      break;
    }
    strcpy(ucastpkt_outport_repository_entry_p->local_switch_name_p,crm_port_p->switch_name);
    ucastpkt_outport_repository_entry_p->nw_type        = nw_type;
    ucastpkt_outport_repository_entry_p->dp_handle      = dp_handle;

    ucastpkt_outport_repository_entry_p->nid            = nid;
    ucastpkt_outport_repository_entry_p->serviceport    = serviceport;
    ucastpkt_outport_repository_entry_p->idle_time      = 0;

    metadata = 0;
    metadata = (((uint64_t)(serviceport)) << SERVICE_PORT_OFFSET);
    metadata = metadata | (nid)
                        | (((uint64_t)(nw_type)) << NW_TYPE_OFFSET);

    ucastpkt_outport_repository_entry_p->metadata       = (metadata & 0xF0FFFFFFFFFFFFFF);
    ucastpkt_outport_repository_entry_p->metadata_mask  = 0xF0FFFFFFFFFFFFFF;
    ucastpkt_outport_repository_entry_p->tun_id         = (metadata & 0x0000000000FFFFFF);

    ucastpkt_outport_repository_entry_p->nw_port_b      = FALSE;
    ucastpkt_outport_repository_entry_p->priority       = 2;

    memcpy(ucastpkt_outport_repository_entry_p->dst_mac,crm_port_p->vmInfo.vm_port_mac,6);
    ucastpkt_outport_repository_entry_p->dst_mac_high  =  ntohl(*(uint32_t*)(ucastpkt_outport_repository_entry_p->dst_mac));
    ucastpkt_outport_repository_entry_p->dst_mac_low   =  ntohl(*(uint32_t*)(ucastpkt_outport_repository_entry_p->dst_mac + 2));

    ucastpkt_outport_repository_entry_p->out_port_no = crm_port_p->port_id;
    //ucastpkt_outport_repository_entry_p->tun_dest_ip = crm_port_p->switch_ip;
    ucastpkt_outport_repository_entry_p->port_type   = crm_port_p->crm_port_type;

    /* Add to the table 3 repository table */
    offset = NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;
    hashobj_p = (uchar8_t *)(ucastpkt_outport_repository_entry_p) + offset;
    
    hashmask = nsc_no_of_repository_table_3_buckets_g;
    hashkey  = nsc_compute_hash_repository_table_3(serviceport,
                                                   ucastpkt_outport_repository_entry_p->dst_mac_high,
                                                   ucastpkt_outport_repository_entry_p->dst_mac_low,
                                                   dp_handle,
                                                   hashmask);
    
    MCHASH_APPEND_NODE(nsc_repository_table_p,hashkey,ucastpkt_outport_repository_entry_p,index,magic,hashobj_p,status_b);
    if(status_b == FALSE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add table 3 repository entry");

      retval = NSC_FAILED_TO_ADD_REPOSITORY_ENTRY;
      retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                         (uchar8_t *)ucastpkt_outport_repository_entry_p,
                                         ucastpkt_outport_repository_entry_p->heap_b
                                        );
      status = OF_FAILURE;
      break;
    }

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_3 Successfully appended ucastpkt outport repository entry to hash table");
    ucastpkt_outport_repository_entry_p->magic = magic;
    ucastpkt_outport_repository_entry_p->index = index;

    ucastpkt_outport_repository_entry_p->safe_reference   = magic;
    (ucastpkt_outport_repository_entry_p->safe_reference) = (((ucastpkt_outport_repository_entry_p->safe_reference) << 32) | (index));

    retval = tsc_add_flwmod_1_3_msg_table_3(dp_handle,ucastpkt_outport_repository_entry_p);
    if(retval == OF_FAILURE)
    {
      status = OF_FAILURE;
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," tsc_add_v3_proactive_flow failed to send Flow Mod message to Table 3");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Successfully Sent Flow Mod message to Table 3");
      //printf("\r\n tsc_add_t3_proactive_flow_for_vm successfully added a flow");
      printf("\r\n T3PF-S");
    }    
  }while(0);

  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}  
/*******************************************************************************************************
Function Name: tsc_unicast_table_3_miss_entry_pkt_rcvd
Input  Parameters:
        dp_handle  :datapath handle of the Logical switch
        pkt_in     :miss Packet received
        in_port_id :ID of the Port on which the packet is received.
        metadata   :metadata field in the pipeline
Output Parameters: NONE
Description:Miss Packet received from OF Table 3 of a logical switch whose handle is received.
            Table_3 is used for Flow Control purpose.
*******************************************************************************************************/
int32_t tsc_unicast_table_3_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                struct   ofl_packet_in* pkt_in,
                                                struct   of_msg* msg,
                                                uint64_t metadata,
                                                uint32_t in_port,
                                                uint32_t tun_src_ip)
{
  uint8_t   ii,nw_type;
  uint32_t  nid;
  uint16_t  serviceport;
  char*     local_switch_name_p;
  uint8_t*  pkt_data;
  struct    ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p = NULL;
  struct    table_barrier_msg*        table_3_barrier = NULL;   
  int32_t   retval = OF_SUCCESS;

  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Miss Packet received by Table_3");

  nw_type = (uint8_t)(metadata >> NW_TYPE_OFFSET);
  nid = (uint32_t)(metadata & TUN_ID_MASK);
  serviceport  = (uint16_t)(metadata >> SERVICE_PORT_OFFSET);

  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nw_type = %x",nw_type);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nid = %x",nid);
  
  do
  {
    CNTLR_RCU_READ_LOCK_TAKE();
    retval = get_swname_by_dphandle(dp_handle,&(local_switch_name_p));
    if(retval != DPRM_SUCCESS)
    {
      retval = OF_FAILURE;
      CNTLR_RCU_READ_LOCK_RELEASE();
      return retval;
    }
    CNTLR_RCU_READ_LOCK_RELEASE();

    retval = tsc_get_ucastpkt_outport_repository_entry(dp_handle,
                                                       local_switch_name_p,nw_type,nid,
                                                       serviceport,metadata,
                                                       pkt_in,
                                                       &ucastpkt_outport_repository_entry_p
                                                      );

    if(retval != OF_SUCCESS)
    {
      printf("\r\n TABLE_3 Failed to get ucastpkt_outport_repository_entry");   
      pkt_data = pkt_in->packet;
      for(ii=0;ii<36;ii++)
        printf(" %x",pkt_data[ii]);  
      return OF_FAILURE;
    } 
    retval = tsc_add_flwmod_1_3_msg_table_3(dp_handle,ucastpkt_outport_repository_entry_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Flow Mod message to Table 3");
    }
    else
    {
      table_3_barrier = (struct    table_barrier_msg*)calloc(1,sizeof(struct    table_barrier_msg));
      table_3_barrier->dp_handle     = dp_handle;
      table_3_barrier->in_port       = in_port;
      table_3_barrier->msg           = msg;
      table_3_barrier->packet        = pkt_in->packet;
      table_3_barrier->packet_length = pkt_in->packet_length;

      retval = of_send_barrier_request_msg(dp_handle,
                                           tsc_barrier_reply_notification_fn,
                                           table_3_barrier,NULL);

      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"barrier request for table 3 failed");
        retval = tsc_send_pktout_1_3_msg(dp_handle,in_port,pkt_in->packet,pkt_in->packet_length);
        if(retval == OF_FAILURE)
        {
           OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Packet out message for Table 2");
        }  
      }
    }

  }while(0);
  return retval;
}
/*******************************************************************************************************
Function Name:tsc_get_ucastpkt_outport_repository_entry
Description:Obtains the matching Table_3 repository entry.
*******************************************************************************************************/
int32_t  tsc_get_ucastpkt_outport_repository_entry(uint64_t dp_handle,
                                                   char*    local_switch_name_p,
                                                   uint8_t  nw_type,
                                                   uint32_t nid,
                                                   uint16_t serviceport,
                                                   uint64_t metadata,
                                                   struct   ofl_packet_in* pkt_in,
                                                   struct   ucastpkt_outport_repository_entry** ucastpkt_outport_repository_node_p_p
                                                  )
{
  struct    ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p     = NULL;
  struct    ucastpkt_outport_repository_entry* ucastpkt_outport_repository_scan_node_p = NULL; 
  uint8_t*  pkt_data;
  uint64_t  crm_port_handle;
  struct    crm_port* dst_crm_port_p; 

  uint64_t  vn_handle;
  uint32_t  offset;
  uint32_t  index,magic;
  uint32_t  hashkey,hashmask;
  uint8_t   heap_b;
  struct    mchash_table*  nsc_repository_table_p;
  void*     nsc_repository_mempool_g;

  uchar8_t* hashobj_p = NULL;
  struct    vn_service_chaining_info* vn_nsc_info_p;
  struct    crm_virtual_network*      vn_entry_p;
  uint32_t  dst_mac_high,dst_mac_low;

  struct   dprm_switch_info config_info;
  struct   dprm_switch_runtime_info runtime_info;
  uint64_t switch_handle;
  uint32_t ii,no_of_remote_ips;
  uint32_t *remote_ips_p = NULL;

  int32_t   retval = OF_SUCCESS;
  uint8_t   status = FALSE;

  pkt_data = pkt_in->packet;

  dst_mac_high  =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET));
  dst_mac_low   =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET + 2));

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    return OF_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    return OF_FAILURE;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    return OF_FAILURE;
  }

  offset = NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;

  CNTLR_RCU_READ_LOCK_TAKE();
    
  nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_3_p;
  nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_3_mempool_g;

  hashmask = nsc_no_of_repository_table_3_buckets_g;
  hashkey  = nsc_compute_hash_repository_table_3(serviceport,
                                                 dst_mac_high,
                                                 dst_mac_low,
                                                 dp_handle,
                                                 hashmask);

  MCHASH_BUCKET_SCAN(nsc_repository_table_p,hashkey,ucastpkt_outport_repository_scan_node_p,struct ucastpkt_outport_repository_entry *, offset)
  {
    if((serviceport  != ucastpkt_outport_repository_scan_node_p->serviceport)  ||
       (dp_handle    != ucastpkt_outport_repository_scan_node_p->dp_handle)    ||
       (dst_mac_high != ucastpkt_outport_repository_scan_node_p->dst_mac_high) || 
       (dst_mac_low  != ucastpkt_outport_repository_scan_node_p->dst_mac_low)  
      )
      continue;
 
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_3 ucastpkt_outport repository entry found");

      *(ucastpkt_outport_repository_node_p_p) = ucastpkt_outport_repository_scan_node_p;
      CNTLR_RCU_READ_LOCK_RELEASE();
      return OF_SUCCESS;
  }
  
  /* get memblock */
  
  CNTLR_RCU_READ_LOCK_RELEASE();

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"get new mem block for ucastpkt outport repository entry");
  retval = mempool_get_mem_block(nsc_repository_mempool_g,(uchar8_t **)&ucastpkt_outport_repository_entry_p,&heap_b);
  if(retval != MEMPOOL_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Memory block allocation for ucastpkt outport repository entry failed");
    return OF_FAILURE;
  }
  strcpy(ucastpkt_outport_repository_entry_p->local_switch_name_p,local_switch_name_p);
  ucastpkt_outport_repository_entry_p->nw_type        = nw_type;
  ucastpkt_outport_repository_entry_p->dp_handle      = dp_handle;
  ucastpkt_outport_repository_entry_p->dst_mac_high   = dst_mac_high;
  ucastpkt_outport_repository_entry_p->dst_mac_low    = dst_mac_low;
   
  ucastpkt_outport_repository_entry_p->nid            = nid;
  ucastpkt_outport_repository_entry_p->serviceport    = serviceport;
  ucastpkt_outport_repository_entry_p->idle_time      = 300;
  ucastpkt_outport_repository_entry_p->metadata       = (metadata & 0xF0FFFFFFFFFFFFFF);
  ucastpkt_outport_repository_entry_p->metadata_mask  = 0xF0FFFFFFFFFFFFFF;
  ucastpkt_outport_repository_entry_p->tun_id         = (metadata & 0x0000000000FFFFFF);
  ucastpkt_outport_repository_entry_p->nw_port_b      = FALSE;
  ucastpkt_outport_repository_entry_p->priority       = 1;  

  /* Extract DMAC from packet. */
  memcpy(ucastpkt_outport_repository_entry_p->dst_mac,pkt_data,6);
  
  do
  {
    /* Assuming it is a remote port get out_port_no,tun_dest_ip,remote_switch_name */
    CNTLR_RCU_READ_LOCK_TAKE();

    retval = tsc_nvm_modules[nw_type].nvm_module_get_vmport_by_dmac_nid(ucastpkt_outport_repository_entry_p->dst_mac,
                                                                        ucastpkt_outport_repository_entry_p->nid,
                                                                        &crm_port_handle);

    if(retval == OF_SUCCESS)
    {
      retval = crm_get_port_byhandle(crm_port_handle,&dst_crm_port_p);
     
      if(retval == OF_SUCCESS)
      {
        ucastpkt_outport_repository_entry_p->out_port_no = dst_crm_port_p->port_id;
        /* Used if it is a local port */
        ucastpkt_outport_repository_entry_p->tun_dest_ip = dst_crm_port_p->switch_ip;
        /* Used if it is a remote port */
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"tun_dest_ip = %x",dst_crm_port_p ->switch_ip);
        strcpy(ucastpkt_outport_repository_entry_p->remote_switch_name_p,dst_crm_port_p->switch_name);
        /* Switch Name */
        ucastpkt_outport_repository_entry_p->port_type = dst_crm_port_p->crm_port_type;
      }
      else
      {
        retval = OF_FAILURE;
        break;
      }    
    } 
    else  
    {
      //printf("\r\n Get NW Node IP");

      /* Unknown Traffic as per databases. Send to NW Node. */
      /* Need to get remote IP */  
      ucastpkt_outport_repository_entry_p->port_type = VM_GUEST_PORT;
      strcpy(ucastpkt_outport_repository_entry_p->remote_switch_name_p,"nw_node");
      no_of_remote_ips = 50; /* Assuming a max of 49 compute nodes */
      remote_ips_p = (uint32_t *)(malloc((sizeof (uint32_t) * no_of_remote_ips)));

      retval = tsc_nvm_modules[nw_type].nvm_module_get_remote_ips(local_switch_name_p,
                                                                  serviceport,0,
                                                                  &no_of_remote_ips,
                                                                  remote_ips_p);
                                                                
      //retval = tsc_vxn_get_remote_ips_by_swname_nid_sp(local_switch_name_p,serviceport,0,
      //                                                 &no_of_remote_ips,
      //                                                 remote_ips_p); 
      if(retval != OF_SUCCESS)
      {
        retval = OF_FAILURE;
        break;
      }
      
      retval = dprm_get_first_switch(&config_info,&runtime_info,&switch_handle);
      if(retval == DPRM_SUCCESS)
      {
        for(ii = 0; ii < no_of_remote_ips;ii++)
        {
          if(config_info.ipv4addr.addr == remote_ips_p[ii])
          {
            remote_ips_p[ii] = 0;
            break;
          }
        }
      }    
      else
      {
        retval = OF_FAILURE;
        break;
      }

      do
      {
        retval = dprm_get_next_switch(&config_info,&runtime_info,&switch_handle);
        if(retval ==  OF_SUCCESS)
        {
          for(ii = 0; ii < no_of_remote_ips;ii++)
          {
            if(config_info.ipv4addr.addr == remote_ips_p[ii])
            {
              remote_ips_p[ii] = 0;
              break;
            }
          }          
        }     
        else if((retval == DPRM_ERROR_INVALID_SWITCH_HANDLE) || (retval == DPRM_ERROR_NO_MORE_ENTRIES))
        {
          break;     
        }   
      }while(1);    
      
      if(retval == DPRM_ERROR_INVALID_SWITCH_HANDLE)  
      {
        retval = OF_FAILURE;
        break;     
      } 
      ucastpkt_outport_repository_entry_p->tun_dest_ip = 0; 
      for(ii = 0; ii < no_of_remote_ips;ii++)
      {
        if(remote_ips_p[ii] != 0)
        {
          ucastpkt_outport_repository_entry_p->tun_dest_ip = remote_ips_p[ii];
          break; 
        }     
      }         
      //printf("\r\n T3 remote_ip = %x",ucastpkt_outport_repository_entry_p->tun_dest_ip);
      if(ucastpkt_outport_repository_entry_p->tun_dest_ip == 0)   
      {
        printf("\r\n Failed to get nw node ip");
        retval = OF_FAILURE;
        break;
      }   
      ucastpkt_outport_repository_entry_p->idle_time  = 1000;
    }

    /*  If local and remote switch names are same 
        - Output the packet to the local VM side port.
        - Add flow with DMAC and metadata as match fields and Output to crm_port as an action. */

    if((ucastpkt_outport_repository_entry_p->port_type == VM_GUEST_PORT)
                                        ||   
       (strcmp((ucastpkt_outport_repository_entry_p->local_switch_name_p),
               (ucastpkt_outport_repository_entry_p->remote_switch_name_p)))
      )              
    {
      /* If local and remote switch names are not matching.crm_port is a remote port.
         - Output packet on appropriate NW port to reach the remote destination crm port. */
       
      /* Get unicast port nw_out_port_no */
      
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_switch_name = %s",local_switch_name_p);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"serviceport = %d",serviceport);

      retval = tsc_nvm_modules[nw_type].nvm_module_get_unicast_nwport(local_switch_name_p,
                                                                      serviceport,
                                                                      &(ucastpkt_outport_repository_entry_p->out_port_no)
                                                                      );

      if(retval != OF_SUCCESS)
      {
        retval = OF_FAILURE;
        break;
      }
      ucastpkt_outport_repository_entry_p->nw_port_b = TRUE;
    } 
  }while(0);
        
  if(retval == OF_FAILURE)
  {
    if(remote_ips_p != NULL)   
    {
      free(remote_ips_p);
    }  

    retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                       (uchar8_t *)ucastpkt_outport_repository_entry_p,
                                       ucastpkt_outport_repository_entry_p->heap_b
                                      );
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }
  
  /* Add to the table 3 repository table */
  offset = NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;
  hashobj_p = (uchar8_t *)(ucastpkt_outport_repository_entry_p) + offset;
  
  MCHASH_APPEND_NODE(nsc_repository_table_p,hashkey,ucastpkt_outport_repository_entry_p,index,magic,hashobj_p,status);
  if(status == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add table 3 repository entry");

    retval = NSC_FAILED_TO_ADD_REPOSITORY_ENTRY;
    retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                       (uchar8_t *)ucastpkt_outport_repository_entry_p,
                                       ucastpkt_outport_repository_entry_p->heap_b
                                      );
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_3 Successfully appended ucastpkt outport repository entry to hash table");
  ucastpkt_outport_repository_entry_p->magic = magic;
  ucastpkt_outport_repository_entry_p->index = index;

  ucastpkt_outport_repository_entry_p->safe_reference = magic;
  (ucastpkt_outport_repository_entry_p->safe_reference) = (((ucastpkt_outport_repository_entry_p->safe_reference) << 32) | (index));

  *(ucastpkt_outport_repository_node_p_p) = ucastpkt_outport_repository_entry_p;
  
  CNTLR_RCU_READ_LOCK_RELEASE();
  return retval;
}
/*******************************************************************************************************
Function Name: tsc_broadcast_outbound_table_4_miss_entry_pkt_rcvd 
Input  Parameters:
        dp_handle  :datapath handle of the Logical switch
        pkt_in     :miss Packet received
        in_port    :ID of the Port on which the packet is received.
        metadata   :metadata field in the pipeline
Output Parameters: NONE
Description:Miss Packet received from OF Table 3 of a logical switch whose handle is received.
            Table_4 is used for handling broadcast outbound packets.
*******************************************************************************************************/
int32_t tsc_broadcast_outbound_table_4_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                           struct   ofl_packet_in* pkt_in,
                                                           struct   of_msg* msg,                 
                                                           uint64_t metadata,
                                                           uint32_t in_port
                                                          )
{
  struct    vmports_repository_entry* vmports_repository_entry_p = NULL;
  struct    nwports_brdcast_repository_entry* nwports_brdcast_repository_entry_p = NULL;
  uint8_t   nw_type;
  uint32_t  nid;
  uint16_t  serviceport;
  char*     local_switch_name_p;
  uint32_t  retval = OF_SUCCESS;
 
  /*
     Step 1:
              - Output to Multiple NW ports.
              - Multiple VxLan ports are created for Broadcast Traffic for 
              - (Tunnel Destination IP,Service Port)
              - At present we need to create broadcast ports with ip.
     Step 2:
              - Output to all the Local VM ports and VM_NS ports belonging to the Virtual NW.
              - No VLAN ID is added for packets to VM_NS port.
              - All ports are added to the same Flow Mod message.
  */

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Miss Packet received by Table_4");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "PKT-IN METADATA3 =%llx",metadata);
 
  nw_type      = (uint8_t)(metadata  >> NW_TYPE_OFFSET);
 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nw_type =%d",nw_type); 

  CNTLR_RCU_READ_LOCK_TAKE();

  retval = get_swname_by_dphandle(dp_handle,&(local_switch_name_p));
  if(retval != DPRM_SUCCESS)
  {
    retval = OF_FAILURE;
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "switch is not found");
    return retval;    
  }
   
  CNTLR_RCU_READ_LOCK_RELEASE();

  nid = (uint32_t)(metadata & TUN_ID_MASK);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "vni =%x",nid);
  serviceport  = (uint16_t)(metadata >> SERVICE_PORT_OFFSET);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "switch name =%s",local_switch_name_p);

  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"metadata = %llx",metadata);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"switch name = %s",local_switch_name_p);
  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Entering brdcast repository entry");
  retval = tsc_get_nwports_brdcast_repository_entry(local_switch_name_p,
                                                    nw_type,
                                                    0, /* nid,*/
                                                    serviceport,
                                                    metadata,
                                                    &nwports_brdcast_repository_entry_p);

  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Broadcast Ports not found table 4");
    return OF_FAILURE;
  }
  retval = tsc_get_vmports_repository_entry(local_switch_name_p,nw_type,nid,
                                             metadata,
                                             &vmports_repository_entry_p
                                            );
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "VM ports not found table 4");
    if(nwports_brdcast_repository_entry_p != NULL)
      free(nwports_brdcast_repository_entry_p);
    return OF_FAILURE;
  }
  retval = tsc_add_flwmod_1_3_msg_table_4(dp_handle,vmports_repository_entry_p,
                                          nwports_brdcast_repository_entry_p);

  if(retval == OF_FAILURE)
  { 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Flow Mod message to Table 4");  
  }
  else
  {
    retval = tsc_send_pktout_1_3_msg(dp_handle,in_port,pkt_in->packet,pkt_in->packet_length);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Packet out message for Table 4");
    }
  }
  
  if(vmports_repository_entry_p->vmside_ports_p)
      free(vmports_repository_entry_p->vmside_ports_p);
  if(vmports_repository_entry_p != NULL)
    free(vmports_repository_entry_p);
  if(nwports_brdcast_repository_entry_p->nwside_br_ports_p != NULL )
      free(nwports_brdcast_repository_entry_p->nwside_br_ports_p);
  if(nwports_brdcast_repository_entry_p != NULL)
    free(nwports_brdcast_repository_entry_p);

  return retval;
}
/*******************************************************************************************************
*This function is used to delete all flows in the nsc_repository for Table 1 and Table 2 by matching 
*VLAN IDs of the service VM asssocited with the service deleted.
*******************************************************************************************************/
int32_t tsc_delete_nsc_tables_1_2_flows_for_service_vm_deleted(uint64_t vn_handle,uint64_t crm_port_handle)
{
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   nschain_repository_entry* nsc_repository_node_p;
  struct   mchash_table* nsc_repository_table_p;
  uint8_t  skip_this_entry;
  uint32_t hashkey,offset,retval;
  uint16_t vlan_id_in,vlan_id_out;
  struct   crm_virtual_network* vn_entry_p;
  struct   crm_port* crm_port_p;

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    printf("NSMDEL 02 ");
    return OF_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_TAKE();

  retval =  crm_get_port_byhandle(crm_port_handle,&crm_port_p);
  if(retval != CRM_SUCCESS)
  {
    printf("NSMDEL 02A ");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }
  
  vlan_id_in  = crm_port_p->vlan_id_in;
  vlan_id_out = crm_port_p->vlan_id_out;

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }
  nsc_repository_table_p  = vn_nsc_info_p->nsc_repository_table_1_p;
  hashkey = 0;
  skip_this_entry = 1;

  MCHASH_TABLE_SCAN(nsc_repository_table_p,hashkey)
  {
    offset = NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
    MCHASH_BUCKET_SCAN(nsc_repository_table_p,hashkey,nsc_repository_node_p,struct nschain_repository_entry *,offset)
    {
      if( (((nsc_repository_node_p->match_vlan_id) & 0xfff)  == vlan_id_in)
                              ||
          (((nsc_repository_node_p->match_vlan_id) & 0xfff)  == vlan_id_out)
                              ||
          (((nsc_repository_node_p->next_vlan_id)  & 0xfff)  == vlan_id_in )
                              ||
          (((nsc_repository_node_p->next_vlan_id)  & 0xfff)   == vlan_id_out)                    
        )        
      {
        nsc_repository_node_p->skip_this_entry = 1; 
        //dp_handle = nsc_repository_node_p->dp_handle;
        
        if(skip_this_entry)
        {
          CNTLR_LOCK_TAKE(nsc_repository_node_p->selector.cnbind_node_p->cnbind_node_lock);
          nsc_repository_node_p->selector.cnbind_node_p->skip_this_entry = 1;
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Mark the cn_bind_entry as skip_this_entry so that it is not used further.");
          CNTLR_LOCK_RELEASE(nsc_repository_node_p->selector.cnbind_node_p->cnbind_node_lock);
          //printf("\r\n cn_bind_entry is marked as skip_this_entry"); 
          skip_this_entry = 0; 
        }

        tsc_ofplugin_v1_3_del_table_1_2_cnbind_flow_entries(nsc_repository_node_p,TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1);
      }
    }
  }

  nsc_repository_table_p  = vn_nsc_info_p->nsc_repository_table_2_p;
  hashkey = 0;

  MCHASH_TABLE_SCAN(nsc_repository_table_p,hashkey)
  {
    offset = NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
    MCHASH_BUCKET_SCAN(nsc_repository_table_p,hashkey,nsc_repository_node_p,struct nschain_repository_entry *,offset)
    {
      if( (((nsc_repository_node_p->match_vlan_id) & 0xfff)  == vlan_id_in)
                                           ||
          (((nsc_repository_node_p->match_vlan_id) & 0xfff)  == vlan_id_out)
                                           ||
          (((nsc_repository_node_p->next_vlan_id)  & 0xfff)  == vlan_id_in )
                                           ||
          (((nsc_repository_node_p->next_vlan_id)  & 0xfff)  == vlan_id_out)
        )
      {
        nsc_repository_node_p->skip_this_entry = 1;  
        //dp_handle = nsc_repository_node_p->dp_handle;
        tsc_ofplugin_v1_3_del_table_1_2_cnbind_flow_entries(nsc_repository_node_p,TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2);
      }
    }
   }
   CNTLR_RCU_READ_LOCK_RELEASE();
   return OF_SUCCESS;
}
/*************************************************************************************************************
Functi:on Name: tsc_flow_entry_removal_msg_recvd_nschain_tables
Description:A flow entry removal message is received from a logical switch for nschain Tables.
*************************************************************************************************************/
int32_t tsc_flow_entry_removal_msg_recvd_nschain_tables(uint64_t dp_handle,uint64_t metadata,
                                                        uint64_t cookie,uint8_t reason,uint8_t table_id)
{

  uint8_t  nw_type;
  uint32_t nid,offset;
  uint64_t vn_handle;
  struct   crm_virtual_network* vn_entry_p;
  uint8_t  status_b = FALSE;
  uint32_t index,magic;
  struct   mchash_table* nsc_repository_table_p;
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   nschain_repository_entry* nsc_repository_node_p;
  int32_t  retval = OF_SUCCESS;

  nw_type = (uint8_t)(metadata >> NW_TYPE_OFFSET);
  nid     = (uint32_t)(metadata & TUN_ID_MASK);

  retval  = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    return OF_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    return OF_FAILURE;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    return OF_FAILURE;
  }
  offset =  NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;

  if(table_id == TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1)
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_1_p;
  else if(table_id == TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2) 
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_2_p;
  else
    return OF_FAILURE;
  
  magic = (uint32_t)(cookie >>32);
  index = (uint32_t)cookie;

  CNTLR_RCU_READ_LOCK_TAKE();

  MCHASH_ACCESS_NODE(nsc_repository_table_p,index,magic,nsc_repository_node_p,status_b);

  if(status_b != TRUE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"No repository entry found matching with the flow removal message received for table = %d",table_id);
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }

  offset =  NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;

//if((reason == OFPRR_IDLE_TIMEOUT) || (reason == OFPRR_HARD_TIMEOUT ))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Deleting the nschain repository entry found");

    index = nsc_repository_node_p->selector.index;
    magic = nsc_repository_node_p->selector.magic;

    nsc_repository_node_p->skip_this_entry = 1;
    tsc_mark_cn_bind_entry_as_stale(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p,vn_nsc_info_p->nsc_cn_bind_mempool_g,index,magic);

    MCHASH_DELETE_NODE_BY_REF(nsc_repository_table_p, nsc_repository_node_p->index,nsc_repository_node_p->magic,
                              struct nschain_repository_entry *,offset,status_b);

    if(status_b == TRUE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Repository entry deleted successfully from table repository dp handle = %llx,%d",dp_handle,table_id);
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Repository entry deletion failed from table repository dp handle = %llx,%d",dp_handle,table_id);
      retval = OF_FAILURE;
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
#if 0
  if(reason == OFPRR_DELETE)
  {
    if(table_id == TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1)
      retval = tsc_add_flwmod_1_3_msg_table_1(dp_handle,nsc_repository_node_p);
    else
      retval = tsc_add_flwmod_1_3_msg_table_2(dp_handle,nsc_repository_node_p);    

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add the flow entry to the switch upon receiving flow eviction event from switch table = %d",table_id);
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully added the flow entry to the switch upon receiving flow eviction event from switch table = %d",table_id);
    }
  }
#endif  
  return retval;
}
/*******************************************************************************************************
Function Name: tsc_flow_entry_removal_msg_recvd_table_3
*******************************************************************************************************/
int32_t tsc_flow_entry_removal_msg_recvd_table_3(uint64_t dp_handle,uint64_t metadata,
                                                 uint64_t cookie,uint8_t reason)
{
  uint8_t  nw_type;
  uint32_t nid;   
  //uint16_t service_port;
  uint64_t vn_handle;
  struct   crm_virtual_network* vn_entry_p;
  uint8_t  status_b = FALSE;
  uint32_t index,magic,offset;
  struct   mchash_table* nsc_repository_table_p;
  //void*    nsc_repository_mempool_g = NULL;
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p;

  int32_t  retval = OF_SUCCESS;

  nw_type       =  (uint8_t)(metadata >> NW_TYPE_OFFSET);
  nid           =  (uint32_t)(metadata & TUN_ID_MASK);
  //service_port  =  (uint16_t)(metadata >> SERVICE_PORT_OFFSET);


  retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    return OF_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    return OF_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  
  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }
  offset        =  NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;
  nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_3_p;

  magic = (uint32_t)(cookie >>32);
  index = (uint32_t)cookie;

  MCHASH_ACCESS_NODE(nsc_repository_table_p,index,magic,ucastpkt_outport_repository_entry_p,status_b);

  if(status_b != TRUE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"No repository entry found matching with the flow removal message received for table 3");
    CNTLR_RCU_READ_LOCK_RELEASE();
    printf("\r\n table 3 - for flow removal  ucastpkt_outport_repository_entry is not found");
    return OF_FAILURE;
  }

 // if((reason == OFPRR_IDLE_TIMEOUT) || (reason == OFPRR_HARD_TIMEOUT ))
    {
      /* Delete the repository entry matching the flow removed in switch by time outs */
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Deleting the ucastpkt_outport repository entry found");
      status_b = FALSE; 
      MCHASH_DELETE_NODE_BY_REF(nsc_repository_table_p, ucastpkt_outport_repository_entry_p->index,ucastpkt_outport_repository_entry_p->magic,
                                struct ucastpkt_outport_repository_entry *,offset,status_b);
      if(status_b == TRUE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Repository entry deleted successfully from table_3 repository dp handle = %llx",dp_handle);
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Repository entry deletion failed from table_3 repository do handle = %llx",dp_handle);
        retval = OF_FAILURE;
      }
    }
    CNTLR_RCU_READ_LOCK_RELEASE();
#if 0
    if(reason == OFPRR_DELETE)
    {
      /* Add Flow again if it is evicted from table in switch */
     retval = tsc_add_flwmod_1_3_msg_table_3(dp_handle,ucastpkt_outport_repository_entry_p);
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add the flow entry to the switch upon receiving flow eviction event from switch");
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully added the flow entry to the switch upon receiving flow eviction event from switch");
      }
    }
#endif    
  return OF_SUCCESS; 
}  
/*******************************************************************************************************
Function Name: tsc_delete_table_3_flow_entries
Description: Deletes the matching flow entries of Table 3 in multiple logical switches.
*******************************************************************************************************/
int32_t tsc_delete_table_3_flow_entries(uint8_t crm_port_type,uint64_t crm_port_handle,uint64_t dp_handle)
{

  /*
  Addition: Nothing to be Done on port addition. Flows are added when packets are received on Table 3.

  VM or VM_NS port deletion:
   - Upon port deletion, delete flows from repository hash Table for OF Table 3.
   - Multiple flow entries may exist possibly one for each switch.

   - Hash on MAC Address associated with VM port, dp_handle of each logical switch,service port and delete repository entries.
   - Delete entries from switches using dp_handle of each logical switch and with port mac address as match field.

   Obtain vn_handle and vn_entry from crm_port handle and service port.
   In a loop obtain dp_handle for each logical switch.
   Use mac address, service port along with dp_handle of each logical switch to identify the repository entry and remove it.
   Delete entries from switches using dp_handle of each logical switch and with port mac address as match field.

   vn_entry->compute_node->logical_switch. Get dp_handle and delete entry from repository and switch.
   */
   uint8_t  port_mac_addr[6];
   uint16_t service_port = 0;
   uint32_t dst_mac_high,dst_mac_low;
   uint64_t vn_handle;
   struct   crm_port* crm_port_p;
   struct   crm_virtual_network* vn_entry_p;
   struct   crm_compute_node* crm_compute_node_entry_scan_p;
   uint8_t  status_b = FALSE;
   uint32_t hashkey,hashmask,offset;
   struct   mchash_table* nsc_repository_table_p;
   //void*    nsc_repository_mempool_g;
   struct   vn_service_chaining_info* vn_nsc_info_p;
   struct   ucastpkt_outport_repository_entry* ucastpkt_outport_repository_scan_node_p;

   int32_t  retval = OF_SUCCESS;

   retval =  crm_get_port_byhandle(crm_port_handle,&crm_port_p);
   if(retval != CRM_SUCCESS)
   {
    return retval;
   }

   if(crm_port_p->nw_type == VXLAN_TYPE)
     service_port = (crm_port_p->nw_params).vxlan_nw.service_port;
   else if(crm_port_p->nw_type == NVGRE_TYPE)
     service_port = (crm_port_p->nw_params).nvgre_nw.service_port;

   memcpy(port_mac_addr,crm_port_p->vmInfo.vm_port_mac,6);
   dst_mac_high  =  ntohl(*(uint32_t*)(port_mac_addr));
   dst_mac_low   =  ntohl(*(uint32_t*)(port_mac_addr + 2));

   vn_handle = crm_port_p->saferef_vn;

   retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);  
   if(retval != CRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unable to fetch vn_entry");
     return OF_FAILURE;
   } 

   CNTLR_RCU_READ_LOCK_TAKE();
 
   vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  
   vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
   if(vn_nsc_info_p == NULL)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
     CNTLR_RCU_READ_LOCK_RELEASE(); 
     return OF_FAILURE;
   }
   offset = NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;

   nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_3_p;
   //nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_3_mempool_g;

   hashmask = nsc_no_of_repository_table_3_buckets_g;

   OF_LIST_SCAN(vn_entry_p->crm_compute_nodes,
                crm_compute_node_entry_scan_p,
                struct crm_compute_node*,
                CRM_VN_COMPUTE_NODE_ENTRY_LISTNODE_OFFSET)
   {
     dp_handle = crm_compute_node_entry_scan_p->dp_references[0].saferef_logical_switch;   /* Only one logical switch br-int is assumed */
 
     hashkey  = nsc_compute_hash_repository_table_3(service_port,
                                                    dst_mac_high,
                                                    dst_mac_low,
                                                    dp_handle,
                                                    hashmask);

     MCHASH_BUCKET_SCAN(nsc_repository_table_p,hashkey,ucastpkt_outport_repository_scan_node_p,struct ucastpkt_outport_repository_entry *, offset)
     {
       if((service_port  != ucastpkt_outport_repository_scan_node_p->serviceport)  ||
          (dp_handle     != ucastpkt_outport_repository_scan_node_p->dp_handle)    ||
          (dst_mac_high  != ucastpkt_outport_repository_scan_node_p->dst_mac_high) ||
          (dst_mac_low   != ucastpkt_outport_repository_scan_node_p->dst_mac_low)
         )
         continue;

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Deleting the ucastpkt_outport repository entry found");
           
       MCHASH_DELETE_NODE_BY_REF(nsc_repository_table_p, ucastpkt_outport_repository_scan_node_p->index,ucastpkt_outport_repository_scan_node_p->magic,
                                 struct ucastpkt_outport_repository_entry *,offset,status_b);
       if(status_b == TRUE)
       {
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Repository entry deleted successfully from table_3 repository dp handle = %llx",dp_handle);
       }
       else
       {
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Repository entry deletion failed from table_3 repository do handle = %llx",dp_handle);
         CNTLR_RCU_READ_LOCK_RELEASE();
         retval = OF_FAILURE;
       }
       break;
     }
     /* Delete Flow entry from switch */
     tsc_ofplugin_v1_3_del_flows_from_table_3(dp_handle,port_mac_addr);
   }
   CNTLR_RCU_READ_LOCK_RELEASE();
   return retval;  /* return success only if deletion is successful across switches */
}
/*******************************************************************************************************
Function Name:tsc_delete_table_4_flow_entries
*******************************************************************************************************/
int32_t tsc_delete_table_4_flow_entries(uint8_t crm_port_type,uint64_t crm_port_handle,uint64_t dp_handle)
{
  
  /* This function is to be called when a port (only VM,VM_NS,Broadcast) is added or deleted from a switch.
     It is to be called even if a port is added because the list of ports to which the broadcast packets to be sent is re-calculated. */
 
  /* Table_4: 
     When any of the internal VMs send a broadcast packet,the packets reach Table 4.
     A flow entry is added per each network id in one switch by TSC.
     The flow entry includes metadata as match field,VNI is set using set field action and multiple ports of two types
       -  All the local VM and VM_NS  ports in thw switch to send broadcast packets.
       -  All the Broadcast ports configured in the switch to send broadcast packets to remote VMs. 

     Addtion/Deletion of VM/VM_NS Ports: 
        - Construct metadata from the deleted VM/VM_NS port.
        - While preparing metadata mask, nwtype,network id,service port shall be taken from VM/VM_NS port details without masking out anything.
        - Delete Flows for all pkt origin values.
          - set metadata mask to F000 FFFF FFFFFFFF to delete flows set when broadcast packets are received from local VM ports.
     
     Addition/Deletion of Unicast/Broadcast port:      
        - Construct metadata from the deleted Unicast/Broadcast port.
        - While preparing metadata mask, nwtype,service port shall be taken from Unicast/Broadcast port details without masking out anything.
        - Nework ID shall not be compared as it is a flow parameter.So mask it out.
        - Two types of flows shall be deleted with a different metadata mask.
        - Delete Flows for all pkt origin values and for all network IDs with in the network type.
        - Broadcast packets may be received from network side on Unicast or Broadcast ports.    
        - As remote IP is not a flow parameter, we end up deleting flows to multiple remote broadcast destinations even if one port is deleted. 
          - set metadata mask to F000 FFFF 00000000 to delete flows set when broadcast packets are received from remote VMs on network port.

      Addition/Deletion of any of these three types of ports effect the flows in Table 4 of only the local switch. 
      Addition or Deletion of unicast port will not effect flows of Table 4. 
  */

  struct   tsc_ofver_plugin_iface* tsc_params_vm_p;
  struct   crm_port* crm_port_p;
  uint8_t  nw_type,table_id;
  int32_t  retval;

  retval =  crm_get_port_byhandle(crm_port_handle,&crm_port_p);
  if(retval != CRM_SUCCESS)
  {
    return retval;
  }

  /* Get metadata from NVM module */
  nw_type = crm_port_p->nw_type;

  tsc_params_vm_p = (struct tsc_ofver_plugin_iface *) (malloc(sizeof (struct tsc_ofver_plugin_iface )));
  if(tsc_params_vm_p == NULL)
  {
   return OF_FAILURE;
  }

  tsc_nvm_modules[nw_type].nvm_module_construct_metadata(tsc_params_vm_p,0,crm_port_p);

  if((crm_port_type == VM_PORT)      || (crm_port_type == VMNS_PORT) ||
     (crm_port_type == VMNS_IN_PORT) || (crm_port_type == VMNS_OUT_PORT))
  {
    tsc_params_vm_p->metadata      &= 0xF000FFFFFFFFFFFF;
    tsc_params_vm_p->metadata_mask  = 0xF000FFFFFFFFFFFF;
  }
  else
  {
    tsc_params_vm_p->metadata      &= 0xF000FFFF00000000;
    tsc_params_vm_p->metadata_mask  = 0xF000FFFF00000000; 
  }
  table_id = TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4; 
  tsc_ofplugin_v1_3_del_flows_from_table_4_5(dp_handle,tsc_params_vm_p,table_id);
  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Deleted flows from table_4 PKT-IN METADATA1 =%llx",tsc_params_vm_p->metadata);
  return OF_SUCCESS;
}
/*******************************************************************************************************
Function Name: tsc_delete_table_5_flow_entries
*******************************************************************************************************/
int32_t tsc_delete_table_5_flow_entries(uint8_t crm_port_type,uint64_t crm_port_handle,uint64_t dp_handle)
{

  /* This function is to be called when a port is added or deleted from a switch.*/
  /* It is to be called even if a port is added because the local list of ports to which the broadcast packets to be sent is re-calculated.*/

  /* Table_5:
     Broadcast packets are received at Table_5 from network ports of both types from remote Compute Nodes.

     A flow entry is added per each network id in one switch by TSC.
     The flow entry includes metadata as match field,VNI is set using set field action and multiple ports of two types
       -  All the local VM and VM_NS ports in the switch to send the received broadcast packets.

     Addition/Deletion of VM/VM_NS Ports:
        - Construct metadata from the deleted VM/VM_NS port.
        - While preparing metadata mask, nwtype,network id,service port shall be taken from VM/VM_NS port details without masking out anything.
        - Delete Flows for all pkt origin values.
          - set metadata mask to F000 FFFF FFFFFFFF to delete flows set when broadcast packets are received from network ports.

     Packets received from  Broadcast ports will reach Table 5 but not Table 4. 

     Note: We may receive packets from network side on Unicast or Broadcast port.
  */

  struct   tsc_ofver_plugin_iface* tsc_params_vm_p;
  struct   crm_port* crm_port_p;
  uint8_t  nw_type,table_id;
  int32_t  retval;

  retval =  crm_get_port_byhandle(crm_port_handle,&crm_port_p);
  if(retval != CRM_SUCCESS)
  {
    return retval;
  }

  /* Get metadata from NVM module */
  nw_type = crm_port_p->nw_type;

  tsc_params_vm_p = (struct tsc_ofver_plugin_iface *) (malloc(sizeof (struct tsc_ofver_plugin_iface )));
  if(tsc_params_vm_p == NULL)
  {
   return OF_FAILURE;
  }

  tsc_nvm_modules[nw_type].nvm_module_construct_metadata(tsc_params_vm_p,0,crm_port_p);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " PKT-IN METADATA1 =%llx",tsc_params_vm_p->metadata);

  if((crm_port_type == VM_PORT)      || (crm_port_type == VMNS_PORT) ||
     (crm_port_type == VMNS_IN_PORT) || (crm_port_type == VMNS_OUT_PORT))
  {
    tsc_params_vm_p->metadata      &= 0xF000FFFFFFFFFFFF;
    tsc_params_vm_p->metadata_mask  = 0xF000FFFFFFFFFFFF;
  }

  table_id = TSC_APP_BROADCAST_INBOUND_TABLE_ID_5;
  tsc_ofplugin_v1_3_del_flows_from_table_4_5(dp_handle,tsc_params_vm_p,table_id);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Deleted flows from table_5 PKT-IN METADATA1 =%llx",tsc_params_vm_p->metadata);
  return OF_SUCCESS;
}
/*******************************************************************************************************
Functionn Name: tsc_broadcast_outbound_table_5_miss_entry_pkt_rcvd
Input  Parameters:
        dp_handle  :datapath handle of the Logical switch
        pkt_in     :miss Packet received
        in_port    :ID of the Port on which the packet is received.
        metadata   :metadata field in the pipeline
Output Parameters: NONE
Description:Miss Packet received from OF Table 5 of a logical switch whose handle is received.
            Table_5 is used for handling inbound broadcast packets received.
*******************************************************************************************************/
int32_t tsc_broadcast_inbound_table_5_miss_entry_pkt_rcvd(uint64_t  dp_handle,
                                                          struct    ofl_packet_in* pkt_in,
                                                          struct    of_msg* msg,                                             
                                                          uint64_t  metadata,
                                                          uint32_t  in_port,
                                                          uint32_t  tun_src_ip
                                                         )
{
  struct   vmports_repository_entry* vmports_repository_entry_p = NULL;
  uint8_t  nw_type;
  uint32_t nid;
  uint8_t  port_type = 0;
  char*    local_switch_name_p;
  uint8_t* packet = pkt_in->packet; 
  int32_t  retval = OF_SUCCESS;

  struct    crm_port* crm_vmside_port_p;
  uint64_t  crm_port_handle;    
  uint64_t  output_crm_port_handle;
 
  /* output to all the local VM and VM_NS ports belonging to the virtual network in the list */
  /* No VLAN ID is added for packets sent to VM_NS ports.*/
           
  /* Use DPRM API to get local switch name using datapath handle.*/  
  /* Use local switch name, NW Type,VNI to find the list of VM and VM_NS ports */
  /* Send the Broadcast packet to all of them */

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Miss Packet received by Table_5");

  nw_type = (uint8_t)(metadata >> NW_TYPE_OFFSET);
 
  /* Assuming same position and size for all nvm modules */
  /* Introduce deconstruct metadata */

  nid = (uint32_t)(metadata & TUN_ID_MASK);
  
  /* nid is the 32 bit nw identifier - It can be vni,vlanid or tid based on nw type */
  /* nid along with nw type is used to look up for local vm ports for sending broadcast packets received */

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vmport_by_dmac_nid(&packet[6],nid,&crm_port_handle);
  if(retval != CRM_SUCCESS)
  {
    retval =  crm_add_guest_crm_vmport(&packet[6],tun_src_ip,&output_crm_port_handle);
    if(retval == CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully added VM_GUEST_PORT");
      port_type = VM_GUEST_PORT;
      /* Portmac view: Get MAC Address from crm port structure. */
      retval = tsc_vxn_add_vmport_to_portmac_view(output_crm_port_handle,nid);
      if(retval != CRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add VM_GUEST_PORT to the portmac view");
      }  
      else
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Added VM_GUEST_PORT to the portmac view");
      }
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Error in adding VM_GUEST_PORT");
    } 
  }
  else
  {
    crm_get_port_byhandle(crm_port_handle,&crm_vmside_port_p);
    port_type = crm_vmside_port_p->crm_port_type;
  }

  if(port_type == VM_GUEST_PORT)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"A VM_GUEST_PORT is already present");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"A VM PORT is already configured")
  }
 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"VNI = %x",nid);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"mac address = ","%x",packet[9],"%x",packet[10],"%x",packet[11]);    
  
  CNTLR_RCU_READ_LOCK_TAKE();
      
  /* Get Local Switch Name */  
  retval = get_swname_by_dphandle(dp_handle,&(local_switch_name_p));
  if(retval != DPRM_SUCCESS)
  {
    retval = OF_FAILURE;
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }

  CNTLR_RCU_READ_LOCK_RELEASE();

  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vni = %x",nid);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nw type = %x",nw_type);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"metadata = %llx",metadata);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"switch name = %s",local_switch_name_p);    
  
  retval = tsc_get_vmports_repository_entry(local_switch_name_p,
                                            nw_type,
                                            nid,
                                            metadata,
                                            &vmports_repository_entry_p
                                           );
    
  if(retval != OF_SUCCESS)
    return OF_FAILURE;

  /* metadata received in of_params_p is to be set as match field */
  retval = tsc_add_flwmod_1_3_msg_table_5(dp_handle,vmports_repository_entry_p);
  if(retval == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to send Flow Mod message to Table 5");    
  }
  else
  {
    retval = tsc_send_pktout_1_3_msg(dp_handle,in_port,pkt_in->packet,pkt_in->packet_length);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to send Packet out message for Table 5");
    }
  }
  if(vmports_repository_entry_p->vmside_ports_p)
      free(vmports_repository_entry_p->vmside_ports_p);
  if(vmports_repository_entry_p != NULL)
    free(vmports_repository_entry_p);
  return retval;
}
/*******************************************************************************************************
Function Name: tsc_get_vmports_repository_entry
*******************************************************************************************************/
int32_t tsc_get_vmports_repository_entry(char*    local_switch_name_p,
                                         uint8_t  nw_type,
                                         uint32_t nid,
                                         uint64_t metadata,
                                         struct   vmports_repository_entry** vmports_repository_node_p_p   
                                        )  
{
  struct  vmports_repository_entry* vmports_repository_entry_p = NULL;
  int32_t retval = OF_SUCCESS;

  vmports_repository_entry_p = (struct vmports_repository_entry *)
                     (malloc(sizeof (struct vmports_repository_entry)));  
  if(vmports_repository_entry_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to alloc memory for vmport repository entry Table 4");    
    return OF_FAILURE;
  } 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Allocated memory for vmport repository entry Table 4"); 
  strcpy(vmports_repository_entry_p->local_switch_name_p,local_switch_name_p);
  vmports_repository_entry_p->nw_type  = nw_type;
  vmports_repository_entry_p->nid = nid;
  vmports_repository_entry_p->metadata = metadata;
  vmports_repository_entry_p->no_of_vmside_ports = 10; /* No of ports available may be less */
  vmports_repository_entry_p->vmside_ports_p   =
    (uint32_t *)(malloc( (sizeof (uint32_t)) * (vmports_repository_entry_p->no_of_vmside_ports) ));
    
  CNTLR_RCU_READ_LOCK_TAKE();

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vmports_by_swname_nid(
                                                           vmports_repository_entry_p->local_switch_name_p,
                                                           vmports_repository_entry_p->nid,
                                                           &(vmports_repository_entry_p->no_of_vmside_ports),
                                                           vmports_repository_entry_p->vmside_ports_p);

  if(retval != OF_SUCCESS)
  {
    retval = OF_FAILURE;
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to get vmports for broadcastng the packet");
  } 

  CNTLR_RCU_READ_LOCK_RELEASE();    
 
  if(retval == OF_FAILURE)
  {
    if(vmports_repository_entry_p->vmside_ports_p)
      free(vmports_repository_entry_p->vmside_ports_p);
    if(vmports_repository_entry_p)
      free(vmports_repository_entry_p);
    return retval;
  } 

  vmports_repository_entry_p->metadata_mask = 0xF0FFFFFFFFFFFFFF; 
  vmports_repository_entry_p->metadata =
     (vmports_repository_entry_p->metadata & vmports_repository_entry_p->metadata_mask);

  *(vmports_repository_node_p_p) = vmports_repository_entry_p;
  return OF_SUCCESS;
} 
/*******************************************************************************************************
Function Name: tsc_get_nwports_brdcast_repository_entry
*******************************************************************************************************/
int32_t tsc_get_nwports_brdcast_repository_entry(char* local_switch_name_p,
                                                  uint8_t  nw_type,
                                                  uint32_t nid,
                                                  uint16_t serviceport,
                                                  uint64_t metadata,
                                                  struct nwports_brdcast_repository_entry** nwports_brdcast_repository_node_p_p)
{
  struct nwports_brdcast_repository_entry* nwports_brdcast_repository_entry_p = NULL;
  int32_t retval = OF_SUCCESS;

  nwports_brdcast_repository_entry_p = (struct nwports_brdcast_repository_entry *)
                                        (malloc(sizeof (struct nwports_brdcast_repository_entry)));
  if(nwports_brdcast_repository_entry_p == NULL)
  {
    return retval;
  }    
  strcpy(nwports_brdcast_repository_entry_p->local_switch_name_p,local_switch_name_p);
  nwports_brdcast_repository_entry_p->nw_type  = nw_type;
  nwports_brdcast_repository_entry_p->nid = nid;
  nwports_brdcast_repository_entry_p->serviceport = serviceport;
  nwports_brdcast_repository_entry_p->metadata = metadata;
  nwports_brdcast_repository_entry_p->metadata_mask = 0xF0FFFFFFFFFFFFFF;  /* mask out pkt origin */ 
  nwports_brdcast_repository_entry_p->no_of_nwside_br_ports = 50; /* No of ports available may be less */
  nwports_brdcast_repository_entry_p->nwside_br_ports_p  =
                       (uint32_t *)(malloc((sizeof (uint32_t) *
                        nwports_brdcast_repository_entry_p->no_of_nwside_br_ports)));
  
  CNTLR_RCU_READ_LOCK_TAKE(); 

  retval = tsc_nvm_modules[nw_type].nvm_module_get_broadcast_nwports(
               nwports_brdcast_repository_entry_p->local_switch_name_p,
               nwports_brdcast_repository_entry_p->serviceport,
               nwports_brdcast_repository_entry_p->nid,
               &(nwports_brdcast_repository_entry_p->no_of_nwside_br_ports),
               nwports_brdcast_repository_entry_p->nwside_br_ports_p,0); /* we need all ports */
   
  if(retval == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get broadcast nw params");
    if(nwports_brdcast_repository_entry_p->nwside_br_ports_p != NULL )
      free(nwports_brdcast_repository_entry_p->nwside_br_ports_p);
    if(nwports_brdcast_repository_entry_p != NULL)
      free(nwports_brdcast_repository_entry_p);
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }

  *(nwports_brdcast_repository_node_p_p) = nwports_brdcast_repository_entry_p;
  CNTLR_RCU_READ_LOCK_RELEASE();
  return OF_SUCCESS;
}
/*******************************************************************************************************
Function Name: tsc_register_nvm_module
*******************************************************************************************************/
int32_t tsc_register_nvm_module(struct tsc_nvm_module_info*       nvm_module_info_p,
                                struct tsc_nvm_module_interface*  nvm_module_interface_p)

{
  int32_t retval = OF_SUCCESS;

  do
  {
    if((nvm_module_info_p == NULL) || (nvm_module_interface_p == NULL))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Invalid input");
      retval = OF_FAILURE;
      break;
    }

    if(nvm_module_info_p->nvm_module_name == NULL) 
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"NVM_MODULE_NAME is either not specified");
      retval = OF_FAILURE;
      break;
    }

    if((nvm_module_info_p->nw_type < SUPPORTED_NW_TYPE_MIN) || (nvm_module_info_p->nw_type > SUPPORTED_NW_TYPE_MAX))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"NW_TYPE is not supported");
      retval = OF_FAILURE;
      break;
    }

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NVM MODULE NAME = %s",nvm_module_info_p->nvm_module_name);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NW TYPE = %d",nvm_module_info_p->nw_type);

    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_init = nvm_module_interface_p->nvm_module_init;
    
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_create_vmport_views        =  nvm_module_interface_p->nvm_module_create_vmport_views;  
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_delete_vmport_views        =  nvm_module_interface_p->nvm_module_delete_vmport_views;  
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_create_nwport_views        =  nvm_module_interface_p->nvm_module_create_nwport_views;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_delete_nwport_views        =  nvm_module_interface_p->nvm_module_delete_nwport_views;
 
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_get_unicast_nwport         =  nvm_module_interface_p->nvm_module_get_unicast_nwport;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_get_remote_ips             =  nvm_module_interface_p->nvm_module_get_remote_ips;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_get_broadcast_nwports      =  nvm_module_interface_p->nvm_module_get_broadcast_nwports;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_get_vmport_by_dmac_nid     =  nvm_module_interface_p->nvm_module_get_vmport_by_dmac_nid;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_get_vmports_by_swname_nid  =  nvm_module_interface_p->nvm_module_get_vmports_by_swname_nid;

    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_add_vn_to_nid_view         =  nvm_module_interface_p->nvm_module_add_vn_to_nid_view;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_delete_vn_from_nid_view    =  nvm_module_interface_p->nvm_module_delete_vn_from_nid_view;
    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_get_vnhandle_by_nid        =  nvm_module_interface_p->nvm_module_get_vnhandle_by_nid;

    tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_construct_metadata         =  nvm_module_interface_p->nvm_module_construct_metadata;

    retval = tsc_nvm_modules[nvm_module_info_p->nw_type].nvm_module_init();    
    return retval;
  }while(0);
  
  if(retval == OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully Registered NVM Modules");
  }
  return retval;
}
/*********************************************************************************************************************************************/
void tsc_barrier_reply_notification_fn(uint64_t  controller_handle,
                                       uint64_t  domain_handle,
                                       uint64_t  datapath_handle,
                                       struct    of_msg *msg,
                                       void     *cbk_arg1,
                                       void     *cbk_arg2,
                                       uint8_t   status)
{
   int32_t retval = OF_FAILURE;
   struct table_barrier_msg* table_barrier;

   table_barrier = (struct table_barrier_msg*) cbk_arg1;
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dp_handle : %llx",table_barrier->dp_handle);
  
   if(status == OF_RESPONSE_STATUS_SUCCESS)
   {
     retval = tsc_send_pktout_1_3_msg(table_barrier->dp_handle,table_barrier->in_port,
                                      table_barrier->packet,table_barrier->packet_length); 
     if(retval != OF_FAILURE)
     {   
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Sent pkt out through callback in barrier wait");
     }                    
     else 
     {     
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Packet out message through callback in barrier wait");
       printf("\r\n TSC Barrier callback:Failed to send Packet out message through callback in barrier wait");
       return;
     }  
   }
   else
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"status is not OF_RESPONSE_STATUS_SUCCESS- Packet out not sent");
     printf("\r\n TSC Barrier callback:status is not OF_RESPONSE_STATUS_SUCCESS- Packet out not sent");
   }

   if(table_barrier->msg != NULL)
     (table_barrier->msg)->desc.free_cbk(table_barrier->msg);
   free(table_barrier);
}
/********************************************************************************************************************************************/ 
