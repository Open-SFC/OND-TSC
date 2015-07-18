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
 * This file implements listing of repository entries from cli.
*/

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
#include "tsc_controller.h"

struct mchash_table *nsc_repository_table_g = NULL;
struct nsc_repository_key *global_repository_key_p = NULL;
struct mchash_table *nsc_table_3_repository_table_g = NULL;
struct nsc_repository_key *global_table_3_repository_key_p = NULL;

int32_t flag_1_2 = 0;
int32_t flag_3 = 0;

int32_t nsc_add_repository_entry(struct nsc_repository_key *key)
{
   global_repository_key_p = key;
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Adding the KEY temporarily to scan the database");
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nwname : %s",global_repository_key_p->vn_name);

   return TSC_SUCCESS;
}

int32_t nsc_get_first_repository_entry(struct nsc_repository_key *key ,uint64_t *match_saferef, struct  nsc_repository_entry  *nsc_repository_entry_p)
{
  struct   vn_service_chaining_info  *vn_nsc_info_p          = NULL;
  struct   crm_virtual_network       *vn_entry_p             = NULL;
  struct   nschain_repository_entry  *nsc_repository_scan_node_p = NULL;
  
  uint64_t hashkey,vn_handle;
  uint32_t offset;
  int32_t  retval;

  key = global_repository_key_p;

  retval = crm_get_vn_handle(key->vn_name,&vn_handle);
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

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"table id in key :%d",key->table_id); 
  if(key->table_id == 1) 
  {
    nsc_repository_table_g = vn_nsc_info_p->nsc_repository_table_1_p;
  }
  else if(key->table_id == 2)
  {
    nsc_repository_table_g = vn_nsc_info_p->nsc_repository_table_2_p;
  }
  else
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid table ID");
     return TSC_FAILURE;
  }
  if(nsc_repository_table_g == NULL)
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No entries in table repository");
     return TSC_FAILURE;
  }
  
  offset = 0;
  CNTLR_RCU_READ_LOCK_TAKE();
  MCHASH_TABLE_SCAN(nsc_repository_table_g,hashkey)
  {
     MCHASH_BUCKET_SCAN(nsc_repository_table_g,hashkey,nsc_repository_scan_node_p,
                                                  struct nschain_repository_entry*,offset)  
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_switch_name : %s",nsc_repository_scan_node_p->local_switch_name_p);
       strcpy(nsc_repository_entry_p->local_switch_name_p , nsc_repository_scan_node_p->local_switch_name_p);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_switch_name : %s",nsc_repository_entry_p->local_switch_name_p);
       nsc_repository_entry_p->dp_handle	=        nsc_repository_scan_node_p->dp_handle; 
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dphandle : %llx",nsc_repository_scan_node_p->dp_handle);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dphandle : %llx",nsc_repository_entry_p->dp_handle);
       nsc_repository_entry_p->nid              =        nsc_repository_scan_node_p->nid;
       nsc_repository_entry_p->serviceport      =	 nsc_repository_scan_node_p->serviceport;
       nsc_repository_entry_p->remote_switch_ip =	 nsc_repository_scan_node_p->remote_switch_ip;
       nsc_repository_entry_p->in_port_id       =	 nsc_repository_scan_node_p->in_port_id;
       nsc_repository_entry_p->ns_chain_b	=	 nsc_repository_scan_node_p->ns_chain_b;
       nsc_repository_entry_p->more_nf_b	=	 nsc_repository_scan_node_p->more_nf_b;
       nsc_repository_entry_p->vlan_id_pkt	=	 nsc_repository_scan_node_p->vlan_id_pkt;
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlanpkt : %d",nsc_repository_scan_node_p->vlan_id_pkt);
       nsc_repository_entry_p->match_vlan_id	=	 nsc_repository_scan_node_p->match_vlan_id;

       nsc_repository_entry_p->match_vlan_id    =        (nsc_repository_entry_p->match_vlan_id & 0x0fff);

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan match : %d",nsc_repository_entry_p->match_vlan_id);
       nsc_repository_entry_p->next_vlan_id	=	 nsc_repository_scan_node_p->next_vlan_id;
       nsc_repository_entry_p->next_vlan_id     =        (nsc_repository_entry_p->next_vlan_id & 0x0fff);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next vlan : %d",nsc_repository_entry_p->next_vlan_id);
       nsc_repository_entry_p->nw_port_b	=	 nsc_repository_scan_node_p->nw_port_b;
       nsc_repository_entry_p->out_port_no	=	 nsc_repository_scan_node_p->out_port_no;
       nsc_repository_entry_p->next_table_id    =	 nsc_repository_scan_node_p->next_table_id;
       memcpy(nsc_repository_entry_p->local_in_mac, nsc_repository_scan_node_p->local_in_mac , 6);
       memcpy(nsc_repository_entry_p->local_out_mac, nsc_repository_scan_node_p->local_out_mac ,6);
       nsc_repository_entry_p->src_mac_high     =	 nsc_repository_scan_node_p->selector.src_mac_high;
       nsc_repository_entry_p->src_mac_low      =	nsc_repository_scan_node_p->selector.src_mac_low;
       nsc_repository_entry_p->dst_mac_high     =	nsc_repository_scan_node_p->selector.dst_mac_high;
       nsc_repository_entry_p->dst_mac_low      =	nsc_repository_scan_node_p->selector.dst_mac_low;
       memcpy(nsc_repository_entry_p->src_mac,nsc_repository_scan_node_p->selector.src_mac ,6);
       memcpy(nsc_repository_entry_p->dst_mac,nsc_repository_scan_node_p->selector.dst_mac ,6);
       nsc_repository_entry_p->ethernet_type	=	nsc_repository_scan_node_p->selector.ethernet_type;
       nsc_repository_entry_p->protocol         =	nsc_repository_scan_node_p->selector.protocol;
       nsc_repository_entry_p->src_port         =	nsc_repository_scan_node_p->selector.src_port;
       nsc_repository_entry_p->dst_port         =	nsc_repository_scan_node_p->selector.dst_port;
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dstport : %d",nsc_repository_scan_node_p->selector.dst_port);
       nsc_repository_entry_p->src_ip	        =	nsc_repository_scan_node_p->selector.src_ip;
       nsc_repository_entry_p->dst_ip           =	nsc_repository_scan_node_p->selector.dst_ip;
  
       nsc_repository_entry_p->safe_reference   =       nsc_repository_scan_node_p->safe_reference;
   
       *match_saferef = nsc_repository_scan_node_p->safe_reference;
       
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_table_id  = %d",nsc_repository_scan_node_p->next_table_id);    
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_switch_name : %s",nsc_repository_entry_p->local_switch_name_p);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successful");
       CNTLR_RCU_READ_LOCK_RELEASE();
       return TSC_SUCCESS;
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return TSC_FAILURE;
}

int32_t nsc_get_next_repository_entry(struct nsc_repository_key *key ,uint64_t relative_saferef,uint64_t *next_saferef,  struct  nsc_repository_entry  *nsc_repository_entry_p)
{
  struct   vn_service_chaining_info  *vn_nsc_info_p          = NULL;
  struct   crm_virtual_network       *vn_entry_p             = NULL;
  struct   nschain_repository_entry  *nsc_repository_scan_node_p = NULL;
  
  uint64_t hashkey,vn_handle;
  uint8_t  node_found_b = FALSE , relative_node_found = FALSE;
  uint32_t offset;
  int32_t  retval;

  key = global_repository_key_p;

  retval = crm_get_vn_handle(key->vn_name,&vn_handle);
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

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));
  
  if(key->table_id == 1)
  {
    nsc_repository_table_g = vn_nsc_info_p->nsc_repository_table_1_p;
  }
  else if(key->table_id == 2)
  {
    nsc_repository_table_g = vn_nsc_info_p->nsc_repository_table_2_p;
  }
  else
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Invalid table ID");
     return TSC_FAILURE;
  }
 
  if(nsc_repository_table_g == NULL)
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No entries in table 1 repository");
     return TSC_FAILURE;
  }
  
  offset = 0;
  CNTLR_RCU_READ_LOCK_TAKE();
  MCHASH_TABLE_SCAN(nsc_repository_table_g,hashkey)
  {
     MCHASH_BUCKET_SCAN(nsc_repository_table_g,hashkey,nsc_repository_scan_node_p,
                                                  struct nschain_repository_entry*,offset)  
     {
       if(node_found_b == FALSE)
       {
          if(nsc_repository_scan_node_p->safe_reference == relative_saferef)
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"scan saferef : %llx,relative_saferef : %llx",nsc_repository_scan_node_p->safe_reference,relative_saferef);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Relative node found");
            node_found_b = TRUE;
          }
          continue;          
       }
       relative_node_found = TRUE; 
       strcpy(nsc_repository_entry_p->local_switch_name_p , nsc_repository_scan_node_p->local_switch_name_p);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"switch name : %s",nsc_repository_entry_p->local_switch_name_p);
       nsc_repository_entry_p->dp_handle	=        nsc_repository_scan_node_p->dp_handle; 
       nsc_repository_entry_p->nid              =        nsc_repository_scan_node_p->nid;
       nsc_repository_entry_p->serviceport      =	 nsc_repository_scan_node_p->serviceport;
       nsc_repository_entry_p->remote_switch_ip =	 nsc_repository_scan_node_p->remote_switch_ip;
       nsc_repository_entry_p->in_port_id       =	 nsc_repository_scan_node_p->in_port_id;
       nsc_repository_entry_p->ns_chain_b	=	 nsc_repository_scan_node_p->ns_chain_b;
       nsc_repository_entry_p->more_nf_b	=	 nsc_repository_scan_node_p->more_nf_b;
       nsc_repository_entry_p->vlan_id_pkt	=	 nsc_repository_scan_node_p->vlan_id_pkt;
       nsc_repository_entry_p->match_vlan_id	=	 nsc_repository_scan_node_p->match_vlan_id;
       
       nsc_repository_entry_p->match_vlan_id    =        (nsc_repository_entry_p->match_vlan_id & 0x0fff);       
       nsc_repository_entry_p->next_vlan_id	=	 nsc_repository_scan_node_p->next_vlan_id;

       nsc_repository_entry_p->next_vlan_id     =        (nsc_repository_entry_p->next_vlan_id & 0x0fff);

       nsc_repository_entry_p->nw_port_b	=	 nsc_repository_scan_node_p->nw_port_b;
       nsc_repository_entry_p->out_port_no	=	 nsc_repository_scan_node_p->out_port_no;
       nsc_repository_entry_p->next_table_id    =	 nsc_repository_scan_node_p->next_table_id;
       memcpy(nsc_repository_entry_p->local_in_mac,nsc_repository_scan_node_p->local_in_mac , 6);
       memcpy(nsc_repository_entry_p->local_out_mac,nsc_repository_scan_node_p->local_out_mac ,6);
       nsc_repository_entry_p->src_mac_high     =	 nsc_repository_scan_node_p->selector.src_mac_high;
       nsc_repository_entry_p->src_mac_low      =	nsc_repository_scan_node_p->selector.src_mac_low;
       nsc_repository_entry_p->dst_mac_high     =	nsc_repository_scan_node_p->selector.dst_mac_high;
       nsc_repository_entry_p->dst_mac_low      =	nsc_repository_scan_node_p->selector.dst_mac_low;
       memcpy(nsc_repository_entry_p->src_mac,nsc_repository_scan_node_p->selector.src_mac ,6);
       memcpy(nsc_repository_entry_p->dst_mac,nsc_repository_scan_node_p->selector.dst_mac ,6);
       nsc_repository_entry_p->ethernet_type	=	nsc_repository_scan_node_p->selector.ethernet_type;
       nsc_repository_entry_p->protocol         =	nsc_repository_scan_node_p->selector.protocol;
       nsc_repository_entry_p->src_port         =	nsc_repository_scan_node_p->selector.src_port;
       nsc_repository_entry_p->dst_port         =	nsc_repository_scan_node_p->selector.dst_port;
       nsc_repository_entry_p->src_ip	        =	nsc_repository_scan_node_p->selector.src_ip;
       nsc_repository_entry_p->dst_ip           =	nsc_repository_scan_node_p->selector.dst_ip;
       
       nsc_repository_entry_p->safe_reference   =       nsc_repository_scan_node_p->safe_reference;
   
       *next_saferef = nsc_repository_scan_node_p->safe_reference;
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"next_table_id  = %d",nsc_repository_scan_node_p->next_table_id); 
       break;
    } 
    if(node_found_b == TRUE && relative_node_found == TRUE)
      break;
  }
  if(node_found_b == FALSE || relative_node_found == FALSE)
  { 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No repository found or reached last bucket");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return TSC_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Get next record successful ! ");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return TSC_SUCCESS;
}

int32_t nsc_get_exact_repository_entry(void)
{

  if(flag_1_2 == 0)
  {
      flag_1_2++;
      return TSC_FAILURE;
  }
  else
      return TSC_SUCCESS;
}

int32_t nsc_add_table_3_repository_entry(struct nsc_repository_key *key)
{
   global_table_3_repository_key_p = key;
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Adding the KEY temporarily to scan the database");
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nwname : %s",global_table_3_repository_key_p->vn_name);

   return TSC_SUCCESS;
}

int32_t nsc_get_first_table_3_repository_entry(struct   nsc_repository_key *key ,
                                               uint64_t *node_saferef,
                                               struct   ucastpkt_repository_entry  *nsc_repository_entry_p)
{
  struct   vn_service_chaining_info  *vn_nsc_info_p          = NULL;
  struct   crm_virtual_network       *vn_entry_p             = NULL;
  struct   ucastpkt_outport_repository_entry  *nsc_repository_scan_node_p = NULL;

  uint64_t hashkey,vn_handle;
  uint32_t offset;
  int32_t  retval;

  key = global_table_3_repository_key_p;

  retval = crm_get_vn_handle(key->vn_name,&vn_handle);
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

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

  nsc_table_3_repository_table_g = vn_nsc_info_p->nsc_repository_table_3_p;
  if(nsc_table_3_repository_table_g == NULL)
  {
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No entries in table 3 repository");
     return TSC_FAILURE;
  }

  offset = 0;
  CNTLR_RCU_READ_LOCK_TAKE();
  MCHASH_TABLE_SCAN(nsc_table_3_repository_table_g,hashkey)
  {
     MCHASH_BUCKET_SCAN(nsc_table_3_repository_table_g,hashkey,nsc_repository_scan_node_p,
                                                  struct ucastpkt_outport_repository_entry*,offset)
     {

       strcpy(nsc_repository_entry_p->local_switch_name_p , nsc_repository_scan_node_p->local_switch_name_p);

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local switch : %s",nsc_repository_entry_p->local_switch_name_p);
       strcpy(nsc_repository_entry_p->remote_switch_name_p , nsc_repository_scan_node_p->remote_switch_name_p);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"remote switch : %s",nsc_repository_entry_p->remote_switch_name_p);
       nsc_repository_entry_p->dp_handle        =        nsc_repository_scan_node_p->dp_handle;
       nsc_repository_entry_p->nid              =        nsc_repository_scan_node_p->nid;
       nsc_repository_entry_p->port_type        =        nsc_repository_scan_node_p->port_type;
       nsc_repository_entry_p->serviceport      =        nsc_repository_scan_node_p->serviceport;
       nsc_repository_entry_p->tun_dest_ip      =        nsc_repository_scan_node_p->tun_dest_ip;
       nsc_repository_entry_p->nw_port_b        =        nsc_repository_scan_node_p->nw_port_b;
       nsc_repository_entry_p->out_port_no      =        nsc_repository_scan_node_p->out_port_no;
       memcpy(nsc_repository_entry_p->dst_mac,nsc_repository_scan_node_p->dst_mac ,6);

       nsc_repository_entry_p->safe_reference   =       nsc_repository_scan_node_p->safe_reference;

       *node_saferef = nsc_repository_scan_node_p->safe_reference;
       CNTLR_RCU_READ_LOCK_RELEASE();
       return TSC_SUCCESS;
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return TSC_FAILURE;
}

int32_t nsc_get_next_table_3_repository_entry(struct nsc_repository_key *key ,uint64_t relative_saferef,
                                              uint64_t *match_saferef,
                                              struct  ucastpkt_repository_entry  *nsc_repository_entry_p)
{
  struct   vn_service_chaining_info  *vn_nsc_info_p          = NULL;
  struct   crm_virtual_network       *vn_entry_p             = NULL;
  struct   ucastpkt_outport_repository_entry  *nsc_repository_scan_node_p = NULL;

  uint64_t hashkey,vn_handle;
  uint8_t  node_found_b = FALSE , relative_node_found = FALSE;
  uint32_t offset;
  int32_t  retval;

   key = global_table_3_repository_key_p;

  retval = crm_get_vn_handle(key->vn_name,&vn_handle);
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

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

  nsc_table_3_repository_table_g = vn_nsc_info_p->nsc_repository_table_3_p;

  if(nsc_table_3_repository_table_g == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No entries in table 1 repository");
     return TSC_FAILURE;
  }

  offset = 0;
  CNTLR_RCU_READ_LOCK_TAKE(); 
  MCHASH_TABLE_SCAN(nsc_table_3_repository_table_g,hashkey)
  {
     MCHASH_BUCKET_SCAN(nsc_table_3_repository_table_g,hashkey,nsc_repository_scan_node_p,
                                                  struct ucastpkt_outport_repository_entry*,offset)
     {
       if(node_found_b == FALSE)
       {
          if(nsc_repository_scan_node_p->safe_reference == relative_saferef)
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Relative node found");
            node_found_b = TRUE;
          }
          continue;
       }
       relative_node_found = TRUE;
       strcpy(nsc_repository_entry_p->local_switch_name_p , nsc_repository_scan_node_p->local_switch_name_p);
       strcpy(nsc_repository_entry_p->remote_switch_name_p , nsc_repository_scan_node_p->remote_switch_name_p);
       nsc_repository_entry_p->dp_handle        =        nsc_repository_scan_node_p->dp_handle;
       nsc_repository_entry_p->nid              =        nsc_repository_scan_node_p->nid;
       nsc_repository_entry_p->port_type        =        nsc_repository_scan_node_p->port_type;
       nsc_repository_entry_p->serviceport      =        nsc_repository_scan_node_p->serviceport;
       nsc_repository_entry_p->tun_dest_ip      =        nsc_repository_scan_node_p->tun_dest_ip;
       nsc_repository_entry_p->nw_port_b        =        nsc_repository_scan_node_p->nw_port_b;
       nsc_repository_entry_p->out_port_no      =        nsc_repository_scan_node_p->out_port_no;
       memcpy(nsc_repository_entry_p->dst_mac,nsc_repository_scan_node_p->dst_mac ,6);

       nsc_repository_entry_p->safe_reference   =       nsc_repository_scan_node_p->safe_reference;
       *match_saferef = nsc_repository_scan_node_p->safe_reference;
       break;
    }
    if(node_found_b == TRUE && relative_node_found == TRUE)
      break;
  }
  if(node_found_b == FALSE || relative_node_found == FALSE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No repository found or reached last bucket");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return TSC_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Get next record successful ! ");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return TSC_SUCCESS;
}

int32_t nsc_get_exact_table_3_repository_entry(void)

{
   if(flag_3 == 0)
   {
      flag_3++;
      return TSC_FAILURE;
   }
  else
      return TSC_SUCCESS;
}
