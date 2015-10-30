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

/* File: tsc_nsc_iface_zone_left.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file implements zone based interface to service chaining core module.
 */

#include "controller.h"
#include "tsc_controller.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_nsc_iface.h"
#include "tsc_nvm_modules.h"
#include "nsrm.h"
#include "nsrmldef.h"
#include "tsc_ofv1_3_plugin.h"

extern uint32_t vn_nsc_info_offset_g;   /* offset for nsc info in vn structure in bytes */
extern struct   tsc_nvm_module_interface tsc_nvm_modules[SUPPORTED_NW_TYPE_MAX + 1];

extern   uint32_t  vn_nsc_info_offset_g;
extern   uint32_t  nsc_no_of_repository_table_1_buckets_g;
extern   uint32_t  nsc_no_of_repository_table_2_buckets_g;
extern   uint32_t  nsc_no_of_repository_table_3_buckets_g;

int32_t tsc_nsc_send_proactive_flow_zone_left(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                              struct    tsa_all_flows_members* tsc_all_flows_p,
                                              struct    nwservice_instance *nw_service_p,
                                              struct    nschain_repository_entry** repository_entry_1_p);

int32_t tsc_nsc_send_proactive_flow_table_3_zone_left(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                                      struct    nwservice_instance *nw_service_p,
                                                      uint8_t   use_dstmac,
                                                      uint32_t  nid);
/*****************************************************************************************************************************************/
/* Zone specific.Primary->ZONE_LEFT,Secondary->ZONE_RIGHT. May contain both 1-port and 2-port S-VMs. */
int32_t tsc_send_all_flows_to_all_tsas_zone_left(struct nschain_repository_entry* nschain_repository_entry_p)
{
  uint8_t  nw_type,no_of_nwservice_instances_1,no_of_nwservice_instances_2;
  uint8_t  use_dstmac;
  uint16_t serviceport;
  uint32_t no_of_nwbr_ports;
  uint32_t nid,retval = OF_SUCCESS;

  struct   nw_services_to_apply  *nw_services_p;
  struct   nw_services_to_apply  *first_service_sel_1_p,*first_service_sel_2_p;
  struct   nw_services_to_apply  *next_service_sel_1_p,*next_service_sel_2_p;  
  struct   nwservice_instance    *first_nwservice_sel_1_instance_p,*next_nwservice_sel_1_instance_p;
  struct   nwservice_instance    *first_nwservice_sel_2_instance_p,*next_nwservice_sel_2_instance_p;
  struct   l2_connection_to_nsinfo_bind_node* cn_bind_entry_p; 
  struct   nschain_repository_entry* repository_entry_1_p;
  struct   tsa_all_flows_members  all_flows;
  struct   tsa_all_flows_members* tsc_all_flows_p;

  tsc_all_flows_p = &all_flows;    

  /* For zone based chains,mac addresses are not changed.VLAN-IDs are not added to packets sent to 2 port S-VMs. */ 
  tsc_all_flows_p->copy_local_mac_addresses_b     = FALSE;
  tsc_all_flows_p->copy_original_mac_addresses_b  = FALSE;

  /* Process 2 consecutive services in a loop. Send two flows at a time */

  CNTLR_RCU_READ_LOCK_TAKE();

  if((nschain_repository_entry_p->skip_this_entry == 1) || (nschain_repository_entry_p->vm_and_service1_not_on_same_switch == 2))
  {
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    printf("\r\n PPH-50"); 
    return OF_SUCCESS;
  }
  cn_bind_entry_p = nschain_repository_entry_p->selector.cnbind_node_p;

  nw_type      = nschain_repository_entry_p->nw_type;
  nid          = nschain_repository_entry_p->nid;         /* introduced for table_3 */   
  serviceport  = nschain_repository_entry_p->serviceport;

  if(cn_bind_entry_p == NULL) 
  {
    printf("\r\n cn_bind_entry is NULL");
    CNTLR_RCU_READ_LOCK_RELEASE();
    printf("\r\n PPH-52");
    return OF_SUCCESS;
  }
   
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"cn_bind_entry   = %x",cn_bind_entry_p); 
  no_of_nwservice_instances_1  = (cn_bind_entry_p->nwservice_info).no_of_nwservice_instances;
  no_of_nwservice_instances_2  = no_of_nwservice_instances_1; 
  nw_services_p = (struct  nw_services_to_apply *)(cn_bind_entry_p->nwservice_info.nw_services_p);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"no_of_nwservice_instances = %d",no_of_nwservice_instances_1);
 
  printf("\r\nNS= %d",no_of_nwservice_instances_1);
  if(no_of_nwservice_instances_1 ==  0)
  { 
    CNTLR_RCU_READ_LOCK_RELEASE();
    printf("\r\n PPH-53");
    return OF_FAILURE;
  }
  
  first_service_sel_1_p   = nw_services_p; /* Assumed that first service is present */
  first_service_sel_2_p   = (struct nw_services_to_apply*)(nw_services_p + (no_of_nwservice_instances_1 - 1)); 

  if(nschain_repository_entry_p->selector.selector_type == SELECTOR_SECONDARY)
  {
    retval = OF_SUCCESS;
    if(nschain_repository_entry_p->vm_and_service1_not_on_same_switch == 1)
    {
      //printf("NOT-SSW-SEC");
      nschain_repository_entry_p->vm_and_service1_not_on_same_switch = 2;

      retval = nsrm_get_nwservice_instance_byhandle(first_service_sel_2_p->nschain_object_handle,
                                                    first_service_sel_2_p->nwservice_object_handle,
                                                    first_service_sel_2_p->nwservice_instance_handle,
                                                    &first_nwservice_sel_2_instance_p);

      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the first service instance for secondary selector.");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-101");
        return retval; /* Packet miss logic is any way in place */
      }
      /* Get IP of the first switch */
      retval = dprm_get_switch_data_ip(nschain_repository_entry_p->local_switch_name_p,&(tsc_all_flows_p->remote_ip));
      if(retval != DPRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-102");
        return OF_FAILURE;
      }
      no_of_nwbr_ports = 1;
      retval = tsc_nvm_modules[nw_type].nvm_module_get_broadcast_nwports(first_nwservice_sel_2_instance_p->switch_name,
                                                                         serviceport,
                                                                         0,  /* nss vxlan_nw_p->nid */
                                                                         &no_of_nwbr_ports,
                                                                         &(tsc_all_flows_p->inport_id),
                                                                         tsc_all_flows_p->remote_ip  /* one port specific to remote ip */
                                                                        );

      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"vxlan broadcast ports not found");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-103");
        return OF_FAILURE;
      }
     
      tsc_all_flows_p->sel_type_swap_e = SEL_SECONDARY_NOSWAP;
      tsc_all_flows_p->table_no        = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
      tsc_all_flows_p->pkt_origin      = NW_BROADCAST_SIDE;
      tsc_all_flows_p->dp_handle       = first_nwservice_sel_2_instance_p->system_br_saferef;
      tsc_all_flows_p->nw_port_b       = FALSE;

      /* out network is not used for this flow */
      if(first_nwservice_sel_2_instance_p->no_of_ports == 1)
      {
        tsc_all_flows_p->out_port_no   = first_nwservice_sel_2_instance_p->port1_id;
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->next_vlan_id  = first_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_out;
      }
      else /* 2-port */
      {
        tsc_all_flows_p->out_port_no   = first_nwservice_sel_2_instance_p->port2_id;
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->next_vlan_id  = 0;
        tsc_all_flows_p->vlan_id_pkt   = 0;

        nschain_repository_entry_p->nw_type = first_nwservice_sel_2_instance_p->outport_nw_type;
        nschain_repository_entry_p->nid     = first_nwservice_sel_2_instance_p->outport_nw_id;
      }
      /* F9 Flow */
      retval = tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                                     tsc_all_flows_p,
                                                     first_nwservice_sel_2_instance_p,
                                                     &repository_entry_1_p);

    }
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }

  if(nschain_repository_entry_p->vm_and_service1_not_on_same_switch == 1)
  {
      //printf("NOT-SSW-PRI");
      nschain_repository_entry_p->vm_and_service1_not_on_same_switch = 2;

      retval = nsrm_get_nwservice_instance_byhandle(first_service_sel_1_p->nschain_object_handle,
                                                    first_service_sel_1_p->nwservice_object_handle,
                                                    first_service_sel_1_p->nwservice_instance_handle,
                                                    &first_nwservice_sel_1_instance_p);

      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the first service instance for primary selector.");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-01");
        return retval; /* Packet miss logic is any way in place */
      }
  
      /* Get IP of the first switch */
      retval = dprm_get_switch_data_ip(nschain_repository_entry_p->local_switch_name_p,&(tsc_all_flows_p->remote_ip));
      if(retval != DPRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-02");
        return OF_FAILURE;
      }
      no_of_nwbr_ports = 1;
      retval = tsc_nvm_modules[nw_type].nvm_module_get_broadcast_nwports(first_nwservice_sel_1_instance_p->switch_name,
                                                                         serviceport,
                                                                         0,  /* nss vxlan_nw_p->nid */
                                                                         &no_of_nwbr_ports,
                                                                         &(tsc_all_flows_p->inport_id),
                                                                         tsc_all_flows_p->remote_ip  /* one port specific to remote ip */
                                                                        );

      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"vxlan broadcast ports not found");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-03");
        return OF_FAILURE;
      }
      tsc_all_flows_p->sel_type_swap_e = SEL_PRIMARY;
      tsc_all_flows_p->table_no        = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
      tsc_all_flows_p->pkt_origin      = NW_BROADCAST_SIDE;  
      tsc_all_flows_p->dp_handle       = first_nwservice_sel_1_instance_p->system_br_saferef;
      tsc_all_flows_p->nw_port_b       = FALSE;
      
      /* out network is not used for this flow */ 
      if(first_nwservice_sel_1_instance_p->no_of_ports == 1)
      {    
        tsc_all_flows_p->out_port_no   = first_nwservice_sel_1_instance_p->port1_id; 
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->next_vlan_id  = first_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_in;
      }  
      else /* 2-port */
      {
        tsc_all_flows_p->out_port_no   = first_nwservice_sel_1_instance_p->port1_id;   
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_in; 
        tsc_all_flows_p->next_vlan_id  = 0;
        tsc_all_flows_p->vlan_id_pkt   = 0;

        nschain_repository_entry_p->nw_type = first_nwservice_sel_1_instance_p->inport_nw_type;
        nschain_repository_entry_p->nid     = first_nwservice_sel_1_instance_p->inport_nw_id;
      }
      /* F9 Flow */   
      retval = tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                                     tsc_all_flows_p,               
                                                     first_nwservice_sel_1_instance_p,
                                                     &repository_entry_1_p);
      
      if(retval == OF_SUCCESS)
        nschain_repository_entry_p = repository_entry_1_p;
      /* Before entering while loop send flow F9 using Table 2 for first_service for primary selector. */
  }
  else
  {    
    retval = nsrm_get_nwservice_instance_byhandle(first_service_sel_1_p->nschain_object_handle,
                                                  first_service_sel_1_p->nwservice_object_handle,
                                                  first_service_sel_1_p->nwservice_instance_handle,
                                                  &first_nwservice_sel_1_instance_p);

    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the first service instance for primary selector.");
      CNTLR_RCU_READ_LOCK_RELEASE();
      printf("\r\n PPH-04");
      return retval; /* Packet miss logic is any way in place */
    }
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"first network service instance name = %s",first_nwservice_sel_1_instance_p->name_p); 
  }
  while(1)
  {
    if(--no_of_nwservice_instances_1) 
    {
      next_service_sel_1_p  = first_service_sel_1_p + 1;
    }
    else
    {
      next_service_sel_1_p = NULL;
      break;
    }  

    if(next_service_sel_1_p)  /* atleast two services are present */
    {
      retval = nsrm_get_nwservice_instance_byhandle(next_service_sel_1_p->nschain_object_handle,
                                                    next_service_sel_1_p->nwservice_object_handle,
                                                    next_service_sel_1_p->nwservice_instance_handle,
                                                    &next_nwservice_sel_1_instance_p);

      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the next service instance.");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-05");
        return retval; /* Packet miss logic is any way in place */
      }
      
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"next network service instance name = %s",next_nwservice_sel_1_instance_p->name_p);

      if(strcmp(first_nwservice_sel_1_instance_p->switch_name,next_nwservice_sel_1_instance_p->switch_name))
      {
        /* Two services are on different switches. Push F3 using table 1 on first switch and F4 using table 2 on next switch. */   
        /* Their dp_hanldes are obtained from their nw_instances */

        tsc_all_flows_p->sel_type_swap_e = SEL_PRIMARY;
        tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->dp_handle       = first_nwservice_sel_1_instance_p->system_br_saferef; 
        tsc_all_flows_p->nw_port_b       = TRUE;    

        if(first_nwservice_sel_1_instance_p->no_of_ports == 1) 
        {
          tsc_all_flows_p->pkt_origin    = VM_NS;  /* F3 Flow */  
          tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_out;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_out;
          
          nschain_repository_entry_p->out_nw_type = next_nwservice_sel_1_instance_p->inport_nw_type;
          nschain_repository_entry_p->out_nid     = next_nwservice_sel_1_instance_p->inport_nw_id;
        }    
        else /* 2-port */
        {
          tsc_all_flows_p->pkt_origin    = VM_NS_OUT;  /* F3 Flow */
          tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port2_id;
          tsc_all_flows_p->match_vlan_id = 0; /* Don't add match field */
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->vlan_id_pkt   = 0;

          nschain_repository_entry_p->nw_type = first_nwservice_sel_1_instance_p->outport_nw_type;
          nschain_repository_entry_p->nid     = first_nwservice_sel_1_instance_p->outport_nw_id;
       
          nschain_repository_entry_p->out_nw_type = next_nwservice_sel_1_instance_p->inport_nw_type;
          nschain_repository_entry_p->out_nid     = next_nwservice_sel_1_instance_p->inport_nw_id;
        }
        retval = tsc_nvm_modules[nw_type].nvm_module_get_unicast_nwport(first_nwservice_sel_1_instance_p->switch_name,
                                                                        serviceport,
                                                                        &(tsc_all_flows_p->out_port_no));
        if(retval == OF_FAILURE)
        {
          printf("unable to obtain unicast port for proactive flow pushing");
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"unable to obtain unicast port for proactive flow pushing");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-06");
          return OF_FAILURE;
        }
        /* Get IP of the remote switch */
        retval = dprm_get_switch_data_ip(next_nwservice_sel_1_instance_p->switch_name,&(tsc_all_flows_p->remote_ip));
        if(retval != DPRM_SUCCESS)
        {
          printf("Invalid remote switch name. unable to get remoteip for proactive pushing");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-07");
          return OF_FAILURE;
        }

        /* F3 Flow */   
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p,
                                              first_nwservice_sel_1_instance_p,
                                              &repository_entry_1_p);

       /* send F4 flow as if it is received on broadcast port */
        
        /* Get IP of the first switch */
        retval = dprm_get_switch_data_ip(first_nwservice_sel_1_instance_p->switch_name,&(tsc_all_flows_p->remote_ip));
        if(retval != DPRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-08");
          return OF_FAILURE;
        }
        no_of_nwbr_ports = 1;
        retval = tsc_nvm_modules[nw_type].nvm_module_get_broadcast_nwports(next_nwservice_sel_1_instance_p->switch_name,
                                                                           serviceport,
                                                                           0,  /* nss vxlan_nw_p->nid */
                                                                           &no_of_nwbr_ports,
                                                                           &(tsc_all_flows_p->inport_id),
                                                                           (tsc_all_flows_p->remote_ip)  /* one port specific to remote ip */
                                                                          );

        if(retval != OF_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"vxlan broadcast ports not found");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-09");
          return OF_FAILURE;
        }
        tsc_all_flows_p->sel_type_swap_e = SEL_PRIMARY;
        tsc_all_flows_p->table_no        = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
        tsc_all_flows_p->pkt_origin      = NW_BROADCAST_SIDE; 
        tsc_all_flows_p->dp_handle       = next_nwservice_sel_1_instance_p->system_br_saferef;
        tsc_all_flows_p->nw_port_b       = FALSE;

        /* out network is not used for this flow */ 
        if(next_nwservice_sel_1_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_1_instance_p->vlan_id_in;
        }
        else /* 2-port */
        {
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->next_vlan_id  = 0;
          tsc_all_flows_p->vlan_id_pkt   = 0;

          nschain_repository_entry_p->nw_type = next_nwservice_sel_1_instance_p->inport_nw_type; 
          nschain_repository_entry_p->nid     = next_nwservice_sel_1_instance_p->inport_nw_id;   
        }
        /* F4 Flow */ 
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p,
                                              next_nwservice_sel_1_instance_p,
                                              &repository_entry_1_p);
      }
      else
      {
        /* Two services are on same switch. Push two flows F1 and F2 to table 1  */ 
        /* Both the services contain the same dp_handle as br-int bridge is used */
        
        tsc_all_flows_p->sel_type_swap_e = SEL_PRIMARY;
        tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->dp_handle       = first_nwservice_sel_1_instance_p->system_br_saferef; 
        tsc_all_flows_p->nw_port_b       = FALSE;

        if(first_nwservice_sel_1_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->pkt_origin    = VM_NS;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port1_id; 
          tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_out;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_out;
        } 
        else
        {
          tsc_all_flows_p->pkt_origin    = VM_NS_OUT;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port2_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port1_id;  
          tsc_all_flows_p->match_vlan_id = 0;
          tsc_all_flows_p->next_vlan_id  = 0;
          tsc_all_flows_p->vlan_id_pkt   = 0;
          
          nschain_repository_entry_p->nw_type = first_nwservice_sel_1_instance_p->outport_nw_type;
          nschain_repository_entry_p->nid     = first_nwservice_sel_1_instance_p->outport_nw_id;
        }
        /* F1 Flow */  
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p, 
                                              tsc_all_flows_p,
                                              first_nwservice_sel_1_instance_p,
                                              &repository_entry_1_p);

        tsc_all_flows_p->sel_type_swap_e = SEL_PRIMARY;
        tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->dp_handle       = first_nwservice_sel_1_instance_p->system_br_saferef;
        tsc_all_flows_p->nw_port_b       = FALSE;

        /* out network is not used for this flow */
        if(next_nwservice_sel_1_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->pkt_origin    = VM_NS;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
          tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_1_instance_p->vlan_id_in;
        }
        else /* 2-port */
        {
          tsc_all_flows_p->pkt_origin    = VM_NS_OUT;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port2_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = 0;
          tsc_all_flows_p->next_vlan_id  = 0;
          tsc_all_flows_p->vlan_id_pkt   = 0;

          nschain_repository_entry_p->nw_type = next_nwservice_sel_1_instance_p->inport_nw_type;
          nschain_repository_entry_p->nid     = next_nwservice_sel_1_instance_p->inport_nw_id;
        }
        /* TBD F2 Flow Is it required. F1 may be enough. */
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p,
                                              next_nwservice_sel_1_instance_p,
                                              &repository_entry_1_p);

      }   
      first_nwservice_sel_1_instance_p = next_nwservice_sel_1_instance_p;
      first_service_sel_1_p = next_service_sel_1_p;
    }
    else
     break;
  }/* while loop */  
  
  if(next_service_sel_1_p == NULL) /* Last Service */
  {
    /*Using first_service push flow F6 on Table 1 so that it goes to table 3 in the same switch as there are no more services. */
    /* To be done for both sel_1 and sel_2 as the number of services is same */
    tsc_all_flows_p->sel_type_swap_e = SEL_PRIMARY;
    tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
    tsc_all_flows_p->dp_handle       = first_nwservice_sel_1_instance_p->system_br_saferef;
    tsc_all_flows_p->out_port_no     = 0;
    tsc_all_flows_p->nw_port_b       = FALSE;
    tsc_all_flows_p->nid_vm          = nid;
    if(first_nwservice_sel_1_instance_p->no_of_ports == 1)
    {
      tsc_all_flows_p->pkt_origin    = VM_NS;
      tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port1_id;
      tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_out;
      tsc_all_flows_p->next_vlan_id  = 0;
      tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_out;
    }
    else /* 2-port */
    {
      tsc_all_flows_p->pkt_origin    = VM_NS_OUT;
      tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port2_id;
      tsc_all_flows_p->match_vlan_id = 0;
      tsc_all_flows_p->next_vlan_id  = 0;
      tsc_all_flows_p->vlan_id_pkt   = 0;
      
      nschain_repository_entry_p->nw_type = first_nwservice_sel_1_instance_p->outport_nw_type;
      nschain_repository_entry_p->nid     = first_nwservice_sel_1_instance_p->outport_nw_id;

      /* Going to VM */
      nschain_repository_entry_p->out_nw_type = first_nwservice_sel_1_instance_p->inport_nw_type;
      nschain_repository_entry_p->out_nid     = first_nwservice_sel_1_instance_p->inport_nw_id;
    }
    /* F6 Flow */
    tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                          tsc_all_flows_p,
                                          first_nwservice_sel_1_instance_p,
                                          &repository_entry_1_p);
  
    //nschain_repository_entry_p->nw_type = nw_type;  /* VM values */
    //nschain_repository_entry_p->nid     = nid;

    use_dstmac = TRUE;

    tsc_nsc_send_proactive_flow_table_3_zone_left(nschain_repository_entry_p,
                                                  first_nwservice_sel_1_instance_p,
                                                  use_dstmac,nid);
#if 0
    use_dstmac = FALSE;
    tsc_nsc_send_proactive_flow_table_3_zone_left(nschain_repository_entry_p,
                                                  first_nwservice_sel_1_instance_p,
                                                  use_dstmac,nid);
#endif                                           
  }

  retval = nsrm_get_nwservice_instance_byhandle(first_service_sel_2_p->nschain_object_handle,
                                                first_service_sel_2_p->nwservice_object_handle,
                                                first_service_sel_2_p->nwservice_instance_handle,
                                                &first_nwservice_sel_2_instance_p);
  if(retval == OF_FAILURE)
  {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the first service instance for secondary selector.");
      CNTLR_RCU_READ_LOCK_RELEASE();
      printf("\r\n PPH-10");
      return retval; /* Packet miss logic is any way in place */
  }
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"first network service instance name = %s",first_nwservice_sel_2_instance_p->name_p);

  while(1)
  {
    if(--no_of_nwservice_instances_2)
    {
      next_service_sel_2_p  = first_service_sel_2_p - 1;
    }
    else
    {
      next_service_sel_2_p = NULL;
      break;
    }
 
    if(next_service_sel_2_p)  /* atleast two services are present */
    {

      retval = nsrm_get_nwservice_instance_byhandle(next_service_sel_2_p->nschain_object_handle,
                                                    next_service_sel_2_p->nwservice_object_handle,
                                                    next_service_sel_2_p->nwservice_instance_handle,
                                                    &next_nwservice_sel_2_instance_p);

      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the next service instance.");
        CNTLR_RCU_READ_LOCK_RELEASE();
        printf("\r\n PPH-11");
        return retval; /* Packet miss logic is any way in place */
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"next network service instance name = %s",next_nwservice_sel_2_instance_p->name_p);

      if(strcmp(first_nwservice_sel_2_instance_p->switch_name,next_nwservice_sel_2_instance_p->switch_name))
      {
        /* Selector 2 is used for addding responses.*/ 
        /* Two services are on different switches. Push F3 using table 1 on first switch and F4 using table 2 on next switch. */
        /* Their dp_hanldes are obtained from their nw_instances */
      
        tsc_all_flows_p->sel_type_swap_e = SEL_SECONDARY;
        tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->dp_handle       = first_nwservice_sel_2_instance_p->system_br_saferef;
        tsc_all_flows_p->nw_port_b       = TRUE;
        
        if(first_nwservice_sel_2_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->pkt_origin    = VM_NS;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_in;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_in;
      
          nschain_repository_entry_p->out_nw_type = next_nwservice_sel_2_instance_p->inport_nw_type;
          nschain_repository_entry_p->out_nid     = next_nwservice_sel_2_instance_p->inport_nw_id;
        }
        else /* 2-port */
        {
          tsc_all_flows_p->pkt_origin    = VM_NS_IN;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = 0; /* Don't add match field */
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->vlan_id_pkt   = 0;
          
          nschain_repository_entry_p->nw_type = first_nwservice_sel_2_instance_p->inport_nw_type;
          nschain_repository_entry_p->nid     = first_nwservice_sel_2_instance_p->inport_nw_id;

          nschain_repository_entry_p->out_nw_type = next_nwservice_sel_2_instance_p->outport_nw_type;
          nschain_repository_entry_p->out_nid     = next_nwservice_sel_2_instance_p->outport_nw_id;
        }
        
        retval = tsc_nvm_modules[nw_type].nvm_module_get_unicast_nwport(first_nwservice_sel_2_instance_p->switch_name,
                                                                        serviceport,
                                                                        &(tsc_all_flows_p->out_port_no));
        if(retval == OF_FAILURE)
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"unable to obtain unicast port for proactive flow pushing");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-12");
          return OF_FAILURE;
        }
        /* Get IP of the remote switch */
        retval = dprm_get_switch_data_ip(next_nwservice_sel_2_instance_p->switch_name,&(tsc_all_flows_p->remote_ip));
        if(retval != DPRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-13");
          return OF_FAILURE;
        }

        /* F3 Flow */        
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p,
                                              first_nwservice_sel_2_instance_p,
                                              &repository_entry_1_p);
        
        /* Get IP of the remote switch */
        retval = dprm_get_switch_data_ip(next_nwservice_sel_2_instance_p->switch_name,&(tsc_all_flows_p->remote_ip));
        if(retval != DPRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-14");
          return OF_FAILURE;
        }
        no_of_nwbr_ports = 1;
        retval = tsc_nvm_modules[nw_type].nvm_module_get_broadcast_nwports(first_nwservice_sel_2_instance_p->switch_name,
                serviceport,
                0,  /* nss vxlan_nw_p->nid */
                &no_of_nwbr_ports,
                &(tsc_all_flows_p->inport_id),
                (tsc_all_flows_p->remote_ip)  /* one port specific to remote ip */
                );

        if(retval != OF_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"vxlan broadcast ports not found");
          CNTLR_RCU_READ_LOCK_RELEASE();
          printf("\r\n PPH-15");
          return OF_FAILURE;
        }

        tsc_all_flows_p->sel_type_swap_e = SEL_SECONDARY;
        tsc_all_flows_p->table_no        = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
        tsc_all_flows_p->pkt_origin      = NW_BROADCAST_SIDE;
        tsc_all_flows_p->dp_handle       = next_nwservice_sel_2_instance_p->system_br_saferef;
        tsc_all_flows_p->nw_port_b       = FALSE;
       
        if(next_nwservice_sel_2_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_2_instance_p->vlan_id_out;
        }
        else /* 2-port */
        {
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port2_id;
          tsc_all_flows_p->match_vlan_id = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->next_vlan_id  = 0;
          tsc_all_flows_p->vlan_id_pkt   = 0;

          nschain_repository_entry_p->nw_type = next_nwservice_sel_2_instance_p->outport_nw_type;
          nschain_repository_entry_p->nid     = next_nwservice_sel_2_instance_p->outport_nw_id;
        }
        /* F4 Flow */         
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p,
                                              next_nwservice_sel_2_instance_p,
                                              &repository_entry_1_p);
      }
      else
      {
        /* Two services are on same switch. Push two flows F1 to table 1 */
        /* Both the services contain the same dp_handle as br-int bridge is used */
        /* first_nwservice_instance_p->system_br_saferef */
      
        tsc_all_flows_p->sel_type_swap_e = SEL_SECONDARY;
        tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->dp_handle       = first_nwservice_sel_2_instance_p->system_br_saferef;    
        tsc_all_flows_p->nw_port_b       = FALSE;

        if(first_nwservice_sel_2_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->pkt_origin    = VM_NS;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_in;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_in;
        }
        else
        {
          tsc_all_flows_p->pkt_origin    = VM_NS_IN; 
          tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port2_id;
          tsc_all_flows_p->match_vlan_id = 0;
          tsc_all_flows_p->next_vlan_id  = 0;
          tsc_all_flows_p->vlan_id_pkt   = 0;

          nschain_repository_entry_p->nw_type = first_nwservice_sel_2_instance_p->inport_nw_type;
          nschain_repository_entry_p->nid     = first_nwservice_sel_2_instance_p->inport_nw_id;
        }
        /* F1 Flow */  
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p, 
                                              first_nwservice_sel_2_instance_p,
                                              &repository_entry_1_p);
      
        tsc_all_flows_p->sel_type_swap_e = SEL_SECONDARY;
        tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->dp_handle       = first_nwservice_sel_2_instance_p->system_br_saferef;
        tsc_all_flows_p->nw_port_b       = FALSE;

        if(next_nwservice_sel_2_instance_p->no_of_ports == 1)
        {
          tsc_all_flows_p->pkt_origin    = VM_NS; 
          tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->match_vlan_id = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
          tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_2_instance_p->vlan_id_out;
        }
        else /* 2-port */
        {
          tsc_all_flows_p->pkt_origin    = VM_NS_IN;
          tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
          tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port2_id;
          tsc_all_flows_p->match_vlan_id = 0;
          tsc_all_flows_p->next_vlan_id  = 0;
          tsc_all_flows_p->vlan_id_pkt   = 0;
    
          nschain_repository_entry_p->nw_type = next_nwservice_sel_2_instance_p->outport_nw_type;
          nschain_repository_entry_p->nid     = next_nwservice_sel_2_instance_p->outport_nw_id;
        }

        /* F2 Flow It may not be required and F1 is sufficient.*/
        tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                              tsc_all_flows_p,
                                              next_nwservice_sel_2_instance_p,
                                              &repository_entry_1_p);

      }
      first_nwservice_sel_2_instance_p = next_nwservice_sel_2_instance_p;   
      first_service_sel_2_p = next_service_sel_2_p;
    }
    else
     break;
  }/* while loop */  
    
  if(next_service_sel_2_p == NULL)  /* Last Service */ 
  {
    /*Using first_service push flow F6 on Table 1 so that it goes to table 3 in the same switch as there are no more services. */ 
    /* To be done for both sel_1 and sel_2 as the number of services is same */

    tsc_all_flows_p->sel_type_swap_e = SEL_SECONDARY;
    tsc_all_flows_p->table_no        = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
    tsc_all_flows_p->dp_handle       = first_nwservice_sel_2_instance_p->system_br_saferef;  
    tsc_all_flows_p->out_port_no     = 0;
    tsc_all_flows_p->nw_port_b       = FALSE;
    tsc_all_flows_p->nid_vm          = nid;
    if(first_nwservice_sel_2_instance_p->no_of_ports == 1)
    {
      tsc_all_flows_p->pkt_origin    = VM_NS;
      tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
      tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_in;
      tsc_all_flows_p->next_vlan_id  = 0;
      tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_in;
    }
    else /* 2-port */
    {
      tsc_all_flows_p->pkt_origin    = VM_NS_IN;
      tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port1_id;
      tsc_all_flows_p->match_vlan_id = 0;
      tsc_all_flows_p->next_vlan_id  = 0;
      tsc_all_flows_p->vlan_id_pkt   = 0;

      nschain_repository_entry_p->nw_type = first_nwservice_sel_2_instance_p->inport_nw_type;
      nschain_repository_entry_p->nid     = first_nwservice_sel_2_instance_p->inport_nw_id;
  
      /* Going to VM */
      nschain_repository_entry_p->out_nw_type = first_nwservice_sel_2_instance_p->inport_nw_type;
      nschain_repository_entry_p->out_nid     = first_nwservice_sel_2_instance_p->inport_nw_id;
    }
    tsc_nsc_send_proactive_flow_zone_left(nschain_repository_entry_p,
                                          tsc_all_flows_p,
                                          first_nwservice_sel_2_instance_p,
                                          &repository_entry_1_p);
    /* Add a flow to Table 3 */

     //nschain_repository_entry_p->nw_type = nw_type;  /* VM values */
     //nschain_repository_entry_p->nid     = nid;

#if 0     
     use_dstmac = TRUE;
     tsc_nsc_send_proactive_flow_table_3_zone_left(nschain_repository_entry_p,
                                                   first_nwservice_sel_2_instance_p,
                                                   use_dstmac,nid);
#endif

     use_dstmac = FALSE;

     tsc_nsc_send_proactive_flow_table_3_zone_left(nschain_repository_entry_p,
                                                   first_nwservice_sel_2_instance_p,
                                                   use_dstmac,nid);
  }    
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
}

int32_t tsc_nsc_send_proactive_flow_zone_left(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                              struct    tsa_all_flows_members* tsc_flow_p,
                                              struct    nwservice_instance *nw_service_p,
                                              struct    nschain_repository_entry** repository_entry_1_p)   
{    
  uint8_t  nw_type,port_type,heap_b;
  uint8_t  status = FALSE;
  uint32_t nid,hashkey,hashmask,offset,index,magic,ii;
  int32_t  retval = OF_SUCCESS;
  struct   nschain_repository_entry *nsc_repository_entry_p,*nsc_repository_scan_node_p;
  uint64_t  vn_handle,crm_port_handle;
  struct    mchash_table*  nsc_repository_table_p;
  void*     nsc_repository_mempool_g;
  uchar8_t* hashobj_p = NULL;
  struct    vn_service_chaining_info* vn_nsc_info_p;
  struct    crm_virtual_network*      vn_entry_p;
  struct    crm_port* dst_crm_port_p;

  nw_type = (nschain_repository_entry_first_p->nw_type);
  nid     = (nschain_repository_entry_first_p->nid);

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    printf("\r\n PPH-16");
    return OF_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    printf("\r\n PPH-17");
    return OF_FAILURE;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
  vn_nsc_info_p = vn_nsc_info_p->vn_nsc_info_p;
  if(vn_nsc_info_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"vn_nsc_info_p is NULL");
    return OF_FAILURE;
  }    
  if(tsc_flow_p->table_no == TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1)
    nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_1_mempool_g; 
  else if(tsc_flow_p->table_no == TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2)
    nsc_repository_mempool_g = vn_nsc_info_p->nsc_repository_table_2_mempool_g;
 
  retval = mempool_get_mem_block(nsc_repository_mempool_g,(uchar8_t **)&nsc_repository_entry_p,&heap_b);
  if(retval != MEMPOOL_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Memory block allocation for nsc repository entry failed");
    printf("\r\n PPH-19");
    return OF_FAILURE;
  }
  nsc_repository_entry_p->heap_b = heap_b;

  nsc_repository_entry_p->nw_type = nw_type;
  nsc_repository_entry_p->nid     = nid;
  nsc_repository_entry_p->out_nw_type = nschain_repository_entry_first_p->out_nw_type;
  nsc_repository_entry_p->out_nid     = nschain_repository_entry_first_p->out_nid;   

  nsc_repository_entry_p->selector.ethernet_type = nschain_repository_entry_first_p->selector.ethernet_type;
  nsc_repository_entry_p->selector.protocol      = nschain_repository_entry_first_p->selector.protocol;

  if(tsc_flow_p->sel_type_swap_e == SEL_PRIMARY)
  {
    nsc_repository_entry_p->selector.selector_type = SELECTOR_PRIMARY;
    nsc_repository_entry_p->selector.src_ip  = nschain_repository_entry_first_p->selector.src_ip;
    nsc_repository_entry_p->selector.dst_ip  = nschain_repository_entry_first_p->selector.dst_ip;

    if(nsc_repository_entry_p->selector.protocol == OF_IPPROTO_ICMP)
    {
      nsc_repository_entry_p->selector.icmp_type = 8;
      nsc_repository_entry_p->selector.icmp_code = 0;
      nsc_repository_entry_p->selector.src_port  = 8;
      nsc_repository_entry_p->selector.dst_port  = 0;
    }
    else    
    {
      nsc_repository_entry_p->selector.src_port = nschain_repository_entry_first_p->selector.src_port;
      nsc_repository_entry_p->selector.dst_port = nschain_repository_entry_first_p->selector.dst_port; 
    }  
  }    
  else if (tsc_flow_p->sel_type_swap_e == SEL_SECONDARY)
  {
    nsc_repository_entry_p->selector.selector_type = SELECTOR_SECONDARY;
    nsc_repository_entry_p->selector.src_ip  = nschain_repository_entry_first_p->selector.dst_ip;
    nsc_repository_entry_p->selector.dst_ip  = nschain_repository_entry_first_p->selector.src_ip;
    
    if(nsc_repository_entry_p->selector.protocol == OF_IPPROTO_ICMP)
    {
      nsc_repository_entry_p->selector.icmp_type = 0;
      nsc_repository_entry_p->selector.icmp_code = 0;
      nsc_repository_entry_p->selector.src_port  = 0;
      nsc_repository_entry_p->selector.dst_port  = 0;
    }   
    else   
    {     
      nsc_repository_entry_p->selector.src_port = nschain_repository_entry_first_p->selector.dst_port;
      nsc_repository_entry_p->selector.dst_port = nschain_repository_entry_first_p->selector.src_port;
    }  
  }
  else if (tsc_flow_p->sel_type_swap_e == SEL_SECONDARY_NOSWAP)
  {
    nsc_repository_entry_p->selector.selector_type = SELECTOR_SECONDARY;
    nsc_repository_entry_p->selector.src_ip  = nschain_repository_entry_first_p->selector.src_ip;
    nsc_repository_entry_p->selector.dst_ip  = nschain_repository_entry_first_p->selector.dst_ip;

    if(nsc_repository_entry_p->selector.protocol == OF_IPPROTO_ICMP)
    {
      nsc_repository_entry_p->selector.icmp_type = 0;
      nsc_repository_entry_p->selector.icmp_code = 0;
      nsc_repository_entry_p->selector.src_port  = 0;
      nsc_repository_entry_p->selector.dst_port  = 0;
    }
    else
    {
      nsc_repository_entry_p->selector.src_port = nschain_repository_entry_first_p->selector.src_port;
      nsc_repository_entry_p->selector.dst_port = nschain_repository_entry_first_p->selector.dst_port;
    }    
  }    
 
  if(tsc_flow_p->table_no == TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1)
  {
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_1_p;
    hashmask = nsc_no_of_repository_table_1_buckets_g;
  }
  else /* TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2 */
  {
    nsc_repository_table_p   = vn_nsc_info_p->nsc_repository_table_2_p;
    hashmask = nsc_no_of_repository_table_2_buckets_g;
  }

  hashkey = nsc_compute_hash_repository_table_1_2(nsc_repository_entry_p->selector.src_ip,
                                                  nsc_repository_entry_p->selector.dst_ip,
                                                  nsc_repository_entry_p->selector.src_port,
                                                  nsc_repository_entry_p->selector.dst_port,
                                                  nsc_repository_entry_p->selector.protocol,
                                                  nsc_repository_entry_p->selector.ethernet_type,
                                                  tsc_flow_p->inport_id,
                                                  tsc_flow_p->dp_handle,
                                                  tsc_flow_p->vlan_id_pkt,
                                                  hashmask);

  nsc_repository_entry_p->selector.hashkey = hashkey;

  offset = NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
  MCHASH_BUCKET_SCAN(nsc_repository_table_p,hashkey,nsc_repository_scan_node_p,struct nschain_repository_entry *, offset)
  {
    if(nsc_repository_scan_node_p->skip_this_entry == 1)
      continue;

    if((nsc_repository_entry_p->selector.src_ip   != nsc_repository_scan_node_p->selector.src_ip )  ||
       (nsc_repository_entry_p->selector.dst_ip   != nsc_repository_scan_node_p->selector.dst_ip )  ||
       (nsc_repository_entry_p->selector.protocol != nsc_repository_scan_node_p->selector.protocol) ||
       (nsc_repository_entry_p->selector.src_port != nsc_repository_scan_node_p->selector.src_port) ||
       (nsc_repository_entry_p->selector.dst_port != nsc_repository_scan_node_p->selector.dst_port)
                                    ||
       (nsc_repository_entry_p->selector.ethernet_type != nsc_repository_scan_node_p->selector.ethernet_type)
                                    ||
       ((tsc_flow_p->inport_id != nsc_repository_scan_node_p->in_port_id))
                                    ||
       ((tsc_flow_p->dp_handle  != nsc_repository_scan_node_p->dp_handle))
                                    ||
       ((tsc_flow_p->vlan_id_pkt != nsc_repository_scan_node_p->vlan_id_pkt))
     )
       continue;
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"T1 or T2 nsc repository entry is already existing,skipping proactive push");
     //printf("\r\n PPH-18");
     printf("z");
     retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                        (uchar8_t *)nsc_repository_entry_p,
                                        nsc_repository_entry_p->heap_b
                                       );
     *repository_entry_1_p = nsc_repository_scan_node_p;
     return OF_SUCCESS;
  }
  nsc_repository_entry_p->service_chaining_type   = nschain_repository_entry_first_p->service_chaining_type;
  nsc_repository_entry_p->selector.index          = nschain_repository_entry_first_p->selector.index; 
  nsc_repository_entry_p->selector.magic          = nschain_repository_entry_first_p->selector.magic;
  nsc_repository_entry_p->selector.safe_reference = nschain_repository_entry_first_p->selector.safe_reference;
 
  if((tsc_flow_p->sel_type_swap_e == SEL_PRIMARY) || (tsc_flow_p->sel_type_swap_e == SEL_SECONDARY_NOSWAP))
  {
    nsc_repository_entry_p->selector.other_selector_p = (nschain_repository_entry_first_p->selector.other_selector_p);
    for(ii=0; ii<6; ii++)
    {
      nsc_repository_entry_p->selector.dst_mac[ii]  = nschain_repository_entry_first_p->selector.dst_mac[ii];
      nsc_repository_entry_p->selector.src_mac[ii]  = nschain_repository_entry_first_p->selector.src_mac[ii];
    }  
    nsc_repository_entry_p->selector.dst_mac_high   = nschain_repository_entry_first_p->selector.dst_mac_high; 
    nsc_repository_entry_p->selector.dst_mac_low    = nschain_repository_entry_first_p->selector.dst_mac_low; 
    nsc_repository_entry_p->selector.src_mac_high   = nschain_repository_entry_first_p->selector.src_mac_high; 
    nsc_repository_entry_p->selector.src_mac_low    = nschain_repository_entry_first_p->selector.src_mac_low; 
  }
  else if(tsc_flow_p->sel_type_swap_e == SEL_SECONDARY)
  {
    nsc_repository_entry_p->selector.other_selector_p = (&(nschain_repository_entry_first_p->selector));  
    for(ii=0; ii<6; ii++)
    {
      nsc_repository_entry_p->selector.dst_mac[ii] = nschain_repository_entry_first_p->selector.src_mac[ii];
      nsc_repository_entry_p->selector.src_mac[ii] = nschain_repository_entry_first_p->selector.dst_mac[ii];
    }
    nsc_repository_entry_p->selector.dst_mac_high   = nschain_repository_entry_first_p->selector.src_mac_high;
    nsc_repository_entry_p->selector.dst_mac_low    = nschain_repository_entry_first_p->selector.src_mac_low;
    nsc_repository_entry_p->selector.src_mac_high   = nschain_repository_entry_first_p->selector.dst_mac_high;
    nsc_repository_entry_p->selector.src_mac_low    = nschain_repository_entry_first_p->selector.dst_mac_low;
  }
  nsc_repository_entry_p->selector.cnbind_node_p = nschain_repository_entry_first_p->selector.cnbind_node_p;
  strcpy(nsc_repository_entry_p->local_switch_name_p,nw_service_p->switch_name);

  nsc_repository_entry_p->in_port_id    = tsc_flow_p->inport_id;
  nsc_repository_entry_p->metadata      = (nschain_repository_entry_first_p->metadata & 0xF0FFFFFFFFFF0000);
  nsc_repository_entry_p->metadata      = (nsc_repository_entry_p->metadata | nsc_repository_entry_p->nid);
  nsc_repository_entry_p->serviceport   = (uint16_t)((nschain_repository_entry_first_p->metadata) >> SERVICE_PORT_OFFSET);
  nsc_repository_entry_p->metadata_mask = 0xF0FFFFFFFFFFFFFF;
  nsc_repository_entry_p->pkt_origin    = tsc_flow_p->pkt_origin;
  nsc_repository_entry_p->dp_handle     = tsc_flow_p->dp_handle;
 
  nsc_repository_entry_p->flow_mod_priority = 2; /* 1; */
  nsc_repository_entry_p->skip_this_entry   = 0;
  
  nsc_repository_entry_p->next_vlan_id                  = tsc_flow_p->next_vlan_id;  
  nsc_repository_entry_p->match_vlan_id                 = tsc_flow_p->match_vlan_id; 
  nsc_repository_entry_p->vlan_id_pkt                   = tsc_flow_p->vlan_id_pkt; 
  nsc_repository_entry_p->selector.vlan_id_pkt          = tsc_flow_p->vlan_id_pkt; 
  nsc_repository_entry_p->copy_local_mac_addresses_b    = tsc_flow_p->copy_local_mac_addresses_b; 
  nsc_repository_entry_p->copy_original_mac_addresses_b = tsc_flow_p->copy_original_mac_addresses_b; 

  if(tsc_flow_p->sel_type_swap_e == SEL_PRIMARY)
    nsc_repository_entry_p->zone = ZONE_LEFT; 
  else
    nsc_repository_entry_p->zone = ZONE_RIGHT; 

  if(tsc_flow_p->out_port_no != 0)
  {   
    nsc_repository_entry_p->ns_chain_b = TRUE;
    nsc_repository_entry_p->more_nf_b  = TRUE;
    nsc_repository_entry_p->next_table_id = 0;

    nsc_repository_entry_p->out_port_no   = tsc_flow_p->out_port_no;    
    nsc_repository_entry_p->nw_port_b = tsc_flow_p->nw_port_b;          
    if(nsc_repository_entry_p->nw_port_b == TRUE)                       
      nsc_repository_entry_p->remote_switch_ip = tsc_flow_p->remote_ip; 
    else                                                                 
      nsc_repository_entry_p->remote_switch_ip = 0;   
    nsc_repository_entry_p->ns_port_b  = FALSE;     
  }
  else
  {
    /* out_port_no can be 0 only for some table 1 flows but not for table_2 flows. */
    nsc_repository_entry_p->ns_chain_b = TRUE;
    nsc_repository_entry_p->more_nf_b  = FALSE;
    nsc_repository_entry_p->next_table_id = TSC_APP_UNICAST_TABLE_ID_3; //TBD to be removed
    /* Avoid Table 3 */
    do
    {
      /* Assuming it is a remote port get out_port_no,tun_dest_ip,remote_switch_name */
      retval = tsc_nvm_modules[nw_type].nvm_module_get_vmport_by_dmac_nid(nsc_repository_entry_p->selector.dst_mac,
                                                                          tsc_flow_p->nid_vm,&crm_port_handle);

      if(retval != OF_SUCCESS)
      {
        retval = OF_FAILURE;
        printf("\r\n PPH-24E"); 
        break;
      }
      retval = crm_get_port_byhandle(crm_port_handle,&dst_crm_port_p);
      if(retval == OF_SUCCESS)
      {
        nsc_repository_entry_p->out_port_no      = dst_crm_port_p->port_id;
        /* Used if it is a local port */
        nsc_repository_entry_p->remote_switch_ip = dst_crm_port_p->switch_ip;
        /* Used if it is a remote port */
        port_type                                = dst_crm_port_p->crm_port_type;
        nsc_repository_entry_p->ns_port_b        = TRUE;
        nsc_repository_entry_p->nw_port_b        = FALSE;
      }
      else
      {
        retval = OF_FAILURE;
        printf("\r\n PPH-24F");
        break;
      }
      /*  If local and remote switch names are same
       *  - Output the packet to the local VM side port.
       *  - Add flow without DMAC as match field??? and Output to crm_port as an action. */

      if((port_type == VM_GUEST_PORT)
                       ||
         (strcmp((nsc_repository_entry_p->local_switch_name_p),
                 (dst_crm_port_p->switch_name)))
        )
      {
          /* If local and remote switch names are not matching.crm_port is a remote port.
           * - Output packet on appropriate NW port to reach the remote destination crm port. */

          /* Get unicast port nw_out_port_no */

          retval = tsc_nvm_modules[nw_type].nvm_module_get_unicast_nwport(nsc_repository_entry_p->local_switch_name_p,
                                                                          nsc_repository_entry_p->serviceport,
                                                                          &(nsc_repository_entry_p->out_port_no)
                                                                         );

          if(retval != OF_SUCCESS)
          {
              retval = OF_FAILURE;
              printf("\r\n PPH-24G");
              break;
          }
          nsc_repository_entry_p->ns_port_b  = TRUE;    
          nsc_repository_entry_p->nw_port_b  = TRUE;
      }
    }while(0);

    if(retval == OF_FAILURE)
    {
      retval = mempool_release_mem_block(nsc_repository_mempool_g,
               (uchar8_t *)nsc_repository_entry_p,
               nsc_repository_entry_p->heap_b);
      printf("\r\n PPH-24A");
      return OF_FAILURE;
    }
  }

  hashobj_p = (uchar8_t *)(nsc_repository_entry_p) + offset;
  /* Append to table 1 or 2 */
  MCHASH_APPEND_NODE(nsc_repository_table_p,hashkey,nsc_repository_entry_p,index,magic,hashobj_p,status);
  if(status == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to append repository entry");
    retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                      (uchar8_t *)nsc_repository_entry_p,
                                       nsc_repository_entry_p->heap_b
                                     );
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"PF-Failed to append repository entry and free the repository entry memory");
    printf("\r\n PPH-20");
    return OF_FAILURE;
  }
  nsc_repository_entry_p->magic = magic;
  nsc_repository_entry_p->index = index;

  nsc_repository_entry_p->safe_reference = magic;
  (nsc_repository_entry_p->safe_reference) = (((nsc_repository_entry_p->safe_reference) << 32) | (index));

  CNTLR_LOCK_TAKE(nsc_repository_entry_p->selector.cnbind_node_p->cnbind_node_lock);
  ++(nsc_repository_entry_p->selector.cnbind_node_p->no_of_flow_entries_deduced);
  //printf("\r\n present no_of_flow_entries_deduced = %d",nsc_repository_entry_p->selector.cnbind_node_p->no_of_flow_entries_deduced);
  CNTLR_LOCK_RELEASE(nsc_repository_entry_p->selector.cnbind_node_p->cnbind_node_lock);

  *repository_entry_1_p = nsc_repository_entry_p; 

  if(tsc_flow_p->table_no == TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1)
  {
    retval = tsc_add_flwmod_1_3_msg_table_1(tsc_flow_p->dp_handle,nsc_repository_entry_p);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to send Flow Mod message to Table 1");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Deleting the nschain repository entry added just as failed to send the flow entry to switch");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add Flow Mod message to Table 1 proactively ");
      tsc_delete_nschain_repository_entry(nsc_repository_entry_p,nw_type,nid,tsc_flow_p->table_no); /* NSME */
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully sent Flow Mod message to Table 1 proactively ");
    }    
  }
  else if(tsc_flow_p->table_no == TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2)
  {
    retval = tsc_add_flwmod_1_3_msg_table_2(tsc_flow_p->dp_handle,nsc_repository_entry_p);
    if(retval == OF_FAILURE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to send Flow Mod message to Table 2");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Deleting the nschain repository entry added just as failed to send the flow entry to switch");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add Flow Mod message to Table 2 proactively ");
      tsc_delete_nschain_repository_entry(nsc_repository_entry_p,nw_type,nid,tsc_flow_p->table_no); /* NSME */
    }
    else
    {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully sent Flow Mod message to Table 2 proactively ");
    }    
  }
  retval = OF_SUCCESS;
  return retval;
}
int32_t tsc_nsc_send_proactive_flow_table_3_zone_left(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                                      struct    nwservice_instance *nw_service_p,
                                                      uint8_t   use_dstmac,
                                                      uint32_t  nid)
{
  uint64_t  crm_port_handle,vn_handle,dp_handle,metadata;
  uint32_t  offset,index,magic,hashkey,hashmask;
  uint32_t  dst_mac_high,dst_mac_low;
  uint16_t  serviceport;
  uint8_t   heap_b,nw_type;
  uchar8_t  *hashobj_p = NULL;
  int32_t   retval = OF_SUCCESS;
  int32_t   status = OF_SUCCESS;

  struct    mchash_table*  nsc_repository_table_p;
  void*     nsc_repository_mempool_g;
  struct    vn_service_chaining_info* vn_nsc_info_p;
  struct    crm_virtual_network*      vn_entry_p;
  struct    ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p     = NULL;
  struct    ucastpkt_outport_repository_entry* ucastpkt_outport_repository_scan_node_p = NULL;
  struct    crm_port* dst_crm_port_p;

  dp_handle  = nw_service_p->system_br_saferef;

  nw_type =  nschain_repository_entry_first_p->nw_type;
  nschain_repository_entry_first_p->nid = nid;

  metadata     = (nschain_repository_entry_first_p->metadata & 0xF0FFFFFFFFFF0000);
  metadata     = (nschain_repository_entry_first_p->metadata | nid); 
  serviceport  = (uint16_t)((nschain_repository_entry_first_p->metadata) >> SERVICE_PORT_OFFSET);

  if(use_dstmac == TRUE)
  {    
    dst_mac_high  =  nschain_repository_entry_first_p->selector.dst_mac_high;
    dst_mac_low   =  nschain_repository_entry_first_p->selector.dst_mac_low;
  }
  else
  {
    dst_mac_high  =  nschain_repository_entry_first_p->selector.src_mac_high;
    dst_mac_low   =  nschain_repository_entry_first_p->selector.src_mac_low;
  }    

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vnhandle");
    printf("\r\n PPH-21");
    return OF_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn by handle");
    printf("\r\n PPH-22");
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

     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_3 ucastpkt_outport repository entry found,skipping proactive push");
     return OF_SUCCESS;
  }   

  /* get memblock */

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"get new mem block for ucastpkt outport repository entry");
  retval = mempool_get_mem_block(nsc_repository_mempool_g,(uchar8_t **)&ucastpkt_outport_repository_entry_p,&heap_b);
  if(retval != MEMPOOL_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Memory block allocation for ucastpkt outport repository entry failed");
    printf("\r\n PPH-23");
    return OF_FAILURE;
  }
  
  strcpy(ucastpkt_outport_repository_entry_p->local_switch_name_p,nw_service_p->switch_name);
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

  /* Extract DMAC */
  if(use_dstmac == TRUE)
    memcpy(ucastpkt_outport_repository_entry_p->dst_mac,nschain_repository_entry_first_p->selector.dst_mac,6); 
  else
    memcpy(ucastpkt_outport_repository_entry_p->dst_mac,nschain_repository_entry_first_p->selector.src_mac,6);

  do
  {
    /* Assuming it is a remote port get out_port_no,tun_dest_ip,remote_switch_name */
    /* In 2 port case also, it works as we require that the two communicating VMs are in the same VN for them 
       to respond to ARP requests.
       The communicating VMs must be in opposite (LEFT,RIGHT) zones. 
    */   
    retval = tsc_nvm_modules[nw_type].nvm_module_get_vmport_by_dmac_nid(ucastpkt_outport_repository_entry_p->dst_mac,
                                                                        ucastpkt_outport_repository_entry_p->nid,
                                                                        &crm_port_handle);

    if(retval != OF_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

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

    /*  If local and remote switch names are same
     *  - Output the packet to the local VM side port.
     *  - Add flow with DMAC and metadata as match fields and Output to crm_port as an action. */

     if((ucastpkt_outport_repository_entry_p->port_type == VM_GUEST_PORT)
                                   ||
        (strcmp((ucastpkt_outport_repository_entry_p->local_switch_name_p),
                (ucastpkt_outport_repository_entry_p->remote_switch_name_p)))
       )
     {
       /* If local and remote switch names are not matching.crm_port is a remote port.
        * - Output packet on appropriate NW port to reach the remote destination crm port. */

       /* Get unicast port nw_out_port_no */

       retval = tsc_nvm_modules[nw_type].nvm_module_get_unicast_nwport(ucastpkt_outport_repository_entry_p->local_switch_name_p,
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
     retval = mempool_release_mem_block(nsc_repository_mempool_g,
                                        (uchar8_t *)ucastpkt_outport_repository_entry_p,
                                        ucastpkt_outport_repository_entry_p->heap_b
                                       );
     printf("\r\n PPH-24");
     return OF_FAILURE;
   }

   ucastpkt_outport_repository_entry_p->priority = 1; /* 2; */
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
     printf("\r\n PPH-25");
     return retval;
   }

   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table_3 Successfully appended ucastpkt outport repository entry to hash table");
   ucastpkt_outport_repository_entry_p->magic = magic;
   ucastpkt_outport_repository_entry_p->index = index;

   ucastpkt_outport_repository_entry_p->safe_reference = magic;
   (ucastpkt_outport_repository_entry_p->safe_reference) = (((ucastpkt_outport_repository_entry_p->safe_reference) << 32) | (index));

   retval = tsc_add_flwmod_1_3_msg_table_3(dp_handle,ucastpkt_outport_repository_entry_p);

   if(retval == OF_FAILURE)
   {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Failed to send Flow Mod message to Table 3 proactively ");
   }
   else
   {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Successfully sent Flow Mod message to Table 3 proactively ");
   }    
   return OF_SUCCESS;
}
/****************************************************************************************************************************************/
