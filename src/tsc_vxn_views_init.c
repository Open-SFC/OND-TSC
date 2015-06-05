/*
 *
 * Copyright  2012, 2013  Freescale Semiconductor
 *
 *
 * Licensed under the Apache License, version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
*/
/*
 *
 * File name: tsc_vxn_view_init.c
 * Author: ns murthy(b37840@freescale.com), Varun Kumar Reddy <B36461@freescale.com>
 * Date:   03/05/2014
 * Description:internal views  on crm database for vxlan nvm.
*/

#include "controller.h"
#include "cntlr_epoll.h"
#include "cntrl_queue.h"
#include "of_utilities.h"
#include "of_msgapi.h"
#include "of_multipart.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_vxn_views.h"

struct tsc_vxn_dmac_vmport_database_view              dmac_vmport_view;
struct tsc_vxn_swname_vmport_database_view            swname_vmport_view;
struct tsc_vxn_swname_unicast_nwport_database_view    swname_unicast_nwport_view;
struct tsc_vxn_swname_broadcast_nwport_database_view  swname_broadcast_nwport_view;
struct tsc_vxn_nid_vn_database_view                   nid_vn_view;

void tsc_vxn_dmac_vmport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p)
{
  int32_t  ret_val;
  uint32_t offset;
  struct   tsc_vxn_dmac_vmport_view_node *db_view_node_info_p = (struct tsc_vxn_dmac_vmport_view_node*)db_view_node_entry_p;

  if(db_view_node_entry_p)
  {
    offset = TSC_VXN_DMAC_VMPORT_VIEW_NODE_TBL_OFFSET;
    db_view_node_info_p = (struct tsc_vxn_dmac_vmport_view_node*)(((char *)db_view_node_entry_p-RCUNODE_OFFSET_IN_MCHASH_TBLLNK)-offset);
    ret_val = mempool_release_mem_block(dmac_vmport_view.db_viewmempool_p,(uchar8_t*)db_view_node_info_p,db_view_node_info_p->heap_b);
    if(ret_val == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "memory block released succussfully!.");
    }
    else
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "failed to release memory block");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," NULL passed for deletion.");
  }
}

void tsc_vxn_swname_vmport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p)
{
  int32_t  ret_val;
  uint32_t offset;
  struct tsc_vxn_swname_vmport_view_node *db_view_node_info_p = (struct tsc_vxn_swname_vmport_view_node*)db_view_node_entry_p;
  
  if(db_view_node_entry_p)
  {
    offset = TSC_VXN_SWNAME_VMPORT_VIEW_NODE_TBL_OFFSET;
    db_view_node_info_p = (struct tsc_vxn_swname_vmport_view_node*)(((char *)db_view_node_entry_p-RCUNODE_OFFSET_IN_MCHASH_TBLLNK)-offset);
    ret_val = mempool_release_mem_block(swname_vmport_view.db_viewmempool_p,(uchar8_t*)db_view_node_info_p,db_view_node_info_p->heap_b);
    if(ret_val == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "memory block released succussfully!.");
    }
    else
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "failed to release memory block");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," NULL passed for deletion.");
  }
}

void tsc_vxn_swname_unicast_nwport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p)
{
  int32_t  ret_val;
  uint32_t offset;
  struct   tsc_vxn_swname_unicast_nwport_view_node *db_view_node_info_p = (struct tsc_vxn_swname_unicast_nwport_view_node*)db_view_node_entry_p;

  if(db_view_node_entry_p)
  {
    offset = TSC_VXN_SWNAME_UNICAST_NWPORT_VIEW_NODE_TBL_OFFSET;
    db_view_node_info_p = (struct tsc_vxn_swname_unicast_nwport_view_node*)(((char *)db_view_node_entry_p-RCUNODE_OFFSET_IN_MCHASH_TBLLNK)-offset);
    ret_val = mempool_release_mem_block(swname_unicast_nwport_view.db_viewmempool_p,(uchar8_t*)db_view_node_info_p,db_view_node_info_p->heap_b);
    if(ret_val == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "memory block released succussfully!.");
    }
    else
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "failed to release memory block");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," NULL passed for deletion.");
  }
}

void tsc_vxn_swname_broadcast_nwport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p)
{
  int32_t  ret_val;
  uint32_t offset;
  struct tsc_vxn_swname_broadcast_nwport_view_node *db_view_node_info_p=(struct tsc_vxn_swname_broadcast_nwport_view_node*)db_view_node_entry_p;
  
  if(db_view_node_entry_p)
  {
    offset = TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET;

    db_view_node_info_p = (struct tsc_vxn_swname_broadcast_nwport_view_node*)(((char *)db_view_node_entry_p-RCUNODE_OFFSET_IN_MCHASH_TBLLNK)-offset);

    ret_val = mempool_release_mem_block(swname_broadcast_nwport_view.db_viewmempool_p,(uchar8_t*)db_view_node_info_p,db_view_node_info_p->heap_b);

    if(ret_val == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "memory block released succussfully!.");
    }
    else
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "failed to release memory block");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," NULL passed for deletion.");
  }
}

void tsc_vxn_nid_vn_free_view_node_rcu(struct rcu_head *db_view_node_entry_p)
{
  int32_t  ret_val;
  uint32_t offset;

  struct tsc_vxn_nid_vn_view_node *db_view_node_info_p=(struct tsc_vxn_nid_vn_view_node*)db_view_node_entry_p;
  if(db_view_node_entry_p)
  {
    offset = TSC_VXN_TUNID_VN_VIEW_NODE_TBL_OFFSET;
    db_view_node_info_p = (struct tsc_vxn_nid_vn_view_node*)(((char *)db_view_node_entry_p-RCUNODE_OFFSET_IN_MCHASH_TBLLNK)-offset);
    ret_val = mempool_release_mem_block(nid_vn_view.db_viewmempool_p,(uchar8_t*)db_view_node_info_p,db_view_node_info_p->heap_b);
    if(ret_val == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "memory block released succussfully!.");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "failed to release memory block");
    }
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," NULL passed for deletion.");
  }
}

void tsc_vxn_get_view_node_mempoolentries(uint32_t* view_node_entries_max,uint32_t* view_node_static_entries)
{
  *view_node_entries_max    = TSC_VXN_MAX_VIEW_NODE_ENTRIES;
  *view_node_static_entries = TSC_VXN_MAX_VIEW_STATIC_NODE_ENTRIES;
}
/******************************************************************************
* Funcation : tsc_vxn_vmport_portmac_view_init
* Description:
*     This function  creates mempool and hash table for
*     dmac-nid view. each node in hash table maintains the dmac , nid and
*     crm_port handle.based on mac and nid gives the crm_port info     
******************************************************************************/
int32_t tsc_vxn_vmport_portmac_view_init(char *view_name_p)
{
  int32_t  ret_value;
  uint32_t view_node_entries_max,view_node_static_entries;
  struct   mempool_params mempool_info = {};

  /** creating mempool and hash table for dmac-vmport views**/
  tsc_vxn_get_view_node_mempoolentries(&view_node_entries_max, &view_node_static_entries);


  mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
  mempool_info.block_size = sizeof(struct tsc_vxn_dmac_vmport_view_node);
  mempool_info.max_blocks = view_node_entries_max;
  mempool_info.static_blocks = view_node_static_entries;
  mempool_info.threshold_min = view_node_static_entries/10;
  mempool_info.threshold_max = view_node_static_entries/3;
  mempool_info.replenish_count = view_node_static_entries/10;
  mempool_info.b_memset_not_required = FALSE;
  mempool_info.b_list_per_thread_required = FALSE;
  mempool_info.noof_blocks_to_move_to_thread_list = 0;


  ret_value = mempool_create_pool("tsc_vxn_dmac_vmport_db_view_pool",
      &mempool_info,
      &(dmac_vmport_view.db_viewmempool_p)
      );
  if(ret_value != MEMPOOL_SUCCESS)
    return CRM_FAILURE;

  ret_value = mchash_table_create(((view_node_entries_max/5)+1),
      view_node_entries_max,
      tsc_vxn_dmac_vmport_free_view_node_rcu,
      &(dmac_vmport_view.db_viewtbl_p));
  if(ret_value != MCHASHTBL_SUCCESS)
    return CRM_FAILURE;

  dmac_vmport_view.no_of_view_node_buckets_g = (view_node_entries_max/5) + 1;
  strcpy(dmac_vmport_view.view,view_name_p);
  return CRM_SUCCESS;
}

int32_t tsc_vxn_vmport_portmac_view_uninit()
{
  int32_t ret_value;
  
  ret_value = mchash_table_delete(dmac_vmport_view.db_viewtbl_p);
  if(ret_value != MCHASHTBL_SUCCESS)
    return ret_value;

  ret_value = mempool_delete_pool(dmac_vmport_view.db_viewmempool_p);
  if(ret_value != MEMPOOL_SUCCESS)
    return ret_value;

  return CRM_SUCCESS;
}
/******************************************************************************
* Funcation : tsc_vxn_vmport_swname_view_init
* Description:
*     This function  creates mempool and hash table for
*     swname-nid view. each node in hash table maintains the swname , nid and
*     crm_port handle.based on swname and nid gives the crm_port info .
******************************************************************************/
int32_t tsc_vxn_vmport_swname_view_init(char *view_name_p)
{
  int32_t  ret_value;
  uint32_t view_node_entries_max,view_node_static_entries;
  struct   mempool_params mempool_info = {};
  /** creating mempool and hash table for flow control dmac-vmport views**/
  tsc_vxn_get_view_node_mempoolentries(&view_node_entries_max, &view_node_static_entries);

  mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
  mempool_info.block_size = sizeof(struct tsc_vxn_swname_vmport_view_node);
  mempool_info.max_blocks = view_node_entries_max;
  mempool_info.static_blocks = view_node_static_entries;
  mempool_info.threshold_min = view_node_static_entries/10;
  mempool_info.threshold_max = view_node_static_entries/3;
  mempool_info.replenish_count = view_node_static_entries/10;
  mempool_info.b_memset_not_required = FALSE;
  mempool_info.b_list_per_thread_required = FALSE;
  mempool_info.noof_blocks_to_move_to_thread_list = 0;


  ret_value = mempool_create_pool("tsc_vxn_swname_vmport_db_view_pool",
      &mempool_info,
      &(swname_vmport_view.db_viewmempool_p)
      );
  if(ret_value != MEMPOOL_SUCCESS)
    return CRM_FAILURE;

  ret_value = mchash_table_create(((view_node_entries_max/5)+1),
      view_node_entries_max,
      tsc_vxn_swname_vmport_free_view_node_rcu,
      &(swname_vmport_view.db_viewtbl_p));
  if(ret_value != MCHASHTBL_SUCCESS)
    return CRM_FAILURE;

  swname_vmport_view.no_of_view_node_buckets_g = (view_node_entries_max/5) + 1;
  strcpy(swname_vmport_view.view,view_name_p);
  return CRM_SUCCESS;
}

int32_t tsc_vxn_vmport_swname_view_uninit()
{
  int32_t ret_value;
  ret_value = mchash_table_delete(swname_vmport_view.db_viewtbl_p);
  if(ret_value != MCHASHTBL_SUCCESS)
    return ret_value;

  ret_value = mempool_delete_pool(swname_vmport_view.db_viewmempool_p);
  if(ret_value != MEMPOOL_SUCCESS)
    return ret_value;
  
  return CRM_SUCCESS;
}
/******************************************************************************
* Funcation :tsc_vxn_unicast_nwport_swname_view_init
* Description:
*     This function  creates mempool and hash table for
*     swname- serviceport view for network unicast ports. 
*     each node in hash table maintains the swname , nid and
*     crm_port handle.
******************************************************************************/
int32_t tsc_vxn_unicast_nwport_swname_view_init(char *view_name_p)
{
  int32_t  ret_value;
  uint32_t view_node_entries_max,view_node_static_entries;
  struct   mempool_params mempool_info = {};
  /** creating mempool and hash table for flow control dmac-vmport views**/
  tsc_vxn_get_view_node_mempoolentries(&view_node_entries_max, &view_node_static_entries);
  mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
  mempool_info.block_size = sizeof(struct tsc_vxn_swname_unicast_nwport_view_node);
  mempool_info.max_blocks = view_node_entries_max;
  mempool_info.static_blocks = view_node_static_entries;
  mempool_info.threshold_min = view_node_static_entries/10;
  mempool_info.threshold_max = view_node_static_entries/3;
  mempool_info.replenish_count = view_node_static_entries/10;
  mempool_info.b_memset_not_required = FALSE;
  mempool_info.b_list_per_thread_required = FALSE;
  mempool_info.noof_blocks_to_move_to_thread_list = 0;


  ret_value = mempool_create_pool("tsc_vxn_swname_unicast_nwport_db_view_pool",
      &mempool_info,
      &(swname_unicast_nwport_view.db_viewmempool_p)
      );
  if(ret_value != MEMPOOL_SUCCESS)
    return CRM_FAILURE;

  ret_value = mchash_table_create(((view_node_entries_max/5)+1),
      view_node_entries_max,
      tsc_vxn_swname_unicast_nwport_free_view_node_rcu,
      &(swname_unicast_nwport_view.db_viewtbl_p));
  if(ret_value != MCHASHTBL_SUCCESS)
    return CRM_FAILURE;

  swname_unicast_nwport_view.no_of_view_node_buckets_g = (view_node_entries_max/5) + 1;
  strcpy(swname_unicast_nwport_view.view,view_name_p);
  return CRM_SUCCESS;
}

int32_t tsc_vxn_unicast_nwport_swname_view_uninit()
{
  int32_t ret_value;
  
  ret_value = mchash_table_delete(swname_unicast_nwport_view.db_viewtbl_p);
  if(ret_value != MCHASHTBL_SUCCESS)
    return ret_value;

  ret_value = mempool_delete_pool(swname_unicast_nwport_view.db_viewmempool_p);
  if(ret_value != MEMPOOL_SUCCESS)
    return ret_value;
  return CRM_SUCCESS;
}
/******************************************************************************
* Funcation :tsc_vxn_broadcast_nwport_swname_view_init
* Description:
*     This function  creates mempool and hash table for
*     swname- serviceport and remoteip view for network broad cast ports. 
*     each node in hash table maintains the swname , serviceport,remoteip and
*     crm_port handle.
******************************************************************************/
int32_t tsc_vxn_broadcast_nwport_swname_view_init(char *view_name_p)
{
  int32_t  ret_value;
  uint32_t view_node_entries_max,view_node_static_entries;
  struct   mempool_params mempool_info = {};

  /** creating mempool and hash table for flow control swname-broadcast_nwport views**/
  tsc_vxn_get_view_node_mempoolentries(&view_node_entries_max, &view_node_static_entries);

  mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
  mempool_info.block_size = sizeof(struct tsc_vxn_swname_broadcast_nwport_view_node);
  mempool_info.max_blocks = view_node_entries_max;
  mempool_info.static_blocks = view_node_static_entries;
  mempool_info.threshold_min = view_node_static_entries/10;
  mempool_info.threshold_max = view_node_static_entries/3;
  mempool_info.replenish_count = view_node_static_entries/10;
  mempool_info.b_memset_not_required = FALSE;
  mempool_info.b_list_per_thread_required = FALSE;
  mempool_info.noof_blocks_to_move_to_thread_list = 0;


  ret_value = mempool_create_pool("tsc_vxn_swname_broadcast_nwport_db_view_pool",
      &mempool_info,
      &(swname_broadcast_nwport_view.db_viewmempool_p)
      );
  if(ret_value != MEMPOOL_SUCCESS)
    return CRM_FAILURE;

  ret_value = mchash_table_create(((view_node_entries_max/5)+1),
      view_node_entries_max,
      tsc_vxn_swname_broadcast_nwport_free_view_node_rcu,
      &(swname_broadcast_nwport_view.db_viewtbl_p));
  if(ret_value != MCHASHTBL_SUCCESS)
    return CRM_FAILURE;

  swname_broadcast_nwport_view.no_of_view_node_buckets_g = (view_node_entries_max/5) + 1;
  strcpy(swname_broadcast_nwport_view.view,view_name_p);
  return CRM_SUCCESS;
}

int32_t tsc_vxn_broadcast_nwport_swname_view_uninit()
{
  int32_t ret_value;
  
  ret_value = mchash_table_delete(swname_broadcast_nwport_view.db_viewtbl_p);
  if(ret_value != MCHASHTBL_SUCCESS)
    return ret_value;

  ret_value = mempool_delete_pool(swname_broadcast_nwport_view.db_viewmempool_p);
  if(ret_value != MEMPOOL_SUCCESS)
    return ret_value;
  return CRM_SUCCESS;
}

/******************************************************************************
* Funcation : tsc_vxn_vn_nid_view_init
* Description:
*     This function  creates mempool and hash table for
*     nid view of virtual network. Each node in hash table maintains the nid and
*     vn handle.based on nid gives the vn info.
******************************************************************************/
int32_t tsc_vxn_vn_nid_view_init(char *view_name_p)
{
  int32_t  ret_value;
  uint32_t view_node_entries_max,view_node_static_entries;
  struct   mempool_params mempool_info = {};
  /** creating mempool and hash table for flow control dmac-vmport views**/
  tsc_vxn_get_view_node_mempoolentries(&view_node_entries_max, &view_node_static_entries);
  
  mempool_info.mempool_type = MEMPOOL_TYPE_HEAP;
  mempool_info.block_size = sizeof(struct tsc_vxn_nid_vn_view_node);
  mempool_info.max_blocks = view_node_entries_max;
  mempool_info.static_blocks = view_node_static_entries;
  mempool_info.threshold_min = view_node_static_entries/10;
  mempool_info.threshold_max = view_node_static_entries/3;
  mempool_info.replenish_count = view_node_static_entries/10;
  mempool_info.b_memset_not_required = FALSE;
  mempool_info.b_list_per_thread_required = FALSE;
  mempool_info.noof_blocks_to_move_to_thread_list = 0;


  ret_value = mempool_create_pool("tsc_vxn_nid_vn_db_view_pool",
      &mempool_info,
      &(nid_vn_view.db_viewmempool_p)
      );
  if(ret_value != MEMPOOL_SUCCESS)
    return CRM_FAILURE;

  ret_value = mchash_table_create(((view_node_entries_max/5)+1),
      view_node_entries_max,
      tsc_vxn_nid_vn_free_view_node_rcu,
      &(nid_vn_view.db_viewtbl_p));
  if(ret_value != MCHASHTBL_SUCCESS)
    return CRM_FAILURE;

  nid_vn_view.no_of_view_node_buckets_g = (view_node_entries_max/5) + 1;
  strcpy(nid_vn_view.view,view_name_p);
  return CRM_SUCCESS;
}

int32_t tsc_vxn_vn_nid_view_uninit()
{
  int32_t ret_value;
  
  ret_value = mchash_table_delete(nid_vn_view.db_viewtbl_p);
  if(ret_value != MCHASHTBL_SUCCESS)
    return ret_value;

  ret_value = mempool_delete_pool(nid_vn_view.db_viewmempool_p);
  if(ret_value != MEMPOOL_SUCCESS)
    return ret_value;

  return CRM_SUCCESS;
}
/******************************************************************************************************/
