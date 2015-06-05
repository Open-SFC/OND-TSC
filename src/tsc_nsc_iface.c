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

/* File: tsc_nsc_iface.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file implements interface to service chaining core module.
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

void nsc_print_repository_entry(struct nschain_repository_entry* nschain_repository_entry_p);
int32_t nsrm_test_display_service_vms_in_chain(struct  l2_connection_to_nsinfo_bind_node* cn_bind_entry_p,uint8_t selector_type);
int32_t tsc_nsc_send_proactive_flow(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                    struct    tsa_all_flows_members* tsc_all_flows_p,
                                    struct    nwservice_instance *nw_service_p,
                                    struct    nschain_repository_entry** repository_entry_1_p);

int32_t tsc_nsc_send_proactive_flow_table_3(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                            struct    nwservice_instance *nw_service_p,
                                            uint8_t   use_dstmac);
/*******************************************************************************************************************************
Function Name:nsc_get_l2_nschaining_info
Input Parameters:
	nschain_repository_entry_p: This contains input parameters of network type and id.
                                    This function populates this structure with OF Flow information to be added to datapath.   
	pkt_in: Miss packet received from a datapath via controller.
Description:
        1. This function obtains the cnbind entry matching with the packet selector fields in input argument nschain_repository_entry_p. 
        2. If cnbind entry is created now, obtain the service chaining information for the cnbind entry. 
        3. The nschain repository entry is filled with information extracted from the service chaining info. 
*******************************************************************************************************************************/
int32_t nsc_get_l2_nschaining_info(struct  nschain_repository_entry* nschain_repository_entry_p,
                                   struct  ofl_packet_in* pkt_in,
                                   uint8_t pkt_origin)
{

  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   crm_virtual_network*      vn_entry_p;
  struct   crm_port* dst_crm_port_p;

  uint64_t vn_handle,crm_port_handle;
  struct   l2_connection_to_nsinfo_bind_node* cn_bind_entry_p;
  struct   nwservice_instance* nwservice_instance_p;
  struct   nw_services_to_apply *nw_services_p;
  struct   nw_services_to_apply *nw_services_secondary_p;
  uint8_t  selector_type,ii,port_type;
  uint8_t  nsc_nw_function_found;
  uint8_t  no_of_nwservice_instances;
  uint8_t  nwservice_instance_index;

  int32_t retval = OF_FAILURE; 
  uint8_t copy_original_mac_b; 

  copy_original_mac_b = FALSE;
  if((pkt_origin == VM_NS) && (nschain_repository_entry_p->vlan_id_pkt != 0))
    copy_original_mac_b = TRUE;

  /* vn is deleted only after its internal members are deleted. */
  retval = nsc_get_cnbind_entry(pkt_in,
                                &nschain_repository_entry_p->selector,
                                nschain_repository_entry_p->nw_type,
                                nschain_repository_entry_p->nid,
                                &cn_bind_entry_p,
                                &selector_type,
                                copy_original_mac_b);

  if(retval == OF_FAILURE)
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsc_get_cnbind_entry() returned OF_FAILURE");
    return OF_FAILURE;  /* The caller drops the packets. This error is a systemic error mainly due to memory allocation failures. */ 
  }

  if(retval == NSC_CONNECTION_BIND_ENTRY_CREATED)  
  {
   nschain_repository_entry_p->selector.selector_type  = selector_type;
   nschain_repository_entry_p->selector.cnbind_node_p  = cn_bind_entry_p; /* Added to make it work for proactive flow push */
   if(nschain_repository_entry_p->selector.selector_type == 1)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"A matching PRIMARY SELECTOR is found. selector type 1:PRIMARY 2:SECONDARY  = %d",nschain_repository_entry_p->selector.selector_type);
   }
   else
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"A matching SECONDARY SELECTOR is found. selector type 1:PRIMARY 2:SECONDARY  = %d",nschain_repository_entry_p->selector.selector_type);
   }

    retval = nsc_lookup_l2_nschaining_info(pkt_in,
                                           nschain_repository_entry_p->nw_type,
                                           nschain_repository_entry_p->nid,   
                                           cn_bind_entry_p);

    if(retval == OF_FAILURE)
    {
      /* Either due to improper nscconfiguration or systemic failures like memory allocation failure */
      /* Drop the packets as nsc configuration is not proper. */
      if(cn_bind_entry_p != NULL)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"releasing cn_bind_entry to mempool as nsc lookup failed");
        CNTLR_RCU_READ_LOCK_TAKE();
  
        retval = tsc_nvm_modules[nschain_repository_entry_p->nw_type].nvm_module_get_vnhandle_by_nid(nschain_repository_entry_p->nid,&vn_handle);
        if(retval == OF_SUCCESS)
          retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);

        if(retval == OF_SUCCESS)
        {   
          /* Free Network Service Info before freeing cnbind entry itself */
          cn_bind_entry_p->nwservice_info.no_of_nwservice_instances = 0;
          if(cn_bind_entry_p->nwservice_info.nw_services_p != NULL)
            free(cn_bind_entry_p->nwservice_info.nw_services_p);

          vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
          mempool_release_mem_block(vn_nsc_info_p->nsc_cn_bind_mempool_g,
                                    (uchar8_t *)cn_bind_entry_p,
                                    cn_bind_entry_p->heap_b);

          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"cn_bind_entry is released to mempool as nsc lookup failed");
       }
       CNTLR_RCU_READ_LOCK_RELEASE(); 
      }        
      return OF_FAILURE;
    }

    CNTLR_RCU_READ_LOCK_TAKE();

    retval = tsc_nvm_modules[nschain_repository_entry_p->nw_type].nvm_module_get_vnhandle_by_nid(nschain_repository_entry_p->nid,&vn_handle);
    if(retval == OF_SUCCESS)
      retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);

    if(retval == OF_SUCCESS)
    {
      vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
      retval = nsc_add_selectors(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p,vn_nsc_info_p->nsc_cn_bind_mempool_g ,cn_bind_entry_p);
    }
    CNTLR_RCU_READ_LOCK_RELEASE();  
    if(retval == OF_FAILURE)
      return retval; 
  }

  nschain_repository_entry_p->selector.index = cn_bind_entry_p->selector_1.index;
  nschain_repository_entry_p->selector.magic = cn_bind_entry_p->selector_1.magic;
  nschain_repository_entry_p->selector.safe_reference = cn_bind_entry_p->selector_1.safe_reference;

  nschain_repository_entry_p->selector.other_selector_p = &(cn_bind_entry_p->selector_2);
  nschain_repository_entry_p->selector.other_selector_p->index = cn_bind_entry_p->selector_2.index;
  nschain_repository_entry_p->selector.other_selector_p->magic = cn_bind_entry_p->selector_2.magic;
  nschain_repository_entry_p->selector.other_selector_p->safe_reference = cn_bind_entry_p->selector_2.safe_reference;
  nschain_repository_entry_p->vm_and_service1_not_on_same_switch = 0;
  nschain_repository_entry_p->ns_chain_b = FALSE;
  nschain_repository_entry_p->more_nf_b  = FALSE; 
  nschain_repository_entry_p->ns_port_b  = FALSE; 

  if((cn_bind_entry_p->nsc_config_status ==  NSC_NWSERVICES_CONFIGURED))
  {
    //nsrm_test_display_service_vms_in_chain(cn_bind_entry_p,selector_type);

    nschain_repository_entry_p->ns_chain_b = TRUE;    
    nschain_repository_entry_p->nw_port_b  = FALSE;
        
    no_of_nwservice_instances  =  cn_bind_entry_p->nwservice_info.no_of_nwservice_instances;
    nw_services_p = (struct  nw_services_to_apply *)cn_bind_entry_p->nwservice_info.nw_services_p;
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"no_of_nwservice_instances = %d",no_of_nwservice_instances);

    #if 1  /* comment if order of services shall not be reversed for the second selector. */ 
    if(selector_type == SELECTOR_SECONDARY)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"selector is secondary selector."); 
      nw_services_secondary_p = (struct nw_services_to_apply*)(nw_services_p + no_of_nwservice_instances - 1);
      nw_services_p = nw_services_secondary_p;
    }  
    else
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"selector is primary selector."); 
    } 
    #endif

    nsc_nw_function_found  =  0;
    
    CNTLR_RCU_READ_LOCK_TAKE();
 
     for(nwservice_instance_index = 0; nwservice_instance_index < no_of_nwservice_instances; nwservice_instance_index++) 
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index);
      if(nwservice_instance_index >0)
      {
        if(selector_type == SELECTOR_PRIMARY)  /* //  Uncomment later if order of services to be reversed for the second selector */
        {
          nw_services_p++;
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Incrementing nw_servcies_p");
        }
        else                   /* // comment if order of services shall not be reversed for the second selector */
        {                      /* // */
          nw_services_p--;     /* // */
        }                      /* // */ 
      }
      retval = nsrm_get_nwservice_instance_byhandle(nw_services_p->nschain_object_handle,
                                                    nw_services_p->nwservice_object_handle,
                                                    nw_services_p->nwservice_instance_handle,
                                                    &nwservice_instance_p);
 
      if(retval == OF_FAILURE)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the service instance.Get the next service instance"); 
        continue;  /* Start from next service. A re-lookup may be required. */     
      }
      
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"network service instance name = %s",nwservice_instance_p->name_p);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %d",pkt_origin);

      if(pkt_origin == VM_NS)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Incoming network function found for VM_NS");
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index);

        if(nsc_nw_function_found == 0)
        {
          if((selector_type == SELECTOR_PRIMARY) && (nwservice_instance_p->vlan_id_out == nschain_repository_entry_p->vlan_id_pkt))
          {
            nsc_nw_function_found = 1;  /* Found the nw_function from which the packet is received. */
          }
          
          if((selector_type == SELECTOR_SECONDARY) && (nwservice_instance_p->vlan_id_in == nschain_repository_entry_p->vlan_id_pkt))
          {
            nsc_nw_function_found = 1;  /* Found the nw_function from which the packet is received. */
          }
        }
        else if(nsc_nw_function_found == 1)
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"outgoing network function found for VM_NS");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index); 

          nsc_nw_function_found = 2; /* Target nsc_nw_function found */
          nschain_repository_entry_p->more_nf_b     = TRUE;
          nschain_repository_entry_p->next_vlan_id  = nwservice_instance_p->vlan_id_in;
          if(selector_type == SELECTOR_SECONDARY)
          {
            nschain_repository_entry_p->next_vlan_id  = nwservice_instance_p->vlan_id_out;
          }   
          nschain_repository_entry_p->match_vlan_id = nschain_repository_entry_p->vlan_id_pkt;

          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_id_in = %d",nschain_repository_entry_p->next_vlan_id);
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_it_out= %d",nschain_repository_entry_p->match_vlan_id);
      
          break; 
        }     
      }
      if(pkt_origin == VM_APPLN)
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Target network function found for VM_APPLN ");
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index);
        nsc_nw_function_found = 1; /* Target nw function found */
        nschain_repository_entry_p->more_nf_b     = TRUE;
        nschain_repository_entry_p->next_vlan_id  = nwservice_instance_p->vlan_id_in;
        if(selector_type == SELECTOR_SECONDARY)
        {
          printf("\r\n VM_APPLN SEC");
          nschain_repository_entry_p->next_vlan_id  = nwservice_instance_p->vlan_id_out;
        } 
        else
        {
          printf("\r\n VM_APPLN PRI");
        }    
        nschain_repository_entry_p->match_vlan_id = 0;
          
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_id_in = %d",nschain_repository_entry_p->next_vlan_id);
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_it_out= %d",nschain_repository_entry_p->match_vlan_id);

        break;
      }
      if((pkt_origin == NW_UNICAST_SIDE)||(pkt_origin == NW_BROADCAST_SIDE))
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Packet is received from NW");
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nschain_repository_entry_p->vlan_id_pkt = %d",nschain_repository_entry_p->vlan_id_pkt);
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_p->vlan_id_innwservice_instance_p->vlan_id_in = %d",nwservice_instance_p->vlan_id_in);
      
        if(nschain_repository_entry_p->vlan_id_pkt == nwservice_instance_p->vlan_id_in)
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Target network function found for NW");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index);
          nsc_nw_function_found = 1; /* Target nw function found for the remote VM or VM_NS */
          nschain_repository_entry_p->more_nf_b = TRUE;

          nschain_repository_entry_p->next_vlan_id  = nschain_repository_entry_p->vlan_id_pkt;
          nschain_repository_entry_p->match_vlan_id = nschain_repository_entry_p->vlan_id_pkt;

          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_id_in = %d",nschain_repository_entry_p->next_vlan_id);
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_it_out= %d",nschain_repository_entry_p->match_vlan_id);

          break;
        } 
       
        if(nschain_repository_entry_p->vlan_id_pkt == nwservice_instance_p->vlan_id_out)
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Target network function found for NW");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index);
          nsc_nw_function_found = 1; /* Target nw function found for the remote VM or VM_NS */
          nschain_repository_entry_p->more_nf_b = TRUE;

          nschain_repository_entry_p->next_vlan_id  = nschain_repository_entry_p->vlan_id_pkt;
          nschain_repository_entry_p->match_vlan_id = nschain_repository_entry_p->vlan_id_pkt;

          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_id_in = %d",nschain_repository_entry_p->next_vlan_id);
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_it_out= %d",nschain_repository_entry_p->match_vlan_id);

          break;
        }
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Look for next service function if it matches with the received packet.");
      } 
    } /* for */
  
    CNTLR_RCU_READ_LOCK_RELEASE();

    nschain_repository_entry_p->selector.selector_type = selector_type;

    if(pkt_origin == VM_NS)
    { 
      if(nsc_nw_function_found == 1) /* No more network functions present in the nschain. */
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"No more network functions present in the nschain: VM_NS");
        //OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_index = %d",nwservice_instance_index);
        nschain_repository_entry_p->more_nf_b  = FALSE;
        nschain_repository_entry_p->ns_port_b  = TRUE;
        nschain_repository_entry_p->next_vlan_id  = 0;
        nschain_repository_entry_p->match_vlan_id = nschain_repository_entry_p->vlan_id_pkt;
        nschain_repository_entry_p->next_table_id = TSC_APP_UNICAST_TABLE_ID_3;
        
        do
        {
          CNTLR_RCU_READ_LOCK_TAKE();

          /* Assuming it is a remote port get out_port_no,tun_dest_ip,remote_switch_name */
          printf("\r\n crashing dst mac = " );
          for(ii = 0;ii<6;ii++)
          {
            printf("%x",nschain_repository_entry_p->selector.dst_mac[ii]);
          }    
          retval = tsc_nvm_modules[nschain_repository_entry_p->nw_type].nvm_module_get_vmport_by_dmac_nid(nschain_repository_entry_p->selector.dst_mac,
                                                                                                          nschain_repository_entry_p->nid,
                                                                                                          &crm_port_handle);
          if(retval != OF_SUCCESS)
          {
            retval = OF_FAILURE;
            printf("\r\n MET 01");
            break;
          }
          retval = crm_get_port_byhandle(crm_port_handle,&dst_crm_port_p);
          if(retval == OF_SUCCESS)
          {
            nschain_repository_entry_p->out_port_no      = dst_crm_port_p->port_id;
            nschain_repository_entry_p->remote_switch_ip = dst_crm_port_p->switch_ip;
            port_type                                    = dst_crm_port_p->crm_port_type;
            nschain_repository_entry_p->nw_port_b        = FALSE;
          } 
          else
          {
            retval = OF_FAILURE;
            printf("\r\n MET 02");
            break;
          }
          /*  If local and remote switch names are same
           *  - Output the packet to the local VM side port.
           *  - Add flow without DMAC as match field??? and Output to crm_port as an action. */

          if((port_type == VM_GUEST_PORT)
                           ||
             (strcmp((nschain_repository_entry_p->local_switch_name_p),(dst_crm_port_p->switch_name)))
            )
          {
              /* If local and remote switch names are not matching.crm_port is a remote port.
               * - Output packet on appropriate NW port to reach the remote destination crm port. */

              /* Get unicast port nw_out_port_no */

              retval = tsc_nvm_modules[nschain_repository_entry_p->nw_type].nvm_module_get_unicast_nwport(nschain_repository_entry_p->local_switch_name_p,
                                                                                                          nschain_repository_entry_p->serviceport,
                                                                                                          &(nschain_repository_entry_p->out_port_no)
                                                                                                         );

              if(retval != OF_SUCCESS)
              {
                retval = OF_FAILURE;
                printf("\r\n MET 03");
                break;
              }
              nschain_repository_entry_p->ns_port_b = TRUE;
              nschain_repository_entry_p->nw_port_b = TRUE;
          }
        }while(0);

        CNTLR_RCU_READ_LOCK_RELEASE();   
       
        if(retval == OF_FAILURE)
        {
          printf("\r\n MET 04");
          return OF_FAILURE;
        }
      }
      if(nsc_nw_function_found == 0) 
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"No matching network function found in the nschain: VM_NS");
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"This message will not appear in general.VM-NS is using a non servicing chain vlan id and may be a configuration packet"); 
        nschain_repository_entry_p->more_nf_b     = FALSE; 
        nschain_repository_entry_p->next_vlan_id  = 0;
        nschain_repository_entry_p->match_vlan_id = nschain_repository_entry_p->vlan_id_pkt;
        nschain_repository_entry_p->next_table_id = TSC_APP_UNICAST_TABLE_ID_3;

        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_id_in = %d",nschain_repository_entry_p->next_vlan_id);
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"vlan_it_out= %d",nschain_repository_entry_p->match_vlan_id);
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"next_table_id= %d",nschain_repository_entry_p->next_table_id);
      }
    }
    if((pkt_origin == VM_APPLN) && (nsc_nw_function_found == 0)) 
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"No matching network function found:VM_APPLN ");
      nschain_repository_entry_p->ns_chain_b    = FALSE;
      nschain_repository_entry_p->next_vlan_id  = 0;
      nschain_repository_entry_p->match_vlan_id = 0;
      nschain_repository_entry_p->next_table_id = TSC_APP_UNICAST_TABLE_ID_3;

      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Service Chaining rule not found, send packet to table 3");
    }

    if(((pkt_origin == NW_UNICAST_SIDE)||(pkt_origin == NW_BROADCAST_SIDE)) && (nsc_nw_function_found == 0))
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"No matching network function found:NW");
      nschain_repository_entry_p->ns_chain_b    = TRUE;
      nschain_repository_entry_p->more_nf_b     = FALSE;
      nschain_repository_entry_p->next_vlan_id  = 0;
      nschain_repository_entry_p->match_vlan_id = nschain_repository_entry_p->vlan_id_pkt;
      nschain_repository_entry_p->next_table_id = TSC_APP_UNICAST_TABLE_ID_3;

      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," Service Chaining rule not found, send packet to table 3");
    }
  }   

  if((cn_bind_entry_p->nsc_config_status == NSC_NWSERVICES_NOT_CONFIGURED) || (cn_bind_entry_p->nsc_config_status ==  NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN))  
  {
    nschain_repository_entry_p->selector.selector_type = selector_type;
    nschain_repository_entry_p->ns_chain_b    = FALSE;
    nschain_repository_entry_p->next_vlan_id  = 0;
    nschain_repository_entry_p->match_vlan_id = 0;
    nschain_repository_entry_p->next_table_id = TSC_APP_UNICAST_TABLE_ID_3;
   
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," nw_service chaining not enabled for this connection.send packet to its destination VM via table 3"); 
  }
  if(nschain_repository_entry_p->more_nf_b == TRUE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"more network functions in the ns chain");
    if(strcmp(nschain_repository_entry_p->local_switch_name_p,nwservice_instance_p->switch_name))   
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"The next NW-Service VM in the ns chain is in another compute node");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local  switch name = %s",nschain_repository_entry_p->local_switch_name_p);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"remote switch name = %s",nwservice_instance_p->switch_name);     
      nschain_repository_entry_p->vm_and_service1_not_on_same_switch = 1; /* For Proactive pushing  */

      CNTLR_RCU_READ_LOCK_TAKE();
      /* Get IP of the remote switch */ 
      retval = dprm_get_switch_data_ip(nwservice_instance_p->switch_name,&(nschain_repository_entry_p->remote_switch_ip));
      if(retval != DPRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid remote switch name");
        CNTLR_RCU_READ_LOCK_RELEASE();
        return OF_FAILURE;
      }  
    
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Calling nvm_module_get_unicast_nwport");
      retval = tsc_nvm_modules[nschain_repository_entry_p->nw_type].nvm_module_get_unicast_nwport(nschain_repository_entry_p->local_switch_name_p,
                                                                                                  nschain_repository_entry_p->serviceport,
                                                                                                  &(nschain_repository_entry_p->out_port_no));
      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unicast nwport not found");
        CNTLR_RCU_READ_LOCK_RELEASE();
        return OF_FAILURE;
      }
      CNTLR_RCU_READ_LOCK_RELEASE();

      nschain_repository_entry_p->nw_port_b = TRUE;
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," nw side unicastport number = %d",nschain_repository_entry_p->out_port_no);
    }  
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"The next NW-Service VM in the ns chain is in the same compute node");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local  switch name = %s",nschain_repository_entry_p->local_switch_name_p);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"remote switch name = %s",nwservice_instance_p->switch_name);

      nschain_repository_entry_p->nw_port_b   = FALSE;   
      nschain_repository_entry_p->out_port_no = nwservice_instance_p->port_id;
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port number = %d",nschain_repository_entry_p->out_port_no); 
    }
  }
  if(selector_type == SELECTOR_PRIMARY)
  {
    for(ii=0; ii<6; ii++)
    {
      nschain_repository_entry_p->local_in_mac[ii]  = cn_bind_entry_p->local_in_mac[ii];
      nschain_repository_entry_p->local_out_mac[ii] = cn_bind_entry_p->local_out_mac[ii];
    }
  }
  if(selector_type == SELECTOR_SECONDARY)
  {
    for(ii=0; ii<6; ii++) 
    {
      nschain_repository_entry_p->local_in_mac[ii]  = cn_bind_entry_p->local_out_mac[ii];
      nschain_repository_entry_p->local_out_mac[ii] = cn_bind_entry_p->local_in_mac[ii];
    }
  }

  for(ii=0;ii<6;ii++)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_mac = %x",nschain_repository_entry_p->local_out_mac[ii]);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_mac = %x",nschain_repository_entry_p->local_in_mac[ii]);
  }
  
   nschain_repository_entry_p->copy_local_mac_addresses_b       = FALSE;
   nschain_repository_entry_p->copy_original_mac_addresses_b    = FALSE;

  if((pkt_origin == VM_APPLN) && (nschain_repository_entry_p->more_nf_b == TRUE) && (nschain_repository_entry_p->nw_port_b == FALSE))
  {
    nschain_repository_entry_p->copy_local_mac_addresses_b = TRUE;
  }

  if((pkt_origin == VM_NS) && (nschain_repository_entry_p->more_nf_b == TRUE))
  {
    if(nschain_repository_entry_p->nw_port_b == TRUE)
    {
      nschain_repository_entry_p->copy_original_mac_addresses_b = TRUE;
    }
  } 

  else if((pkt_origin == VM_NS) && (nschain_repository_entry_p->more_nf_b == FALSE))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Set match fields as local mac addresses for packets received from local service VM");
    nschain_repository_entry_p->copy_original_mac_addresses_b = TRUE;
  }  
 
  if(((pkt_origin == NW_UNICAST_SIDE)||(pkt_origin == NW_BROADCAST_SIDE)) && (nschain_repository_entry_p->more_nf_b == TRUE) && (nschain_repository_entry_p->nw_port_b == FALSE)) 
  {
    nschain_repository_entry_p->copy_local_mac_addresses_b = TRUE; 
  }
  //nsc_print_repository_entry(nschain_repository_entry_p);
  
  return OF_SUCCESS;
} 
/****************************************************************************************************************
Debug Function which displays all the entries of nschain repository.It is called loacally from this file. 
****************************************************************************************************************/
void nsc_print_repository_entry(struct nschain_repository_entry* nschain_repository_entry_p)
{
  uint8_t index;
  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"*********Packet matching selector details:****************");  
  
  if(nschain_repository_entry_p->selector.selector_type == SELECTOR_PRIMARY)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"selector_type = %s"," PRIMARY");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"selector_type = %s"," SECONDARY");
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_ip   = %x",nschain_repository_entry_p->selector.src_ip);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_ip   = %x",nschain_repository_entry_p->selector.dst_ip);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_port = %d",nschain_repository_entry_p->selector.src_port);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_port = %d",nschain_repository_entry_p->selector.dst_port);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"protocol = %d",nschain_repository_entry_p->selector.protocol);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac_high = %x",nschain_repository_entry_p->selector.src_mac_high);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac_low  = %x",nschain_repository_entry_p->selector.src_mac_low);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_high = %x",nschain_repository_entry_p->selector.dst_mac_high);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_low  = %x",nschain_repository_entry_p->selector.dst_mac_low);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Source mac Address = ");

  for(index = 0;index < 6;index++)
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," %02x",nschain_repository_entry_p->selector.src_mac[index]);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Destination mac Address = ");
  for(index = 0;index < 6;index++)
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," %02x",nschain_repository_entry_p->selector.dst_mac[index]);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"ethernet_type = %d",nschain_repository_entry_p->selector.ethernet_type);
  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan_id_pkt = %d",nschain_repository_entry_p->selector.vlan_id_pkt);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"hashkey = %d",nschain_repository_entry_p->selector.hashkey);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"***Service Chaining Information:***");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"****************************");

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Switch Name = %s",nschain_repository_entry_p->local_switch_name_p);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"DP handle   = %llx",nschain_repository_entry_p->dp_handle); 

  if(nschain_repository_entry_p->nw_type == VXLAN_TYPE) 
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw-type = %s","VXLAN_TYPE");
  }
  else if(nschain_repository_entry_p->nw_type == NVGRE_TYPE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw-type = %s","NVGRE_TYPE");
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nid         = %x",nschain_repository_entry_p->nid);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"in_port_id  = %d",nschain_repository_entry_p->in_port_id);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"metadata      = %llx",nschain_repository_entry_p->metadata);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"metadata_mask = %llx",nschain_repository_entry_p->metadata_mask);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %d",nschain_repository_entry_p->pkt_origin);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_type    = %d",nschain_repository_entry_p->nw_type);

  
  if(nschain_repository_entry_p->pkt_origin == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %s"," VM_APPLN");
  }
  else if (nschain_repository_entry_p->pkt_origin == 1)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %s"," VM_NS");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"pkt_origin = %s"," NW ");
  }
  if(nschain_repository_entry_p->ns_chain_b == 1)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"ns_chain_b = %s"," TRUE");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"ns_chain_b = %s"," FALSE");
  }
  if(nschain_repository_entry_p->more_nf_b == 1)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"more_nf_b = %s"," TRUE");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"more_nf_b = %s"," FALSE");
  }  
  if(nschain_repository_entry_p->service_chaining_type == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"service_chaining_type = %s"," NONE");
  }
  else if(nschain_repository_entry_p->service_chaining_type == 1)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"service_chaining_type = %s"," L2");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"service_chaining_type = %s"," L3");
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan_id_pkt    = %d",nschain_repository_entry_p->vlan_id_pkt);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"match_vlan_id  = %d",nschain_repository_entry_p->match_vlan_id);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_vlan_id   = %d",nschain_repository_entry_p->next_vlan_id);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"out_port_no    = %d",nschain_repository_entry_p->out_port_no);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_table_id  = %d",nschain_repository_entry_p->next_table_id); 
 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"flow_mod_priority = %d",nschain_repository_entry_p->flow_mod_priority);
}
/*****************************************************************************************************************************************/
#if 1
int32_t tsc_send_all_flows_to_all_tsas(struct nschain_repository_entry* nschain_repository_entry_p)
{
  uint8_t  nw_type,no_of_nwservice_instances_1,no_of_nwservice_instances_2;
  uint8_t  use_dstmac;
  uint16_t serviceport;
  uint32_t no_of_nwbr_ports;
  uint32_t retval = OF_SUCCESS;

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
  serviceport  = nschain_repository_entry_p->serviceport;

  if(cn_bind_entry_p == NULL) 
  {
    printf("\r\n cn_bind_entry is NULL");
    CNTLR_RCU_READ_LOCK_RELEASE();
    printf("\r\n PPH-52");
    return OF_SUCCESS;
  }
   
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"cn_bind_entry   = %x",cn_bind_entry_p); 
  no_of_nwservice_instances_1  = (cn_bind_entry_p->nwservice_info).no_of_nwservice_instances;
  no_of_nwservice_instances_2  = no_of_nwservice_instances_1; 
  nw_services_p = (struct  nw_services_to_apply *)(cn_bind_entry_p->nwservice_info.nw_services_p);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"no_of_nwservice_instances = %d",no_of_nwservice_instances_1);
 
  printf("\r\nNS= %d",no_of_nwservice_instances_1);
  if(no_of_nwservice_instances_1 ==  0)
  { 
    CNTLR_RCU_READ_LOCK_RELEASE();
    printf("\r\n PPH-53");
    return OF_FAILURE;
  }
  first_service_sel_1_p   = nw_services_p; /* Assumed that first service is present */
  first_service_sel_2_p   = (struct nw_services_to_apply*)(nw_services_p + (no_of_nwservice_instances_1 - 1)); 

  if(nschain_repository_entry_p->vm_and_service1_not_on_same_switch == 1)
  {
      printf("NOT-SSW");
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
      tsc_all_flows_p->selector_type = SELECTOR_PRIMARY;
      tsc_all_flows_p->table_no      = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
      tsc_all_flows_p->pkt_origin    = NW_BROADCAST_SIDE;  
      tsc_all_flows_p->dp_handle     = first_nwservice_sel_1_instance_p->system_br_saferef;
      tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_in;
      tsc_all_flows_p->next_vlan_id  = first_nwservice_sel_1_instance_p->vlan_id_in;
      tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_in;
      tsc_all_flows_p->out_port_no   = first_nwservice_sel_1_instance_p->port_id;
      tsc_all_flows_p->copy_local_mac_addresses_b       = TRUE;
      tsc_all_flows_p->copy_original_mac_addresses_b    = FALSE;
      tsc_all_flows_p->nw_port_b     = FALSE;

      /* F9 Flow */   
      retval = tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
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

        tsc_all_flows_p->selector_type = SELECTOR_PRIMARY;
        tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->pkt_origin    = VM_NS;  /* F3 Flow */ 
        tsc_all_flows_p->dp_handle     = first_nwservice_sel_1_instance_p->system_br_saferef; 
        tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port_id;
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_out;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_out;
        tsc_all_flows_p->copy_local_mac_addresses_b     = FALSE;
        tsc_all_flows_p->copy_original_mac_addresses_b  = TRUE;
        tsc_all_flows_p->nw_port_b                      = TRUE;                           
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
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
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
       
        tsc_all_flows_p->selector_type = SELECTOR_PRIMARY;
        tsc_all_flows_p->table_no      = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
        tsc_all_flows_p->pkt_origin    = NW_BROADCAST_SIDE; 
        tsc_all_flows_p->dp_handle     = next_nwservice_sel_1_instance_p->system_br_saferef;
        tsc_all_flows_p->match_vlan_id = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port_id;
        tsc_all_flows_p->copy_local_mac_addresses_b     = TRUE;
        tsc_all_flows_p->copy_original_mac_addresses_b  = FALSE;
        tsc_all_flows_p->nw_port_b                      = FALSE;

        /* F4 Flow */ 
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
                                    tsc_all_flows_p,
                                    next_nwservice_sel_1_instance_p,
                                    &repository_entry_1_p);
      }
      else
      {
        /* Two services are on same switch. Push two flows F1 and F2 to table 1  */ 
        /* Both the services contain the same dp_handle as br-int bridge is used */
        
        tsc_all_flows_p->selector_type = SELECTOR_PRIMARY;
        tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->pkt_origin    = VM_NS;
        tsc_all_flows_p->dp_handle     = first_nwservice_sel_1_instance_p->system_br_saferef; 
        tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port_id;
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_out;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in; 
        tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_out;
        tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port_id;
        tsc_all_flows_p->copy_local_mac_addresses_b       = FALSE;
        tsc_all_flows_p->copy_original_mac_addresses_b    = FALSE;
        tsc_all_flows_p->nw_port_b                        = FALSE;

        /* F1 Flow */  
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p, 
                                    tsc_all_flows_p,
                                    first_nwservice_sel_1_instance_p,
                                    &repository_entry_1_p);

        tsc_all_flows_p->selector_type = SELECTOR_PRIMARY;
        tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->pkt_origin    = VM_NS;
        tsc_all_flows_p->dp_handle     = first_nwservice_sel_1_instance_p->system_br_saferef;
        tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port_id;
        tsc_all_flows_p->match_vlan_id = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_1_instance_p->vlan_id_in;
        tsc_all_flows_p->out_port_no   = next_nwservice_sel_1_instance_p->port_id;
        tsc_all_flows_p->copy_local_mac_addresses_b       = FALSE;
        tsc_all_flows_p->copy_original_mac_addresses_b    = FALSE;
        tsc_all_flows_p->nw_port_b                        = FALSE;

        
        /* F2 Flow */
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
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
    tsc_all_flows_p->selector_type = SELECTOR_PRIMARY;
    tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
    tsc_all_flows_p->pkt_origin    = VM_NS;
    tsc_all_flows_p->dp_handle     = first_nwservice_sel_1_instance_p->system_br_saferef;
    tsc_all_flows_p->inport_id     = first_nwservice_sel_1_instance_p->port_id;
    tsc_all_flows_p->match_vlan_id = first_nwservice_sel_1_instance_p->vlan_id_out;
    tsc_all_flows_p->next_vlan_id  = 0;
    tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_1_instance_p->vlan_id_out;
    tsc_all_flows_p->out_port_no   = 0;
    tsc_all_flows_p->copy_local_mac_addresses_b      = FALSE;
    tsc_all_flows_p->copy_original_mac_addresses_b   = TRUE;
    tsc_all_flows_p->nw_port_b                       = FALSE;

    /* F6 Flow */
    tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
                                tsc_all_flows_p,
                                first_nwservice_sel_1_instance_p,
                                &repository_entry_1_p);
    /*
    use_dstmac = TRUE;

    tsc_nsc_send_proactive_flow_table_3(nschain_repository_entry_p,
                                        first_nwservice_sel_1_instance_p,
                                        use_dstmac);
    */ 
    use_dstmac = FALSE;
    tsc_nsc_send_proactive_flow_table_3(nschain_repository_entry_p,
                                        first_nwservice_sel_1_instance_p,
                                        use_dstmac);
                                           
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
      
        tsc_all_flows_p->selector_type = SELECTOR_SECONDARY;
        tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->pkt_origin    = VM_NS;
        tsc_all_flows_p->dp_handle     = first_nwservice_sel_2_instance_p->system_br_saferef;
        tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port_id;
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_in;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_in;
        tsc_all_flows_p->copy_local_mac_addresses_b       = FALSE;
        tsc_all_flows_p->copy_original_mac_addresses_b    = TRUE;
        tsc_all_flows_p->nw_port_b                        = TRUE;

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
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
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

        tsc_all_flows_p->selector_type = SELECTOR_SECONDARY;
        tsc_all_flows_p->table_no      = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
        tsc_all_flows_p->pkt_origin    = NW_BROADCAST_SIDE;
        tsc_all_flows_p->dp_handle     = next_nwservice_sel_2_instance_p->system_br_saferef;
        tsc_all_flows_p->match_vlan_id = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_2_instance_p->vlan_id_out ;
        tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port_id;
        tsc_all_flows_p->copy_local_mac_addresses_b       = TRUE;
        tsc_all_flows_p->copy_original_mac_addresses_b    = FALSE;
        tsc_all_flows_p->nw_port_b                        = FALSE;
        
        /* F4 Flow */         
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
                                    tsc_all_flows_p,
                                    next_nwservice_sel_2_instance_p,
                                    &repository_entry_1_p);
      }

      else
      {
        /* Two services are on same switch. Push two flows F1 to table 1 */
        /* Both the services contain the same dp_handle as br-int bridge is used */
        /* first_nwservice_instance_p->system_br_saferef */
      
        tsc_all_flows_p->selector_type = SELECTOR_SECONDARY;
        tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->pkt_origin    = VM_NS;
        tsc_all_flows_p->dp_handle     = first_nwservice_sel_2_instance_p->system_br_saferef;    
        tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port_id;
        tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_in;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_in;
        tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port_id;
        tsc_all_flows_p->copy_local_mac_addresses_b       = FALSE;
        tsc_all_flows_p->copy_original_mac_addresses_b    = FALSE;
        tsc_all_flows_p->nw_port_b                        = FALSE;

        /* F1 Flow */  
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
                                    tsc_all_flows_p, 
                                    first_nwservice_sel_2_instance_p,
                                    &repository_entry_1_p);
      
        tsc_all_flows_p->selector_type = SELECTOR_SECONDARY;
        tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
        tsc_all_flows_p->pkt_origin    = VM_NS;
        tsc_all_flows_p->dp_handle     = first_nwservice_sel_2_instance_p->system_br_saferef;
        tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port_id;
        tsc_all_flows_p->match_vlan_id = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->next_vlan_id  = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->vlan_id_pkt   = next_nwservice_sel_2_instance_p->vlan_id_out;
        tsc_all_flows_p->out_port_no   = next_nwservice_sel_2_instance_p->port_id;
        tsc_all_flows_p->copy_local_mac_addresses_b       = FALSE;
        tsc_all_flows_p->copy_original_mac_addresses_b    = FALSE;
        tsc_all_flows_p->nw_port_b                        = FALSE;

        /* F2 Flow */
        tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
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

    tsc_all_flows_p->selector_type = SELECTOR_SECONDARY;
    tsc_all_flows_p->table_no      = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
    tsc_all_flows_p->pkt_origin    = VM_NS;
    tsc_all_flows_p->dp_handle     = first_nwservice_sel_2_instance_p->system_br_saferef;  
    tsc_all_flows_p->inport_id     = first_nwservice_sel_2_instance_p->port_id;
    tsc_all_flows_p->match_vlan_id = first_nwservice_sel_2_instance_p->vlan_id_in;
    tsc_all_flows_p->next_vlan_id  = 0;
    tsc_all_flows_p->vlan_id_pkt   = first_nwservice_sel_2_instance_p->vlan_id_in;
    tsc_all_flows_p->out_port_no   = 0;
    tsc_all_flows_p->copy_local_mac_addresses_b      = FALSE;
    tsc_all_flows_p->copy_original_mac_addresses_b   = TRUE;
    tsc_all_flows_p->nw_port_b                       = FALSE;
     
    /* This F6 Flow is giving problems if we push.pushing looks to be successful. */
#if 1
    tsc_nsc_send_proactive_flow(nschain_repository_entry_p,
                                tsc_all_flows_p,
                                first_nwservice_sel_2_instance_p,
                                &repository_entry_1_p);
#endif     
    /* Add a flow to Table 3 */
     
     use_dstmac = TRUE;
     tsc_nsc_send_proactive_flow_table_3(nschain_repository_entry_p,
                                         first_nwservice_sel_2_instance_p,
                                         use_dstmac);
#if 0
     use_dstmac = FALSE;

     tsc_nsc_send_proactive_flow_table_3(nschain_repository_entry_p,
                                         first_nwservice_sel_2_instance_p,
                                         use_dstmac);
#endif
  }    
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
}
#endif
int32_t tsc_nsc_send_proactive_flow(struct    nschain_repository_entry* nschain_repository_entry_first_p,
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

  //printf("nw_type = %d, nid = %d \r\n",nw_type,nid);
 
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

  nsc_repository_entry_p->selector.ethernet_type = nschain_repository_entry_first_p->selector.ethernet_type;
  nsc_repository_entry_p->selector.protocol      = nschain_repository_entry_first_p->selector.protocol;

  if(tsc_flow_p->selector_type == SELECTOR_PRIMARY)
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
  else if (tsc_flow_p->selector_type == SELECTOR_SECONDARY)
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
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"T1 or T2 nsc repository entry is already existing,skipping proactive push");
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
 
  if(tsc_flow_p->selector_type == SELECTOR_PRIMARY)
  {
    nsc_repository_entry_p->selector.other_selector_p = (nschain_repository_entry_first_p->selector.other_selector_p);
    for(ii=0; ii<6; ii++)
    {
      nsc_repository_entry_p->local_in_mac[ii]      = nschain_repository_entry_first_p->local_in_mac[ii];
      nsc_repository_entry_p->local_out_mac[ii]     = nschain_repository_entry_first_p->local_out_mac[ii];
      nsc_repository_entry_p->selector.dst_mac[ii]  = nschain_repository_entry_first_p->selector.dst_mac[ii];
      nsc_repository_entry_p->selector.src_mac[ii]  = nschain_repository_entry_first_p->selector.src_mac[ii];
    }  
    nsc_repository_entry_p->selector.dst_mac_high   = nschain_repository_entry_first_p->selector.dst_mac_high; 
    nsc_repository_entry_p->selector.dst_mac_low    = nschain_repository_entry_first_p->selector.dst_mac_low; 
    nsc_repository_entry_p->selector.src_mac_high   = nschain_repository_entry_first_p->selector.src_mac_high; 
    nsc_repository_entry_p->selector.src_mac_low    = nschain_repository_entry_first_p->selector.src_mac_low; 
  }
  else if(tsc_flow_p->selector_type == SELECTOR_SECONDARY)
  {
    nsc_repository_entry_p->selector.other_selector_p = (&(nschain_repository_entry_first_p->selector));  
    for(ii=0; ii<6; ii++)
    {
      nsc_repository_entry_p->local_in_mac[ii]     = nschain_repository_entry_first_p->local_out_mac[ii];
      nsc_repository_entry_p->local_out_mac[ii]    = nschain_repository_entry_first_p->local_in_mac[ii];
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
  nsc_repository_entry_p->metadata      = (nschain_repository_entry_first_p->metadata & 0xF0FFFFFFFFFFFFFF);
  nsc_repository_entry_p->serviceport   = (uint16_t)((nschain_repository_entry_first_p->metadata) >> SERVICE_PORT_OFFSET);
  nsc_repository_entry_p->metadata_mask = 0xF0FFFFFFFFFFFFFF;
  nsc_repository_entry_p->pkt_origin    = tsc_flow_p->pkt_origin;
  nsc_repository_entry_p->dp_handle     = tsc_flow_p->dp_handle;
 
  nsc_repository_entry_p->flow_mod_priority = 2;
  nsc_repository_entry_p->skip_this_entry   = 0;
  
  nsc_repository_entry_p->next_vlan_id                  = tsc_flow_p->next_vlan_id;  
  nsc_repository_entry_p->match_vlan_id                 = tsc_flow_p->match_vlan_id; 
  nsc_repository_entry_p->vlan_id_pkt                   = tsc_flow_p->vlan_id_pkt; 
  nsc_repository_entry_p->selector.vlan_id_pkt          = tsc_flow_p->vlan_id_pkt; 
  nsc_repository_entry_p->copy_local_mac_addresses_b    = tsc_flow_p->copy_local_mac_addresses_b; 
  nsc_repository_entry_p->copy_original_mac_addresses_b = tsc_flow_p->copy_original_mac_addresses_b; 

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
                                                                          nid,&crm_port_handle);

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
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Successfully sent Flow Mod message to Table 1 proactively ");
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
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Successfully sent Flow Mod message to Table 2 proactively ");
    }    
  }
  retval = OF_SUCCESS;
  return retval;
}
int32_t tsc_nsc_send_proactive_flow_table_3(struct    nschain_repository_entry* nschain_repository_entry_first_p,
                                            struct    nwservice_instance *nw_service_p,
                                            uint8_t   use_dstmac )
{
  uint64_t  crm_port_handle,vn_handle,dp_handle,metadata;
  uint32_t  offset,index,magic,hashkey,hashmask;
  uint32_t  dst_mac_high,dst_mac_low,nid;
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
  nid     =  nschain_repository_entry_first_p->nid;

  metadata     = (nschain_repository_entry_first_p->metadata & 0xF0FFFFFFFFFFFFFF);
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

     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"table_3 ucastpkt_outport repository entry found,skipping proactive push");
     //printf("\r\n table_3 ucastpkt_outport repository entry found,skipping proactive push");  
     //printf("dst_mac_low = %x",dst_mac_low);
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
  
  //printf("\r\nT3 procpush dst_mac_low = %x",dst_mac_low);

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

   ucastpkt_outport_repository_entry_p->priority = 2;
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
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR," Successfully sent Flow Mod message to Table 3 proactively ");
   }    
   return OF_SUCCESS;
}
/****************************************************************************************************************************************/
