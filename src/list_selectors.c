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

/*File: list_selectors.c
 * Author: vinod sarma
 * Description:
 * This file implements listing of selectors from cli.
*/

/* INCLUDE_START ************************************************************/
#include "controller.h"
#include "dprm.h"
#include "nsrm.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "nsrmldef.h"
#include "cntlr_epoll.h"
#include "cntrl_queue.h"
#include "of_utilities.h"
#include "of_msgapi.h"
#include "of_multipart.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "dprmldef.h"
#include "of_view.h"
#include "tsc_cli.h"
#include "tsc_nsc_core.h"

void *tsc_nwservices_mempool_g;

void cnbind_entry_info_free_entry_rcu(struct rcu_head *list_entry_p)
{
   struct   tsc_nwservices *app_info_p=NULL;

   int32_t ret_val;
   if(list_entry_p)
   {
     app_info_p = (struct tsc_nwservices *)(((char *)list_entry_p - sizeof(of_list_node_t)));

     ret_val = mempool_release_mem_block(tsc_nwservices_mempool_g,
                                        (uchar8_t*)app_info_p,app_info_p->heap_b);
  if(ret_val != MEMPOOL_SUCCESS)
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to release memory block.");
  }
  else
  {
     OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Released  memory block from the memory pool.");
  }
 }
  else
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Null entry to release",__FUNCTION__,__LINE__);
  }

  return TSC_SUCCESS;
}

struct mchash_table *l2_connection_to_nsinfo_bind_table_p;
struct mempool_params mempool_info={};

struct nsc_selector_node_key *selector_key_p = NULL;
int32_t flag = 0;

int32_t tsc_add_selector_key(struct nsc_selector_node_key *selector_key)
{
   selector_key_p = selector_key;
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Adding the KEY temporarily to scan the database");
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nwname : %s",selector_key_p->vn_name);

   return TSC_SUCCESS;
}

int32_t tsc_get_first_matching_cnbind_entry(struct nsc_selector_node_key   *selector_key,
                                            uint64_t                       *matched_node_saferef,
                                            struct tsc_cnbind_entry_info   *cnbind_entry_info_p)
{
  struct   vn_service_chaining_info  *vn_nsc_info_p = NULL;
  struct   crm_virtual_network       *vn_entry_p = NULL;
  struct   nsc_selector_node         *selector_node_scan_p = NULL;
  struct   tsc_nwservices            *tsc_nwservices_p = NULL;
  struct   nsrm_nschainset_object    *nschainset_object_p  = NULL;
  struct   nsrm_nschain_object       *nschain_object_p  = NULL;
  struct   nsrm_nwservice_object     *nwservice_object_p = NULL;
  struct   nwservice_instance        *nwservice_instance_p = NULL;
 
  uint64_t hashkey,node_saferef,attach_handle,vn_handle;
  uint32_t offset;
  int32_t  ii,retval;
  uint8_t  node_found_b = FALSE ,heap_b;

  selector_key = selector_key_p;
  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn name sent : %s",selector_key->vn_name); 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn name sent : %s",selector_key_p->vn_name); 
  retval = crm_get_vn_handle(selector_key->vn_name,&vn_handle);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn doesnt exist");
    return TSC_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle , &vn_entry_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn doesnt exist");
    return TSC_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn name found : %s",vn_entry_p->nw_name);

   vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  
   
   l2_connection_to_nsinfo_bind_table_p = vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p; 

   mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
   mempool_info.block_size = sizeof(struct tsc_nwservices);
   mempool_info.max_blocks = 64;
   mempool_info.static_blocks = 16;
   mempool_info.threshold_min = 16/10;
   mempool_info.threshold_max = 16/3;
   mempool_info.replenish_count = 16/10;
   mempool_info.b_memset_not_required =  FALSE;
   mempool_info.b_list_per_thread_required =  FALSE;
   mempool_info.noof_blocks_to_move_to_thread_list = 0;

   retval = mempool_create_pool("tsc_nwservices_mempool", &mempool_info,
                   &tsc_nwservices_mempool_g );
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"after mempool_create_pool");
   if(retval != MEMPOOL_SUCCESS)
   {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Memory Pool Creation failed  %d", retval);
      return MEMPOOL_FAILURE;
   }

  offset = NSC_SELECTOR_NODE_OFFSET;
  if(l2_connection_to_nsinfo_bind_table_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Table is null");
    mempool_delete_pool(tsc_nwservices_mempool_g);
    return TSC_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  MCHASH_TABLE_SCAN(l2_connection_to_nsinfo_bind_table_p,hashkey)
  {
    MCHASH_BUCKET_SCAN(l2_connection_to_nsinfo_bind_table_p,hashkey,selector_node_scan_p,
                                              struct nsc_selector_node*,offset)
    {
        
       if(selector_key->src_ip !=ANY && selector_key->src_ip != selector_node_scan_p->src_ip)
         continue;
       if(selector_key->dst_ip !=ANY && selector_key->dst_ip != selector_node_scan_p->dst_ip)
         continue; 
       if(selector_key->dst_port !=ANY && selector_key->dst_port != selector_node_scan_p->dst_port)
         continue;
       if(selector_key->protocol !=ANY && selector_key->protocol != selector_node_scan_p->protocol)
         continue;
       
       node_found_b = TRUE;
       break;
    } 
    if(node_found_b == TRUE)
      break;
  }
  if(node_found_b == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"No mataching selector found");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return TSC_FAILURE;
  }
 
  cnbind_entry_info_p->src_ip = selector_node_scan_p->src_ip;
  cnbind_entry_info_p->dst_ip = selector_node_scan_p->dst_ip;
  cnbind_entry_info_p->dst_port = selector_node_scan_p->dst_port;
  cnbind_entry_info_p->protocol = selector_node_scan_p->protocol;
 
  retval = nsrm_get_nschainset_byhandle(vn_nsc_info_p->srf_nschainset_object,&nschainset_object_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"NSCHAINSET invalid");
    mempool_delete_pool(tsc_nwservices_mempool_g);
    CNTLR_RCU_READ_LOCK_RELEASE();
    return TSC_FAILURE;
  }

  
  strcpy(cnbind_entry_info_p->nschainset_name_p , nschainset_object_p->name_p);
  node_saferef = selector_node_scan_p->safe_reference;
  *matched_node_saferef = node_saferef; 
 
  if(selector_node_scan_p->cnbind_node_p->nwservice_info.no_of_nwservice_instances == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No network service instances exist");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return TSC_SUCCESS;
  } 
  retval = mempool_get_mem_block(tsc_nwservices_mempool_g,(uchar8_t **)&tsc_nwservices_p, &heap_b);

  OF_LIST_INIT(cnbind_entry_info_p->nwservices_info,cnbind_entry_info_free_entry_rcu);
  for(ii = 0 ; ii < selector_node_scan_p->cnbind_node_p->nwservice_info.no_of_nwservice_instances;ii++)
  {
     retval = nsrm_get_nschain_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle , &nschain_object_p);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nschain object invalid");
       free(tsc_nwservices_p->nwservice_object_name_p);
       free(tsc_nwservices_p->nschain_name_p);
       free(tsc_nwservices_p->nwservice_instance_name_p);
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE();
       return TSC_FAILURE;
     }
     retval = nsrm_get_nwservice_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nwservice_object_handle , &nwservice_object_p);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice object invalid");
       free(tsc_nwservices_p->nwservice_object_name_p);
       free(tsc_nwservices_p->nschain_name_p);
       free(tsc_nwservices_p->nwservice_instance_name_p);
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE();
       return TSC_FAILURE;
     }
     retval = nsrm_get_attached_nwservice_handle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle ,nwservice_object_p->name_p, &attach_handle);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice object not attached to this chain");
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE();
       return TSC_FAILURE;
     }

     retval = nsrm_get_nwservice_instance_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle,
                                                   attach_handle,
                                                   selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nwservice_instance_handle,
                                                   &nwservice_instance_p);

     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice instance not valid");
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE();
       return TSC_FAILURE;
     }
     tsc_nwservices_p = (struct tsc_nwservices *) calloc(1,sizeof(struct tsc_nwservices));
     tsc_nwservices_p->nwservice_object_name_p    = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     tsc_nwservices_p->nwservice_instance_name_p  = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     tsc_nwservices_p->nschain_name_p             = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     strcpy(tsc_nwservices_p->nwservice_object_name_p , nwservice_object_p->name_p);
     strcpy(tsc_nwservices_p->nschain_name_p , nschain_object_p->name_p);
     strcpy(tsc_nwservices_p->nwservice_instance_name_p , nwservice_instance_p->name_p);
 
     OF_APPEND_NODE_TO_LIST(cnbind_entry_info_p->nwservices_info ,tsc_nwservices_p,
                            CNBIND_NWSERVICE_LIST_NODE_OFFSET);
  } 

  CNTLR_RCU_READ_LOCK_RELEASE();
  return TSC_SUCCESS;
} 


int32_t tsc_get_next_matching_cnbind_entry(struct nsc_selector_node_key   *selector_key,
                                           uint64_t                       relative_node_saferef,
                                           uint64_t                       *matched_node_saferef,
                                           struct tsc_cnbind_entry_info   *cnbind_entry_info_p)
{
  struct   nsc_selector_node         *selector_node_scan_p = NULL;
  struct   tsc_nwservices            *tsc_nwservices_p = NULL;
  struct   crm_virtual_network       *vn_entry_p = NULL;
  struct   vn_service_chaining_info  *vn_nsc_info_p = NULL;
  struct   nsrm_nschainset_object    *nschainset_object_p  = NULL;
  struct   nsrm_nschain_object       *nschain_object_p  = NULL;
  struct   nsrm_nwservice_object     *nwservice_object_p = NULL;
  struct   nwservice_instance        *nwservice_instance_p = NULL;

  uint64_t hashkey,node_saferef,attach_handle,vn_handle;
  uint32_t offset;
  int32_t  ii,retval;
  uint8_t  node_found_b = FALSE , relative_node_found_b = FALSE;
 
  selector_key = selector_key_p;

  retval = crm_get_vn_handle(selector_key->vn_name,&vn_handle);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn doesnt exist");
    return TSC_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle , &vn_entry_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn doesnt exist");
    return TSC_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn name found : %s",vn_entry_p->nw_name);

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

  l2_connection_to_nsinfo_bind_table_p = vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p;

  offset = NSC_SELECTOR_NODE_OFFSET;
  CNTLR_RCU_READ_LOCK_TAKE(); 
  MCHASH_TABLE_SCAN(l2_connection_to_nsinfo_bind_table_p,hashkey)
  {
    MCHASH_BUCKET_SCAN(l2_connection_to_nsinfo_bind_table_p,hashkey,selector_node_scan_p,
                                              struct nsc_selector_node*,offset)
    {  
       if(relative_node_found_b == FALSE)
       {
         if(relative_node_saferef == selector_node_scan_p->safe_reference)
         {
            relative_node_found_b = TRUE;
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Relative node found");
            continue;
         }
       } 
       if(relative_node_found_b == FALSE)
       {
          node_found_b = FALSE;
          continue;
       }
       if(selector_key->src_ip !=ANY && selector_key->src_ip != selector_node_scan_p->src_ip)
         continue;
       if(selector_key->dst_ip !=ANY && selector_key->dst_ip != selector_node_scan_p->dst_ip)
         continue; 
       if(selector_key->dst_port !=ANY && selector_key->dst_port != selector_node_scan_p->dst_port)
         continue;
       if(selector_key->protocol !=ANY && selector_key->protocol != selector_node_scan_p->protocol)
         continue;
       node_found_b = TRUE;
       break;
    } 
    if(node_found_b == TRUE)
      break;
  }
  if(node_found_b == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"No mataching selector found");
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    return TSC_FAILURE;
  }

  cnbind_entry_info_p->src_ip = selector_node_scan_p->src_ip;
  cnbind_entry_info_p->dst_ip = selector_node_scan_p->dst_ip;
  cnbind_entry_info_p->dst_port = selector_node_scan_p->dst_port;
  cnbind_entry_info_p->protocol = selector_node_scan_p->protocol;

  retval = nsrm_get_nschainset_byhandle(vn_nsc_info_p->srf_nschainset_object,&nschainset_object_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"NSCHAINSET invalid");
    mempool_delete_pool(tsc_nwservices_mempool_g);
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    return TSC_FAILURE;
  }
  
  strcpy(cnbind_entry_info_p->nschainset_name_p , nschainset_object_p->name_p);
  node_saferef = selector_node_scan_p->safe_reference;
  *matched_node_saferef = node_saferef; 
 
  if(selector_node_scan_p->cnbind_node_p->nwservice_info.no_of_nwservice_instances == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No network service instances exist");
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    return TSC_FAILURE;
  } 
  OF_LIST_INIT(cnbind_entry_info_p->nwservices_info,cnbind_entry_info_free_entry_rcu);
  for(ii = 0 ; ii < selector_node_scan_p->cnbind_node_p->nwservice_info.no_of_nwservice_instances;ii++)
  {
     retval = nsrm_get_nschain_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle , &nschain_object_p);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nschain object invalid");
       free(tsc_nwservices_p->nwservice_object_name_p);
       free(tsc_nwservices_p->nschain_name_p);
       free(tsc_nwservices_p->nwservice_instance_name_p);
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE(); 
       return TSC_FAILURE;
     }

     retval = nsrm_get_nwservice_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nwservice_object_handle , &nwservice_object_p);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice object invalid");
       free(tsc_nwservices_p->nwservice_object_name_p);
       free(tsc_nwservices_p->nschain_name_p);
       free(tsc_nwservices_p->nwservice_instance_name_p);
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE(); 
       return TSC_FAILURE;
     }
     retval = nsrm_get_attached_nwservice_handle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle ,nwservice_object_p->name_p, &attach_handle);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice object not attached to this chain");
       free(tsc_nwservices_p->nwservice_object_name_p);
       free(tsc_nwservices_p->nschain_name_p);
       free(tsc_nwservices_p->nwservice_instance_name_p);
       mempool_delete_pool(tsc_nwservices_mempool_g);
       CNTLR_RCU_READ_LOCK_RELEASE(); 
       return TSC_FAILURE;
     }

     retval = nsrm_get_nwservice_instance_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle,
                                                   attach_handle,
                                                   selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nwservice_instance_handle,
                                                   &nwservice_instance_p);

     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice instance not valid");
       CNTLR_RCU_READ_LOCK_RELEASE(); 
       return TSC_FAILURE;
     }
     tsc_nwservices_p = (struct tsc_nwservices *) calloc(1,sizeof(struct tsc_nwservices));
     tsc_nwservices_p->nwservice_object_name_p    = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     tsc_nwservices_p->nwservice_instance_name_p  = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     tsc_nwservices_p->nschain_name_p             = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     strcpy(tsc_nwservices_p->nwservice_object_name_p , nwservice_object_p->name_p);
     strcpy(tsc_nwservices_p->nwservice_instance_name_p , nwservice_instance_p->name_p);
     strcpy(tsc_nwservices_p->nschain_name_p , nschain_object_p->name_p);
 
     OF_APPEND_NODE_TO_LIST(cnbind_entry_info_p->nwservices_info ,tsc_nwservices_p,
                            CNBIND_NWSERVICE_LIST_NODE_OFFSET);
  } 

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Get next record successful ! ");
  CNTLR_RCU_READ_LOCK_RELEASE(); 
  return TSC_SUCCESS;
}

int32_t tsc_get_exact_matching_cnbind_entry(struct nsc_selector_node_key   *selector_key,
                                            struct tsc_cnbind_entry_info   *cnbind_entry_info_p)
{
  struct   vn_service_chaining_info  *vn_nsc_info_p = NULL;
  struct   crm_virtual_network       *vn_entry_p = NULL;
  struct   nsc_selector_node         *selector_node_scan_p = NULL;
  struct   tsc_nwservices            *tsc_nwservices_p = NULL;
  struct   nsrm_nschainset_object    *nschainset_object_p  = NULL;
  struct   nsrm_nschain_object       *nschain_object_p  = NULL;
  struct   nsrm_nwservice_object     *nwservice_object_p = NULL;
  struct   nwservice_instance        *nwservice_instance_p = NULL; 

  uint64_t hashkey,attach_handle,vn_handle;
  uint32_t offset;
  int32_t  ii,retval;
  uint8_t  node_found_b = FALSE;

  offset = NSC_SELECTOR_NODE_OFFSET;

  if(flag == 0)
  {
      flag++;
      return TSC_FAILURE;
  }
  else
      return TSC_SUCCESS;

  if(selector_key_p == NULL)
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"selector key null");
     return TSC_FAILURE;
  } 

  if(strcmp(selector_key->vn_name,selector_key_p->vn_name) == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Key found ");
    return TSC_SUCCESS;
  } 

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn name sent : %s",selector_key->vn_name);
  retval = crm_get_vn_handle(selector_key->vn_name,&vn_handle);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn doesnt exist");
    return TSC_FAILURE;
  }

  retval = crm_get_vn_byhandle(vn_handle , &vn_entry_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn doesnt exist");
    return TSC_FAILURE;
  }
  
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn name found : %s",vn_entry_p->nw_name);

   vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

   l2_connection_to_nsinfo_bind_table_p = vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p;

   
  MCHASH_TABLE_SCAN(l2_connection_to_nsinfo_bind_table_p,hashkey)
  {
    MCHASH_BUCKET_SCAN(l2_connection_to_nsinfo_bind_table_p,hashkey,selector_node_scan_p,
                                              struct nsc_selector_node*,offset)
    {
       if((selector_key->src_ip != selector_node_scan_p->src_ip)
                              &&
         (selector_key->src_ip != selector_node_scan_p->dst_ip)
                              &&
         (selector_key->src_ip != selector_node_scan_p->dst_port)
                              &&
         (selector_key->src_ip != selector_node_scan_p->protocol))
      {
        continue;
      }
       node_found_b = TRUE;
       break;
    } 
    if(node_found_b == TRUE)
      break;
  }
  if(node_found_b == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"No mataching selector found");
    return TSC_FAILURE;
  }
  retval = nsrm_get_nschainset_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nschainset_object_handle , &nschainset_object_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"NSCHAINSET invalid");
    return TSC_FAILURE;
  }

  retval = nsrm_get_nschain_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nschain_object_handle , &nschain_object_p);
  if(retval != NSRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"NSCHAIN invalid");
    return TSC_FAILURE;
  }
  
  strcpy(cnbind_entry_info_p->nschainset_name_p , nschainset_object_p->name_p);
  
  if(selector_node_scan_p->cnbind_node_p->nwservice_info.no_of_nwservice_instances == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No network service instances exist");
    return TSC_SUCCESS;
  } 
  OF_LIST_INIT(cnbind_entry_info_p->nwservices_info,cnbind_entry_info_free_entry_rcu);
  for(ii = 0 ; ii < selector_node_scan_p->cnbind_node_p->nwservice_info.no_of_nwservice_instances;ii++)
  {
     retval = nsrm_get_nwservice_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nwservice_object_handle , &nwservice_object_p);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice object invalid");
       return TSC_FAILURE;
     }
     retval = nsrm_get_attached_nwservice_handle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle ,nwservice_object_p->name_p, &attach_handle);
     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice object not attached to this chain");
       return TSC_FAILURE;
     }

     retval = nsrm_get_nwservice_instance_byhandle(selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nschain_object_handle,
                                                   attach_handle,
                                                   selector_node_scan_p->cnbind_node_p->nwservice_info.nw_services_p[ii].nwservice_instance_handle,
                                                   &nwservice_instance_p);

     if(retval != NSRM_SUCCESS)
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"nwservice instance not valid");
       return TSC_FAILURE;
     }
     tsc_nwservices_p = (struct tsc_nwservices *) calloc(1,sizeof(struct tsc_nwservices));
     tsc_nwservices_p->nwservice_object_name_p    = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     tsc_nwservices_p->nwservice_instance_name_p = (char *) calloc(1,NSRM_MAX_NAME_LENGTH);
     strcpy(tsc_nwservices_p->nwservice_object_name_p , nwservice_object_p->name_p);
     strcpy(tsc_nwservices_p->nwservice_instance_name_p , nwservice_instance_p->name_p);
 
     OF_APPEND_NODE_TO_LIST(cnbind_entry_info_p->nwservices_info ,tsc_nwservices_p,
                            CNBIND_NWSERVICE_LIST_NODE_OFFSET);
   }
 
  return TSC_SUCCESS;
} 


