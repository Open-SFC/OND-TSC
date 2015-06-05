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

/* File: tsc_nsc_core.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains implementation of service chaining core.
 * Traffic is load balanced onto multiple service instances at connection level using round robin algorithm. 
 * NSM TBD
 * Launching new service instances based on traffic load/number of connections is not supported in this version.
 * De-Launching of a service is not completely immplemented eventhough it may not disturb new connections.
*/

#include "controller.h"
#include "tsc_controller.h"
#include "dobhash.h"
#include "cntrlappcmn.h"
#include "dprm.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "nsrm.h"
#include "nsrmldef.h"
#include "tsc_nvm_modules.h"

uint32_t nsc_no_of_cnbind_table_buckets_g;
uint32_t vn_nsc_info_offset_g;   /* offset for nsc info in vn structure in bytes */
extern struct tsc_nvm_module_interface tsc_nvm_modules[SUPPORTED_NW_TYPE_MAX + 1];
uint32_t nsc_no_of_repository_table_1_buckets_g;
uint32_t nsc_no_of_repository_table_2_buckets_g;
uint32_t nsc_no_of_repository_table_3_buckets_g;
struct   nwservice_instance* testnwservice_instance_p;

cntlr_lock_t global_mac_lock;

int32_t nsc_get_matching_nschain_object(struct    nsc_selector_node* selector_p,
                                        uint8_t   nw_type,
                                        uint32_t  nid,
                                        uint64_t* selection_rule_handle_p,
                                        uint64_t* nschain_object_handle_p,
                                        uint64_t* nschainset_object_handle_p);

int32_t nsc_get_matching_bypass_rule(struct    nsc_selector_node* selector_p,
                                     uint64_t  nschain_object_handle,
                                     uint64_t* bypass_rule_handle_p);

int32_t nsc_get_nwservice_instances(uint64_t  nschain_object_handle,
                                    uint64_t  bypass_rule_handle,
                                    uint8_t*  no_of_nwservice_objects_p,
                                    struct    nw_services_to_apply** ns_instances_p_p);

extern void tsc_nsc_cnbind_sel_free(struct rcu_head* selector_p);
uint8_t  nsrmdb_current_state_g; /* updated only by NSRM Notification callback */

/*****************************************************************************************************************************************
Function: nsc_lookup_l2_nschaining_info
Input Parameters:
	pkt_in:  Miss packet received from datapath via controller.
        nw_type: VXLAN or NVGRE
        nid:     Network identifier
Output Parameters:
	cn_bind_entry_p:This structure is filled with the obtained service chaining info.  
Description: cnbind entry contains the two selectors. selector_1 is used to obtain the service chianing info.
*****************************************************************************************************************************************/
int32_t nsc_lookup_l2_nschaining_info(struct   ofl_packet_in* pkt_in,
                                      uint8_t  nw_type,
                                      uint32_t nid, 
                                      struct   l2_connection_to_nsinfo_bind_node* cn_bind_entry_p)
{
  uint8_t  vn_nsrmdb_lookup_state;    
  int32_t  retval,status;

  status = OF_FAILURE;

  do
  { 
    vn_nsrmdb_lookup_state = nsrmdb_current_state_g;

    status = nsc_get_matching_nschain_object(&cn_bind_entry_p->selector_1,
                                             nw_type,
                                             nid,    
                                             &(cn_bind_entry_p->nwservice_info.selection_rule_handle),
                                             &(cn_bind_entry_p->nwservice_info.nschain_object_handle),
                                             &(cn_bind_entry_p->nwservice_info.nschainset_object_handle)
                                            );
    if(status != OF_SUCCESS) 
    {
      if(vn_nsrmdb_lookup_state !=  nsrmdb_current_state_g) 
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Lookup failed.Look up for NSCHAIN OBJECT again as NSRM database changed");
        continue;
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Lookup failed.Return as NSRM database is not changed"); 
        break; 
     
        /* In case one of the following errors is returned and if NSRM Database is changed in between,
             - Do a re-lookup of NSRM database. Otherwise return the error. 
     
           NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN,
           NSC_CHAINSET_NOT_CONFIGURED,
           NSC_NO_SELECTION_RULE_FOUND.
           NSC_INVALID_NSCHAIN_NAME,
     
           OF_FAILURE - NSC_INVALID_VIRTUAL_NETWORK 
        */
      } 
    } 
    /* OF_SUCCESS */

    status = nsc_get_matching_bypass_rule(&cn_bind_entry_p->selector_1,
                                          cn_bind_entry_p->nwservice_info.nschain_object_handle,
                                          &(cn_bind_entry_p->nwservice_info.bypass_rule_handle)
                                         );
      
    if(status != OF_SUCCESS)
    {
      if(vn_nsrmdb_lookup_state !=  nsrmdb_current_state_g )
      {  
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Lookup failed.Return as NSRM database is changed")
        continue;
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Lookup failed.Return as NSRM database is not changed");
        break;
        
        /* In case one of the following errors is returned and if NSRM Database is changed in between,
             - Do a re-lookup of NSRM database. Otherwise return the error.

           NSC_INVALID_NSCHAIN_HANDLE
        */
      }
    }
    /* OF_SUCCESS could be NSC_NO_BYPASS_RULE_FOUND also 
       In that case cn_bind_entry_p->nwservice_info.bypass_rule_handle will be 0. */

    /* This function allocates the required memory for returning instance handles in nw_services_p */ 
  
    cn_bind_entry_p->nwservice_info.no_of_nwservice_instances = 0;
    if(cn_bind_entry_p->nwservice_info.nw_services_p != NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"If nsc_get_nwservice_instances API is called second time due to database change, free the memery before calling again"); 
      free(cn_bind_entry_p->nwservice_info.nw_services_p);
    }     
   
    status = nsc_get_nwservice_instances(cn_bind_entry_p->nwservice_info.nschain_object_handle,
                                         cn_bind_entry_p->nwservice_info.bypass_rule_handle,
                                         &(cn_bind_entry_p->nwservice_info.no_of_nwservice_instances),
                                         &(cn_bind_entry_p->nwservice_info.nw_services_p)
                                        );
    if(status != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrm_get_nwservice_instances() returned other than OF_SUCCESS");
    
      /* OF_FAILURE, memory allocation failure.
         NSC_INVALID_NSCHAIN_HANDLE,
         NSC_NO_NW_SERVICES_FOUND,
         NSC_INVALID_BYPASS_HANDLE 
      */
    }
    /* OF_SUCCESS */

  }while(vn_nsrmdb_lookup_state !=  nsrmdb_current_state_g);
  
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"cn_bind_entry_p->nwservice_info.nw_services_p : %p",cn_bind_entry_p->nwservice_info.nw_services_p); 
  
  if(status == OF_SUCCESS)
  {
    cn_bind_entry_p->nsc_config_status = NSC_NWSERVICES_CONFIGURED; 
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG," NSC_NWSERVICES_CONFIGURED - NSRM DB Lokup Successful");
    retval = OF_SUCCESS;
  }
  else if(status == NSC_NO_NW_SERVICES_FOUND)
  {
    cn_bind_entry_p->nsc_config_status = NSC_NWSERVICES_NOT_CONFIGURED;
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"NSC_NWSERVICES_NOT_CONFIGURED - NSRM DB Lokup Successful");
    retval = OF_SUCCESS;
  }
  else if(status ==  NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN)
  {
    cn_bind_entry_p->nsc_config_status = NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN;
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN - NSRM DB Lokup Successful");
    retval = OF_SUCCESS;
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Either incomplete nschain configuration or systemic failure");
    if(cn_bind_entry_p != NULL)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"cn_bind_entry_p is not NULL.");
      cn_bind_entry_p->nsc_config_status  = status; 
      cn_bind_entry_p->nsc_config_status  =  NSC_INCOMPLETE_NSCHAIN_CONFIGURATION;
    }
    /* NSC_CHAINSET_NOT_CONFIGURED, 
       NSC_INVALID_NSCHAIN_HANDLE,
       NSC_NO_SELECTION_RULE_FOUND, 
       NSC_INVALID_BYPASS_HANDLE, 
       NSC_INVAILD_NSCHAIN_NAME,
       OF_FAILURE
    */
    retval = OF_FAILURE; 
  } 
    
  /* 1. We always add selectors even if NSRM database is not lookedup or lookup is not successful and allow traffic without any nw_services in between.
        A return value of OF_SUCCESS may result in two cases
            cn_bin_entry_p->nsc_config_status->NSC_NSRM_CONFIG_LOOKUP_FAILURE results in adding normal flows for such connections.
            cn_bin_entry_p->nsc_config_status->NSC_NSRM_CONFIG_LOOKUP_SUCCESS results in adding NSC flows for such connections.
            cn_bind_entry_p->fw_action == NSC_FW_DENIED results in adding normal flows with deny or drop action.      
        A return value of OF_FAILURE results in dropping the packet by this module itself which is rare.

     2. If L2 Service Chaining is NOT configured for a given VN i.e. if(vn_nsc_info_p->service_chaining_type != L2_SERVICE_CHAINING),
        we still add selectors without adding nw_services.This is to track different connections of traffic.

     3. Later we may allow modules like Firewall and NAT to register with nsc_cnbind module to apply their rules on the traffic.
        Then we can add Firewall rules such that a given traffic can be dropped in switches.We need special OF actions to drop traffic.  

     4. Alternately Admin may add default objects and rules to allow traffic without any network services.In that case unmatched traffic can be dropped.
        We need special OF actions to drop traffic.    
  */

  return retval; 
  
  /* cn_bind_entry contains the network services to be applied for each direction. */ 
  /* Based on selector type i.e. PRIMARY or SECONDARY selector that matched, the caller may select the nw service order */
}
/******************************************************************************************************************
Function Name: nsc_get_matching_nschain_object
Input Parameters:
	selector_p:Selector_1 containing the selector fields from first packet of the connection. 
        nw_type:   Network Type
        nid:       Network Identifier 
Output Parameters:
        selection_rule_handle_p   :Handle to matching selection rule  in NSRM database.
        nschain_object_handle_p   :Handle to matching nschain object  in NSRM database. 
        nschainset_object_handle_p:Handle to the nschainset object for the L2 network in NSRM database. 
Description:This fuction performs a NSRM database lookup into selection rule database to obtain matching selection 
            rule and there by nschain object.It also obtains nschainset object using the L2 network. 
******************************************************************************************************************/
int32_t nsc_get_matching_nschain_object(struct    nsc_selector_node* selector_p,
                                        uint8_t   nw_type,
                                        uint32_t  nid,
                                        uint64_t* selection_rule_handle_p,
                                        uint64_t* nschain_object_handle_p,
                                        uint64_t* nschainset_object_handle_p)
{
   struct    vn_service_chaining_info* vn_nsc_info_p;
   struct    crm_virtual_network*      vn_entry_p;
   uint64_t  vn_handle;
   uint64_t  srf_nschainset_object;   

   struct nsrm_nschainset_object*      nschainset_object_p = NULL;
   struct nsrm_nschain_selection_rule* selection_rule_scan_node_p = NULL;
   struct nsrm_l2nw_service_map*       l2_service_map_record_p = NULL;

   struct   nsrm_nschain_object* nschain_object_p;
   uint64_t hashkey,offset;
   uint8_t  match_found_b;
   int32_t  retval;

   uint32_t* srule_src_mac_high_p;
   uint32_t* srule_src_mac_low_p;
   uint32_t* srule_dst_mac_high_p;
   uint32_t* srule_dst_mac_low_p;

   do
   {
     retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
     if(retval != OF_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn handle");       
       return OF_FAILURE;
     } 
     retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
     if(retval != OF_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to get vn handle");
       return OF_FAILURE;
     }
     CNTLR_RCU_READ_LOCK_TAKE();

     vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */

     if(vn_nsc_info_p->service_chaining_type == SERVICE_CHAINING_NONE)
     {
       OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN");
       CNTLR_RCU_READ_LOCK_RELEASE(); 
       return NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN;
     }
     
     retval = nsrm_get_nschainset_byhandle(vn_nsc_info_p->srf_nschainset_object,&nschainset_object_p);
     if(retval == NSRM_SUCCESS)
       break;
     
     retval = nsrm_get_l2_smap_record_byhandle(vn_nsc_info_p->srf_service_map_record,&l2_service_map_record_p);
     if(retval == NSRM_SUCCESS)
     {
       srf_nschainset_object = l2_service_map_record_p->nschainset_object_saferef;
       retval = nsrm_get_nschainset_byhandle(srf_nschainset_object,&nschainset_object_p);
       if(retval == NSRM_SUCCESS)
       {
         vn_nsc_info_p->srf_nschainset_object = srf_nschainset_object;
         *nschainset_object_handle_p          = srf_nschainset_object;
         break;
       }
     }
     
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No CHAINSET configured for the network.");
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSC_CHAINSET_NOT_CONFIGURED;

   }while(0);

   match_found_b = FALSE;
   hashkey = 0;
   offset  = NSRM_SELECTION_RULE_TBL_OFFSET;
   
   if(nschainset_object_p->nschain_selection_rule_set == 0)
   {
     CNTLR_RCU_READ_LOCK_RELEASE();
     printf("\r\n nschainset_object_p->nschain_selection_rule_set is 0\r\n");
     return NSC_CHAINSET_NOT_CONFIGURED;
   }    
   MCHASH_TABLE_SCAN(nschainset_object_p->nschain_selection_rule_set,hashkey)
   {
      MCHASH_BUCKET_SCAN(nschainset_object_p->nschain_selection_rule_set,hashkey,
                         selection_rule_scan_node_p,
                         struct nsrm_nschain_selection_rule*, offset)
     {
        if((selection_rule_scan_node_p->sip.ip_object_type_e != IP_ANY) && 
           ((selector_p->src_ip < (selection_rule_scan_node_p->sip.ipaddr_start)) ||
            (selector_p->src_ip > (selection_rule_scan_node_p->sip.ipaddr_end))
           )
          )
        {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"pkt_src_ip : %x,start : %x ,end : %x",selector_p->src_ip,selection_rule_scan_node_p->sip.ipaddr_start,selection_rule_scan_node_p->sip.ipaddr_end);  
          continue;
        }
        if((selection_rule_scan_node_p->dip.ip_object_type_e != IP_ANY) &&
           ((selector_p->dst_ip < (selection_rule_scan_node_p->dip.ipaddr_start))  ||
            (selector_p->dst_ip > (selection_rule_scan_node_p->dip.ipaddr_end))
           )
          )
        {
           OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"pkt_dst_ip : %x,start : %x ,end : %x",selector_p->dst_ip,selection_rule_scan_node_p->dip.ipaddr_start,selection_rule_scan_node_p->dip.ipaddr_end);
           continue;
        }       
        if((selection_rule_scan_node_p->sp.port_object_type_e != PORT_ANY) &&  
           ((selector_p->src_port < (selection_rule_scan_node_p->sp.port_start))  ||
            (selector_p->src_port > (selection_rule_scan_node_p->sp.port_end))
           )
          ) 
        {
           OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"src_port : %d , start : %d ,end : %d",selector_p->src_port,selection_rule_scan_node_p->sp.port_start,selection_rule_scan_node_p->sp.port_end);
           continue;
        }
  
        if((selection_rule_scan_node_p->dp.port_object_type_e != PORT_ANY) &&
           ((selector_p->dst_port < (selection_rule_scan_node_p->dp.port_start))    ||
            (selector_p->dst_port > (selection_rule_scan_node_p->dp.port_end))
           )
          )
        {
           OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"dst_port : %d , start : %d ,end : %d",selector_p->dst_port,selection_rule_scan_node_p->dp.port_start,selection_rule_scan_node_p->dp.port_end);
           continue;
        }
        
        srule_src_mac_high_p = (uint32_t *)(selection_rule_scan_node_p->src_mac.mac + 0);
        srule_src_mac_low_p  = (uint32_t *)(selection_rule_scan_node_p->src_mac.mac + 2);
        srule_dst_mac_high_p = (uint32_t *)(selection_rule_scan_node_p->dst_mac.mac + 0);
        srule_dst_mac_low_p  = (uint32_t *)(selection_rule_scan_node_p->dst_mac.mac + 2);

        if((selection_rule_scan_node_p->src_mac.mac_address_type_e != MAC_ANY) && 
           ((selector_p->src_mac_low  != (ntohl)(*srule_src_mac_low_p)) ||
            (selector_p->src_mac_high != (ntohl)(*srule_src_mac_high_p))
           )
          )
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"src_mac_low : %x ,srule_src_mac_low : %x ", selector_p->src_mac_low ,*srule_src_mac_low_p);
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"src_mac_high : %x ,srule_src_mac_high : %x ", selector_p->src_mac_high ,*srule_src_mac_high_p);
          continue;
        }        

        if((selection_rule_scan_node_p->dst_mac.mac_address_type_e != MAC_ANY) && 
           ((selector_p->dst_mac_low  != (ntohl)(*srule_dst_mac_low_p)) ||
            (selector_p->dst_mac_high != (ntohl)(*srule_dst_mac_high_p))  
           )
          )
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"dst_mac_low :  %x ,srule_dst_mac_low : %x ", selector_p->dst_mac_low ,*srule_dst_mac_low_p);
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"dst_mac_high : %x ,srule_dst_mac_high : %x ", selector_p->dst_mac_high ,*srule_dst_mac_high_p); 
          continue;
        }
       
        if(selector_p->ethernet_type != (selection_rule_scan_node_p->eth_type.ethtype))
        {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"selector_p->ethernet_type : %d ,ethtype : %d",selector_p->ethernet_type ,selection_rule_scan_node_p->eth_type.ethtype_type_e);
          continue;
        }
       match_found_b = TRUE;
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"selection rule that matched: %s",selection_rule_scan_node_p->name_p);
       break;
     }
    break;
   }

   if(match_found_b == FALSE)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No matching selection rule found");
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSC_NO_SELECTION_RULE_FOUND;
   }

   retval = nsrm_get_nschain_byhandle(selection_rule_scan_node_p->nschain_object_saferef,&nschain_object_p);
   if(retval != NSRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"invalid nscchain object handle.Lookup again with its name");
     retval = nsrm_get_nschain_object_handle(selection_rule_scan_node_p->nschain_object_name_p,&selection_rule_scan_node_p->nschain_object_saferef);
     if(retval != OF_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"A selection rule is configured with a chain name and the chain itselt is not configured or deleted");
       CNTLR_RCU_READ_LOCK_RELEASE(); 
       return NSC_INVALID_NSCHAIN_NAME;
     }
   }
   *selection_rule_handle_p = selection_rule_scan_node_p->selection_rule_handle;
   *nschain_object_handle_p = selection_rule_scan_node_p->nschain_object_saferef;

   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Exact matching selection rule found is : %s with chain : %s",selection_rule_scan_node_p->name_p,selection_rule_scan_node_p->nschain_object_name_p);
   CNTLR_RCU_READ_LOCK_RELEASE();
   return OF_SUCCESS;
}
/******************************************************************************************************************
Function Name: nsc_get_matching_bypass_rule
Input Parameters:
        selector_p           :Selector_1 containing the selector fields from first packet of the connection.
        nschain_object_handle:Handle to an nschain object in NSRM database.
Output Parameters:
        bypass_rule_handle_p :Handle to the matching bypass rule if any in NSRM database.
Description:This fuction performs a NSRM database lookup into bypass rule database to obtain matching bypass
            rule by using the input argument Handle to nschain object in NSRM database.OF_SUCCESS is returned 
            even if no matching bypass rule is found and bypass_rule_handle_p is set to 0 in this case.
******************************************************************************************************************/
int32_t nsc_get_matching_bypass_rule(struct    nsc_selector_node* selector_p,
                                     uint64_t  nschain_object_handle,
                                     uint64_t* bypass_rule_handle_p)
{
   struct nsrm_nschain_object*         nschain_object_p = NULL;
   struct nsrm_nwservice_bypass_rule*  bypass_rule_scan_node_p = NULL;

   uint64_t hashkey,offset;
   uint8_t  match_found_b;
   int32_t  retval;

   uint32_t* bprule_src_mac_high_p;
   uint32_t* bprule_src_mac_low_p;
   uint32_t* bprule_dst_mac_high_p;
   uint32_t* bprule_dst_mac_low_p;

   CNTLR_RCU_READ_LOCK_TAKE();


   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Entered");

   retval = nsrm_get_nschain_byhandle(nschain_object_handle, &nschain_object_p);
   if(retval != NSRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No chain exists");
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSC_INVALID_NSCHAIN_HANDLE;
   }

   match_found_b = FALSE;
   hashkey = 0;
   offset  = NSRM_BYPASS_RULE_TBL_OFFSET;


   if(nschain_object_p->bypass_rule_list == NULL)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Bypass Rule List is not configured");
     CNTLR_RCU_READ_LOCK_RELEASE();
     *bypass_rule_handle_p = 0;
     return OF_SUCCESS;
   } 

   //CNTLR_RCU_READ_LOCK_TAKE();
   MCHASH_TABLE_SCAN(nschain_object_p->bypass_rule_list,hashkey)
   {
     MCHASH_BUCKET_SCAN(nschain_object_p->bypass_rule_list,hashkey,
                        bypass_rule_scan_node_p,
                        struct nsrm_nwservice_bypass_rule*, offset)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"scanning next bypass rule");
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"bypass rule being scanned : %s",bypass_rule_scan_node_p->name_p);

       if((bypass_rule_scan_node_p->sip.ip_object_type_e != IP_ANY) &&
          ((selector_p->src_ip < (bypass_rule_scan_node_p->sip.ipaddr_start)) ||
           (selector_p->src_ip > (bypass_rule_scan_node_p->sip.ipaddr_end))
          )
         )
       {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"pkt_src_ip : %x,start : %x ,end : %x",selector_p->src_ip,bypass_rule_scan_node_p->sip.ipaddr_start,bypass_rule_scan_node_p->sip.ipaddr_end);
         continue;
       }

       if((bypass_rule_scan_node_p->dip.ip_object_type_e != IP_ANY) &&
          ((selector_p->dst_ip < (bypass_rule_scan_node_p->dip.ipaddr_start))  ||
           (selector_p->dst_ip > (bypass_rule_scan_node_p->dip.ipaddr_end))
          )
         )
       {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"pkt_dst_ip : %x,start : %x ,end : %x",selector_p->dst_ip,bypass_rule_scan_node_p->dip.ipaddr_start,bypass_rule_scan_node_p->dip.ipaddr_end);
         continue;
       }

       if((bypass_rule_scan_node_p->sp.port_object_type_e != PORT_ANY) &&
          ((selector_p->src_port < (bypass_rule_scan_node_p->sp.port_start))  ||
           (selector_p->src_port > (bypass_rule_scan_node_p->sp.port_end))
          )
         )
       {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"src_port : %d , start : %d ,end : %d",selector_p->src_port,bypass_rule_scan_node_p->sp.port_start,bypass_rule_scan_node_p->sp.port_end);
         continue;
       }

       if((bypass_rule_scan_node_p->dp.port_object_type_e != PORT_ANY) &&
          ((selector_p->dst_port < (bypass_rule_scan_node_p->dp.port_start))    ||
           (selector_p->dst_port > (bypass_rule_scan_node_p->dp.port_end))
          )
         )
       {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"dst_port : %d , start : %d ,end : %d",selector_p->dst_port,bypass_rule_scan_node_p->dp.port_start,bypass_rule_scan_node_p->dp.port_end);
         continue;
       }

       bprule_src_mac_high_p = (uint32_t *)(bypass_rule_scan_node_p->src_mac.mac + 0);
       bprule_src_mac_low_p  = (uint32_t *)(bypass_rule_scan_node_p->src_mac.mac + 2);
       bprule_dst_mac_high_p = (uint32_t *)(bypass_rule_scan_node_p->dst_mac.mac + 0);
       bprule_dst_mac_low_p  = (uint32_t *)(bypass_rule_scan_node_p->dst_mac.mac + 2);


       if((bypass_rule_scan_node_p->src_mac.mac_address_type_e != MAC_ANY) &&
          ((selector_p->src_mac_low  != (ntohl)(*bprule_src_mac_low_p)) ||
           (selector_p->src_mac_high != (ntohl)(*bprule_src_mac_high_p))
          )
         )
       {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"src_mac_low : %x ,bprule_src_mac_low : %x ", selector_p->src_mac_low ,*bprule_src_mac_low_p);
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"src_mac_high : %x ,bprule_src_mac_high : %x ", selector_p->src_mac_high ,*bprule_src_mac_high_p);
         continue;
       }

       if((bypass_rule_scan_node_p->dst_mac.mac_address_type_e != MAC_ANY) &&
          ((selector_p->dst_mac_low  != (ntohl)(*bprule_dst_mac_low_p)) ||
           (selector_p->dst_mac_high != (ntohl)(*bprule_dst_mac_high_p))
          )
         )
       {
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"dst_mac_low :  %x ,bprule_dst_mac_low : %x ", selector_p->dst_mac_low ,*bprule_dst_mac_low_p);
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"dst_mac_high : %x ,bprule_dst_mac_high : %x ", selector_p->dst_mac_high ,*bprule_dst_mac_high_p);
         continue;
       }

       if(selector_p->ethernet_type != (bypass_rule_scan_node_p->eth_type.ethtype))
       {
          OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"selector_p->ethernet_type : %d ,ethtype : %d",selector_p->ethernet_type,bypass_rule_scan_node_p->eth_type.ethtype_type_e);
          continue;
       }
       match_found_b = TRUE;
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"bypass rule that matched: %s",bypass_rule_scan_node_p->name_p);
       break;

     }
    break;
   }

   if(match_found_b == FALSE)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No matching bypass rule found");
     CNTLR_RCU_READ_LOCK_RELEASE();
     *bypass_rule_handle_p = 0;
     return OF_SUCCESS;  /* NSC_NO_BYPASS_RULE_FOUND; It is not an error */
   }
   
   *bypass_rule_handle_p = bypass_rule_scan_node_p->bypass_rule_handle;

   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Exact matching bypass rule found");
   CNTLR_RCU_READ_LOCK_RELEASE();
   return OF_SUCCESS;
}
/***************************************************************************************************************************
Function: nsrm_get_nwservice_instances
Description: This function allocates memory fro network service handle.
Input Parameters:
  nschain_object_handle: Handle to nschain_object in NSRM database.
  bypass_rule_handle   : Handle to bypass rule in NSRM database and can be NULL.
output parameters:
  no_of_nwservice_objects_p: Number of nwservice_objects configured for the nschain_object.
  ns_instances_p_p         : Double pointer to hold a pointer to an array of structure variables "nw_services_to_apply"
                             This structure contains three handles required to obtain the network service instance 
                             information for service chaining.
                             Memory is allocated by this function and the caller needs to free when required.
                             The three handles are nschain_object_handle,nwservice_instance_handle,nwservice_object_handle.
Description:This function implements round robin algorithm to select one among multiple instances of a service object
            for each new connection(cnbind_entry).It basically prepares a chian of services to apply on the pacekts of a 
            given connection.The bypass rule handle is used remove the services to be bypassed from the service chain.  
**************************************************************************************************************************/
int32_t nsc_get_nwservice_instances(uint64_t  nschain_object_handle,
                                    uint64_t  bypass_rule_handle,
                                    uint8_t*  no_of_nwservice_objects_p,
                                    struct    nw_services_to_apply** ns_instances_p_p)
{
  struct nsrm_nschain_object*       nschain_object_p = NULL;
  struct nschain_nwservice_object*  nwservice_object_scan_node_p    = NULL;
  struct nwservice_instance*        nwservice_instance_scan_node_p = NULL;

  uint32_t hashkey,offset,offset_nwservice_object;
  //uint8_t  match_found_b; 
  int32_t  retval = OF_SUCCESS;
  uint8_t  no_of_nwservices_assigned = 0;
  uint64_t nwservice_instance_handle_next = 0;
  //uint64_t nwservice_handle;
  uint8_t  nw_service_bypass,no_of_services_to_assign,no_of_nw_services_to_bypass;
  uint8_t  next_service_instance_set;
  uint8_t  nwservice_instance_count;
 
  struct nwservice_object_skip_list  *nwservice_skip_list_scan_p = NULL;
  struct nsrm_nwservice_bypass_rule* bypass_rule_p;
  struct nw_services_to_apply *nw_services_to_apply_p, *nw_services_to_apply_start_p;
 
  CNTLR_RCU_READ_LOCK_TAKE(); 
  
  retval = nsrm_get_nschain_byhandle(nschain_object_handle, &nschain_object_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No chain exists");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return NSC_INVALID_NSCHAIN_HANDLE;
  }

  if(nschain_object_p->no_of_nw_service_objects == 0)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No Network Service objects found");
    return NSC_NO_NW_SERVICES_FOUND; 
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"number of nw_services objects  = %d",nschain_object_p->no_of_nw_service_objects);

  no_of_nw_services_to_bypass = 0;

  if(bypass_rule_handle !=0)
  {
    retval = nsrm_get_bypass_rule_byhandle(nschain_object_p,bypass_rule_handle,&bypass_rule_p);
    if(retval != OF_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid bypass handle");  
      return NSC_INVALID_BYPASS_HANDLE; /* 0 refers to no bypassing required. */
    }
    else
    {
      no_of_nw_services_to_bypass = bypass_rule_p->no_of_nwservice_objects;
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"number of nw_services to bypass = %d",no_of_nw_services_to_bypass);
    }
  }
  no_of_services_to_assign = (nschain_object_p->no_of_nw_service_objects - no_of_nw_services_to_bypass);
  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"number of nw_services to assign  = %d",no_of_services_to_assign);

  if(no_of_services_to_assign == 0)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nmber of nw services to assign is 0");
    return NSC_NO_NW_SERVICES_FOUND;
  }
 
  /* Each variable stores the handles of nschain_object, nwservice_object and nwservice_instance. */
  nw_services_to_apply_p = (struct nw_services_to_apply*)(malloc((sizeof (struct nw_services_to_apply) * no_of_services_to_assign )));  
  if(nw_services_to_apply_p == NULL)
  {
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"memory allocation failure for service handles to return");
    return OF_FAILURE;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();

  nw_services_to_apply_start_p = nw_services_to_apply_p;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_services_to_apply_p = %p", nw_services_to_apply_start_p); 
  /* 2. Get the next nwservice_object in the nschain_object using bucket scan.*/

   //match_found_b = FALSE;
   hashkey = 0;

   CNTLR_RCU_READ_LOCK_TAKE();
   MCHASH_TABLE_SCAN(nschain_object_p->nwservice_objects_list,hashkey)
   {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwchain_object name = %s",nschain_object_p->name_p);
     hashkey = 0;
     offset_nwservice_object  = NSRM_NSCHAIN_NWSERVICE_OBJECT_TBL_OFFSET; 
     MCHASH_BUCKET_SCAN(nschain_object_p->nwservice_objects_list,hashkey,
                        nwservice_object_scan_node_p,
                        struct nschain_nwservice_object*,offset_nwservice_object)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\nInside nwservice_object bucket scan");

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_object name = %s", nwservice_object_scan_node_p->name_p);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"sequence number = %d", nwservice_object_scan_node_p->sequence_number);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_count = %d", nwservice_object_scan_node_p->nwservice_instance_count);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_nwservice_instance_to_be_used  = %llx", nwservice_object_scan_node_p->next_nwservice_instance_to_be_used);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_object service_object_saferef = %llx", nwservice_object_scan_node_p->nwservice_object_saferef);
       
       if(no_of_services_to_assign == 0)
         break; 
 
       if(nwservice_object_scan_node_p->nwservice_instance_count == 0)
       {  
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No network services are attached to the nwservice_object");
         continue; /* go to next service object as no service instances are added yet */
       }
       /* Check bypass rule to allow the service or not */
      
       nw_service_bypass = 0;

       if(bypass_rule_handle !=0) 
       {
         OF_LIST_SCAN(bypass_rule_p->skip_nwservice_objects, nwservice_skip_list_scan_p,
                      struct nwservice_object_skip_list *,
                      NSRM_NWSERVICE_SKIP_LISTNODE_OFFSET)
         {
           OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"*** Check if the service object to be bypassed***");
           OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"skip name :%s , service name :%s",nwservice_skip_list_scan_p->nwservice_object_name_p , nwservice_object_scan_node_p->name_p);
           
           if(nwservice_skip_list_scan_p->nwservice_object_handle == nwservice_object_scan_node_p->nwservice_object_saferef)
           { 
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"It is to be bypassed."); 
             nw_service_bypass = 1;
             break;
           }
         }
       }
       if(nw_service_bypass == 1)
       {
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Check the next nwservice_object");
         continue;
       }

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"It is not to be bypassed.");
      
       hashkey = 0; 
       MCHASH_TABLE_SCAN(nwservice_object_scan_node_p->nwservice_instance_list,hashkey)
       {
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\nInside nwservice instance table scan");
         offset = NSRM_NWSERVICE_INSTANCE_TBL_OFFSET;
         next_service_instance_set = 0;
         nwservice_instance_handle_next = nwservice_object_scan_node_p->next_nwservice_instance_to_be_used;
         nwservice_instance_count = nwservice_object_scan_node_p->nwservice_instance_count;    
         OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nwservice_instance_count = %x",nwservice_instance_count);
         MCHASH_BUCKET_SCAN(nwservice_object_scan_node_p->nwservice_instance_list,hashkey,
                            nwservice_instance_scan_node_p,
                            struct nwservice_instance*,offset)
         {
           OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Inside nwservice instance bucket scan");
           OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Network service instance name = %s",nwservice_instance_scan_node_p->name_p);
           if(next_service_instance_set == 0) 
           {
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_service_instance_set = 0");
             if(nwservice_instance_handle_next == 0)
             {
               OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_handle_next = 0");
               nwservice_instance_handle_next = nwservice_instance_scan_node_p->srf_nwservice_instance;
             }
             else if(nwservice_instance_scan_node_p->srf_nwservice_instance != nwservice_instance_handle_next)
             {
               OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," search for nwservice_instance to be assgned");
               nwservice_instance_count--; 
               continue; /* Scan until the nwservice instance to be assigned is found in round robin method. */   
             }
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Assign the service instance");
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_services_to_apply_p = %p", nw_services_to_apply_p);
             nw_services_to_apply_p->nwservice_instance_handle = nwservice_instance_handle_next;
             nw_services_to_apply_p->nwservice_object_handle   = nwservice_object_scan_node_p->nwservice_object_saferef;
             nw_services_to_apply_p->nschain_object_handle     = nschain_object_handle;
             
             retval = nsrm_get_nwservice_instance_byhandle(nw_services_to_apply_p->nschain_object_handle,
                                                          nw_services_to_apply_p->nwservice_object_handle,
                                                          nw_services_to_apply_p->nwservice_instance_handle,
                                                          &testnwservice_instance_p);

             if(retval == OF_FAILURE)
             {
               OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Failed to get the service instance.Get the next service instance");
               CNTLR_RCU_READ_LOCK_RELEASE();
               return OF_FAILURE;
             }

             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan_id_in  = %d",testnwservice_instance_p->vlan_id_in);
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan_id_out = %d",testnwservice_instance_p->vlan_id_out);
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Service Instance Name = %s",testnwservice_instance_p->name_p);
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Switch Name = %s",testnwservice_instance_p->switch_name);
      
             nw_services_to_apply_p++;
             no_of_nwservices_assigned++;            
             nwservice_instance_count--;
             no_of_services_to_assign--;
             next_service_instance_set = 1; 
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_service_instance_set = 1"); 
             if(nwservice_instance_count == 0)  
             {
               OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwservice_instance_count =  0");
               /* A service instance is just assigned. As the list reached its end, the first one in the list needs to be assigned for next assignement. */
               nwservice_object_scan_node_p->next_nwservice_instance_to_be_used = 0;
             }  
             continue;
           }
           else
           {
             OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next service_instance to be assigned is decided.Copied to nwservice_object_scan_node_p->next_nwservice_instance_to_be_used");  
             nwservice_object_scan_node_p->next_nwservice_instance_to_be_used =  nwservice_instance_scan_node_p->srf_nwservice_instance;
             break;
           }    
         }
         break; /* for  MCHASH_TABLE_SCAN(nwservice_object_scan_node_p->nwservice_instance_list,hashkey) */
       }
     }
     break; /* MCHASH_TABLE_SCAN(nschain_object_p->nwservice_objects_list,hashkey) */
   }
   CNTLR_RCU_READ_LOCK_RELEASE();
   
   *no_of_nwservice_objects_p = no_of_nwservices_assigned;
   if(no_of_nwservices_assigned)
   {
     *ns_instances_p_p = nw_services_to_apply_start_p;
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_services_to_apply_p = %p", nw_services_to_apply_p);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"number of nw_services assigned = %d",no_of_nwservices_assigned);
     retval = OF_SUCCESS;
   }
   else
   {
     retval = NSC_NO_NW_SERVICES_FOUND;
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"no network services are found to assign"); 
     free(nw_services_to_apply_p);
   }
   return retval;

   /* OF_SUCCESS,
      OF_FAILURE, memory allocation failure.
      NSC_INVALID_NSCHAIN_HANDLE, NSC_NO_NW_SERVICES_FOUND, NSC_INVALID_BYPASS_HANDLE */
}
/****************************************************************************************************************/
void nsc_get_cn_bind_table_mempoolentries(uint32_t* cn_bind_entries_max, uint32_t* cn_bind_static_entries)
{
  *cn_bind_entries_max    = NSC_MAX_CNBIND_TBL_ENTRIES;
  *cn_bind_static_entries = NSC_MAX_CNBIND_TBL_STATIC_ENTRIES;
}
/****************************************************************************************************************/
void nsc_get_repository_table_1_mempoolentries(uint32_t* nsc_repository_entries_max, uint32_t* nsc_repository_static_entries)
{
  *nsc_repository_entries_max    = NSC_MAX_REPOSITORY_TBL_1_ENTRIES;
  *nsc_repository_static_entries = NSC_MAX_REPOSITORY_TBL_1_STATIC_ENTRIES;
}
/****************************************************************************************************************/
void nsc_get_repository_table_2_mempoolentries(uint32_t* nsc_repository_entries_max, uint32_t* nsc_repository_static_entries)
{
  *nsc_repository_entries_max    = NSC_MAX_REPOSITORY_TBL_2_ENTRIES;
  *nsc_repository_static_entries = NSC_MAX_REPOSITORY_TBL_2_STATIC_ENTRIES;
}
/****************************************************************************************************************/
void nsc_get_repository_table_3_mempoolentries(uint32_t* nsc_repository_entries_max, uint32_t* nsc_repository_static_entries)
{
  *nsc_repository_entries_max    = NSC_MAX_REPOSITORY_TBL_3_ENTRIES;
  *nsc_repository_static_entries = NSC_MAX_REPOSITORY_TBL_3_STATIC_ENTRIES;
}
/****************************************************************************************************************/
void nsc_free_table_1_repository_entry_rcu(struct rcu_head *nsc_repository_entry_p)
{
  struct   nschain_repository_entry* nsc_repository_node_p;
  uint8_t  nw_type;
  uint32_t nid,offset;
  uint64_t vn_handle;
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   crm_virtual_network* vn_entry_p;
  int32_t  retval;

  if(nsc_repository_entry_p)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"About to free table 1 repository entry");
  
    offset =  NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
    nsc_repository_node_p = (struct  nschain_repository_entry  *) (((char *)nsc_repository_entry_p - (2*(sizeof(struct mchash_dll_node *)))) - offset);

    nw_type = nsc_repository_node_p->nw_type;
    nid     = nsc_repository_node_p->nid;

    retval  = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
    if(retval == OF_SUCCESS)
    {
      retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
      if(retval == OF_SUCCESS)
      {
        vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
        retval = mempool_release_mem_block(vn_nsc_info_p->nsc_repository_table_1_mempool_g,(uchar8_t*)nsc_repository_node_p,nsc_repository_node_p->heap_b);
        if(retval != MEMPOOL_SUCCESS)
        {
          printf("\r\n Failed to delete table_1 repository entry in rcu callback");  
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete table_1 repository entry in rcu callback");
        }             
        else
        {
          //printf("\r\n Successfully deleted table_1 repository entry in rcu callback");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Successfully  deleted table_1 repository entry in rcu callback");
        }    
      } 
    }
    else
    { 
      printf("\r\n Failed to obtain vn entry for L2 network to free table_1_repository_entry");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to obtain vn entry for L2 network to free table_1_repository_entry");
    }
  }
}

void nsc_free_table_2_repository_entry_rcu(struct rcu_head *nsc_repository_entry_p)
{
  struct   nschain_repository_entry* nsc_repository_node_p;
  uint8_t  nw_type;
  uint32_t nid,offset;
  uint64_t vn_handle;
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   crm_virtual_network* vn_entry_p;
  int32_t  retval;

  if(nsc_repository_entry_p)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"About to free table 2 repository entry");

    offset =  NSC_NSCHAIN_REPOSITORY_NODE_OFFSET;
    nsc_repository_node_p = (struct nschain_repository_entry  *) (((char *)nsc_repository_entry_p - (2*(sizeof(struct mchash_dll_node *)))) - offset);

    nw_type = nsc_repository_node_p->nw_type;
    nid     = nsc_repository_node_p->nid;

    retval  = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
    if(retval == OF_SUCCESS)
    {
      retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
      if(retval == OF_SUCCESS)
      {
        vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */
        retval = mempool_release_mem_block(vn_nsc_info_p->nsc_repository_table_2_mempool_g,(uchar8_t*)nsc_repository_node_p,nsc_repository_node_p->heap_b);
        if(retval != MEMPOOL_SUCCESS)
        {
          printf("\r\n Failed to delete table_2 repository entry in rcu callback");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete table_2 repository entry in rcu callback");
        }
        else
        {
          //printf("\r\n Successfully deleted table_2 repository entry in rcu callback");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Successfully deleted table_2 repository entry in rcu callback");
        }
      }
    }
    else
    {
      printf("\r\nFailed to obtain vn entry for L2 network to free table_2_repository_entry");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to obtain vn entry for L2 network to free table_2_repository_entry");
    }
  }
}

void nsc_free_table_3_repository_entry_rcu(struct rcu_head *ucastpkt_outport_repository_entry_p)
{
  struct   ucastpkt_outport_repository_entry* ucastpkt_outport_repository_node_p; 
  uint8_t  nw_type;
  uint32_t nid,offset;
  uint64_t vn_handle;
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   crm_virtual_network* vn_entry_p;
  int32_t  retval;

  if(ucastpkt_outport_repository_entry_p)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"About to free table 3 repository entry");

    offset = NSC_UCASTPKT_REPOSITORY_NODE_OFFSET;
    ucastpkt_outport_repository_node_p = (struct ucastpkt_outport_repository_entry *) (((char *)ucastpkt_outport_repository_entry_p - (2*(sizeof(struct mchash_dll_node *)))) - offset);

    nw_type = ucastpkt_outport_repository_node_p->nw_type;
    nid     = ucastpkt_outport_repository_node_p->nid;

    retval  = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
    if(retval == OF_SUCCESS)
    {
      retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
      if(retval == OF_SUCCESS)
      {
        vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  
        retval = mempool_release_mem_block(vn_nsc_info_p->nsc_repository_table_3_mempool_g,(uchar8_t*)ucastpkt_outport_repository_node_p,ucastpkt_outport_repository_node_p->heap_b);
        if(retval != MEMPOOL_SUCCESS)
        {
          printf("\r\n Failed to delete table_3 repository entry in rcu callback");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete table_3 repository entry in rcu callback");
        }
        else
        {
          //printf("\r\n Successfully deleted table_3 repository entry in rcu callback");
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Successfully  deleted table_3 repository entry in rcu callback");
        }
      }
    }
    else
    {
      printf("\r\nFailed to obtain vn entry for L2 network to free table_3_repository_entry");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to obtain vn entry for L2 network to free table_3_repository_entry");
    }
  }
}
/********************************************************************************************************************/
int32_t nsc_init_vn_resources(struct crm_virtual_network* vn_entry_p)  /* For all nw_types this API is called. */
{
  struct   vn_service_chaining_info* vn_nsc_info_p;
  struct   mempool_params mempool_info={};
  uint32_t cn_bind_entries_max,cn_bind_static_entries;
  uint32_t nsc_repository_entries_table_1_max,nsc_repository_static_entries_table_1;
  uint32_t nsc_repository_entries_table_2_max,nsc_repository_static_entries_table_2;
  uint32_t ucastpkt_outport_repository_entries_max,ucastpkt_outport_repository_static_entries;
  int32_t  retval;

  void *temp_p;

  CNTLR_RCU_READ_LOCK_TAKE();

  temp_p = (void *)(malloc(sizeof (struct  vn_service_chaining_info)));

  if(temp_p == NULL)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"memory allocation failure for vn_service_chaining_info");
    return OF_FAILURE;
  }
  
  *((tscaddr_t *)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g)) = (tscaddr_t)temp_p;
  vn_nsc_info_p = (struct vn_service_chaining_info *)*((tscaddr_t *)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));
  memset(vn_nsc_info_p,0,sizeof (struct vn_service_chaining_info));
 
  do
  {
    nsc_get_cn_bind_table_mempoolentries(&cn_bind_entries_max,&cn_bind_static_entries);
    nsc_no_of_cnbind_table_buckets_g = (cn_bind_entries_max * 2 / 5) + 1; /* Two selectors */
    
    mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
    mempool_info.block_size = sizeof(struct l2_connection_to_nsinfo_bind_node);
    mempool_info.max_blocks = cn_bind_entries_max;
    mempool_info.static_blocks = cn_bind_static_entries;
    mempool_info.threshold_min = cn_bind_static_entries/10;
    mempool_info.threshold_max = cn_bind_static_entries/3;
    mempool_info.replenish_count = cn_bind_static_entries/10;
    mempool_info.b_memset_not_required = FALSE;
    mempool_info.b_list_per_thread_required = FALSE;
    mempool_info.noof_blocks_to_move_to_thread_list = 0;
    /****************************************************/
    retval = mempool_create_pool("nsc_cnbind_pool",
                                 &mempool_info,
                                 &(vn_nsc_info_p->nsc_cn_bind_mempool_g)
                                );

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"debug2");
    if(retval != MEMPOOL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }
    /* The hash table contains the two selectors but not the bind entry */
    retval = mchash_table_create(nsc_no_of_cnbind_table_buckets_g,
                                 cn_bind_entries_max * 2,  /* Two selectors */
                                 tsc_nsc_cnbind_sel_free,
                                 &(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p)
                                );

    if(retval != MCHASHTBL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"debug3");
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"l2_connection_to_nsinfo_bind_table_p = %p",vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"debug4");
    /************************************************/

    /** creating mempool and hash table for nsc repository table 1 **/

    nsc_get_repository_table_1_mempoolentries(&nsc_repository_entries_table_1_max,&nsc_repository_static_entries_table_1);
    nsc_no_of_repository_table_1_buckets_g = (nsc_repository_entries_table_1_max / 5) + 1;

    mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
    mempool_info.block_size = sizeof(struct nschain_repository_entry);
    mempool_info.max_blocks = nsc_repository_entries_table_1_max;
    mempool_info.static_blocks = nsc_repository_static_entries_table_1;
    mempool_info.threshold_min = nsc_repository_static_entries_table_1/10;
    mempool_info.threshold_max = nsc_repository_static_entries_table_1/3;
    mempool_info.replenish_count = nsc_repository_static_entries_table_1/10;
    mempool_info.b_memset_not_required = FALSE;
    mempool_info.b_list_per_thread_required = FALSE;
    mempool_info.noof_blocks_to_move_to_thread_list = 0;
    /****************************************************/

    retval = mempool_create_pool("nsc_repository_table_1_pool",
                                 &mempool_info,
                                 &(vn_nsc_info_p->nsc_repository_table_1_mempool_g)
                                );

    if(retval != MEMPOOL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

    retval = mchash_table_create(nsc_no_of_repository_table_1_buckets_g,
                                 nsc_repository_entries_table_1_max,  
                                 nsc_free_table_1_repository_entry_rcu,
                                 &(vn_nsc_info_p->nsc_repository_table_1_p)
                                );

    if(retval != MCHASHTBL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

    /** creating mempool and hash table for nsc repository tables 2 **/

    nsc_get_repository_table_2_mempoolentries(&nsc_repository_entries_table_2_max,&nsc_repository_static_entries_table_2);
    nsc_no_of_repository_table_2_buckets_g = (nsc_repository_entries_table_2_max / 5) + 1;

    mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
    mempool_info.block_size = sizeof(struct nschain_repository_entry);
    mempool_info.max_blocks = nsc_repository_entries_table_2_max;
    mempool_info.static_blocks = nsc_repository_static_entries_table_2;
    mempool_info.threshold_min = nsc_repository_static_entries_table_2/10;
    mempool_info.threshold_max = nsc_repository_static_entries_table_2/3;
    mempool_info.replenish_count = nsc_repository_static_entries_table_2/10;
    mempool_info.b_memset_not_required = FALSE;
    mempool_info.b_list_per_thread_required = FALSE;
    mempool_info.noof_blocks_to_move_to_thread_list = 0;



   retval = mempool_create_pool("nsc_repository_table_2pool",
                                &mempool_info,
                                &(vn_nsc_info_p->nsc_repository_table_2_mempool_g)
                               );

    if(retval != MEMPOOL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

    retval = mchash_table_create(nsc_no_of_repository_table_2_buckets_g,
                                 nsc_repository_entries_table_2_max,
                                 nsc_free_table_2_repository_entry_rcu,
                                 &(vn_nsc_info_p->nsc_repository_table_2_p)
                                );

    if(retval != MCHASHTBL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

    /** creating mempool and hash table for nsc repository tables 3 **/

    nsc_get_repository_table_3_mempoolentries(&ucastpkt_outport_repository_entries_max,&ucastpkt_outport_repository_static_entries);
    nsc_no_of_repository_table_3_buckets_g = (ucastpkt_outport_repository_entries_max / 5) + 1;

    mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
    mempool_info.block_size = sizeof(struct ucastpkt_outport_repository_entry);
    mempool_info.max_blocks = ucastpkt_outport_repository_entries_max;          
    mempool_info.static_blocks = ucastpkt_outport_repository_static_entries;
    mempool_info.threshold_min = ucastpkt_outport_repository_static_entries/10;
    mempool_info.threshold_max = ucastpkt_outport_repository_static_entries/3;
    mempool_info.replenish_count = ucastpkt_outport_repository_static_entries/10;
    mempool_info.b_memset_not_required = FALSE;
    mempool_info.b_list_per_thread_required = FALSE;
    mempool_info.noof_blocks_to_move_to_thread_list = 0;

    retval = mempool_create_pool("nsc_repository_table_3pool",
                                &mempool_info,
                                &(vn_nsc_info_p->nsc_repository_table_3_mempool_g)
                               );

    if(retval != MEMPOOL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

    retval = mchash_table_create(nsc_no_of_repository_table_3_buckets_g,
                                 ucastpkt_outport_repository_entries_max,
                                 nsc_free_table_3_repository_entry_rcu,
                                 &(vn_nsc_info_p->nsc_repository_table_3_p)
                                );

    if(retval != MCHASHTBL_SUCCESS)
    {
      retval = OF_FAILURE;
      break;
    }

  }while(0);

  if(retval != OF_SUCCESS)
  {
    mempool_delete_pool(vn_nsc_info_p->nsc_cn_bind_mempool_g);
    mchash_table_delete(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p);

    mempool_delete_pool(vn_nsc_info_p->nsc_repository_table_1_mempool_g);
    mchash_table_delete(vn_nsc_info_p->nsc_repository_table_1_p);

    mempool_delete_pool(vn_nsc_info_p->nsc_repository_table_2_mempool_g);
    mchash_table_delete(vn_nsc_info_p->nsc_repository_table_2_p); 

    mempool_delete_pool(vn_nsc_info_p->nsc_repository_table_3_mempool_g);
    mchash_table_delete(vn_nsc_info_p->nsc_repository_table_3_p);
    
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC - initialization of VN resources is unsuccessful");
    return OF_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_RELEASE();
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC - initialization of VN resources is successful");
  return OF_SUCCESS;
}
/*****************************************************************************************************************/
int32_t nsc_deinit_vn_resources(struct crm_virtual_network* vn_entry_p)
{
  struct  vn_service_chaining_info* vn_nsc_info_p;
  int32_t retval = FALSE;

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

  retval = mempool_delete_pool(vn_nsc_info_p->nsc_cn_bind_mempool_g);
  retval = mchash_table_delete(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p);

  retval = mempool_delete_pool(vn_nsc_info_p->nsc_repository_table_1_mempool_g);
  retval = mempool_delete_pool(vn_nsc_info_p->nsc_repository_table_2_mempool_g);
  retval = mempool_delete_pool(vn_nsc_info_p->nsc_repository_table_3_mempool_g);

  retval = mchash_table_delete(vn_nsc_info_p->nsc_repository_table_1_p);
  retval = mchash_table_delete(vn_nsc_info_p->nsc_repository_table_2_p);
  retval = mchash_table_delete(vn_nsc_info_p->nsc_repository_table_3_p);

  return retval;
}
/***************************************************************************************************************/
int32_t nsc_init()
{
  int32_t retval = OF_SUCCESS;

  do
  {
    retval = crm_vn_register_application("nsrmapp1",&vn_nsc_info_offset_g);
    if(retval == CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC Application registration with VN infrastructure is successful");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC Application registration with VN infrastructure is unsuccessful");
      break;
    }

    retval =  nsc_register_nsrm_notifications();
    if(retval == OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC Application registration with NSRM notifications infrastructure is successful");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC Application registration with NSRM notification infrastructure is unsuccessful");
      retval = crm_vn_unregister_application("nsrmapp1");
      break;
    }
  }while(0);
  
  return retval;
}
/**************************************************************************************************************/
int32_t nsc_uninit()
{
  int32_t  retval;

  retval = crm_vn_unregister_application("nsrmapp1");
  if(retval == CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC Application deregistration with VN infrastructure is successful");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSC Application deregistration with VN infrastructure is unsuccessful");
  }
  return retval;
}
/*************************************************************************************************************/
