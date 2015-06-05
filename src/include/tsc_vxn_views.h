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
 * File name: tsc_vxn_views.h
 * Author: ns murthy(b37840@freescale.com), Varun Kumar Reddy <B36461@freescale.com>
 * Date:   03/05/2014
 * Description:internal views  on crm database for vxlan nvm.

*//***************************************************************************/
#ifndef __TSCVXNINTRVIEW_H
#define __TSCVXNINTRVIEW_H

#define TSC_VXN_TUNNID_VIEW_OF_VN                  "vxn-vni-VN"
#define TSC_VXN_DSTMAC_VIEW_OF_VMPORTS             "vxn-dstmac-VMPORTS"
#define TSC_VXN_SWNAME_VIEW_OF_VMPORTS             "vxn-swname-VMPORTS"
#define TSC_VXN_SWNAME_VIEW_OF_UNICAST_NWPORTS     "vxn-swname-UCASTNWPORTS"
#define TSC_VXN_SWNAME_VIEW_OF_BROADCAST_NWPORTS   "vxn-swname-BCASTNWPORTS"
#define TSC_VXN_VIEW_RESERVED                      "reserved" /* Remove if not required */

/* NSM TBD Define separate values for each Hash Table based on some logic at a central place. */                                                                         
#define TSC_VXN_MAX_VIEW_NODE_ENTRIES 64
#define TSC_VXN_MAX_VIEW_STATIC_NODE_ENTRIES 16

/* This structure represents view of dmac-vmports.This structure holds tsc_vxn_dmac_vmport_view_node info */
struct tsc_vxn_dmac_vmport_database_view
{
  char     view[64];			/* view name */
  struct   mchash_table* db_viewtbl_p;  /* Hash table holding the tsc_vxn_dmac_vmport_view_node nodes. */
  void*    db_viewmempool_p; 
  uint32_t no_of_view_node_buckets_g;
};

/* This structure represents one node in the hash table of a given tsc_vxn_dmac_vmport_database_view. */
struct tsc_vxn_dmac_vmport_view_node
{
  struct   mchash_table_link  view_tbl_lnk; /* to link this view node to the tsc_vxn_dmac_vmport_database_view Hash table */
  uint8_t  dst_mac[6]; 		            /* dst_mac is view node name*/
  uint32_t nid;      		            /* nid is the view node value */
  uint64_t port_handle;                     /* crm_port handle */
  int32_t  index;
  int32_t  magic;
  uint8_t  heap_b;
};
#define TSC_VXN_DMAC_VMPORT_VIEW_NODE_TBL_OFFSET offsetof(struct tsc_vxn_dmac_vmport_view_node, view_tbl_lnk)

/* This structure represents view of swname-vmports.This structure holds tsc_vxn_swname_vmport_view_node info*/
struct tsc_vxn_swname_vmport_database_view
{
  char     view[64];                     
  struct   mchash_table* db_viewtbl_p;   
  void*    db_viewmempool_p; 
  uint32_t no_of_view_node_buckets_g;
  uint8_t  uniqueness_flag;                 /* uniqueness_flag =1 node_name is not used in hashing. */
  uint8_t  valid_b;                         /* validity */
};

struct tsc_vxn_swname_vmport_view_node
{
  struct   mchash_table_link  view_tbl_lnk;  
  char     swname[64];                      /* valid for swname view. */
  uint32_t nid;
  uint64_t port_handle;		      	   
  int32_t  index;
  int32_t  magic;
  uint8_t  heap_b;
};
#define TSC_VXN_SWNAME_VMPORT_VIEW_NODE_TBL_OFFSET offsetof(struct tsc_vxn_swname_vmport_view_node, view_tbl_lnk)

/* This structure represents view of swname-nw_unicast ports.This structure holds tsc_vxn_swname_nw_unicast_port_view_node */
struct tsc_vxn_swname_unicast_nwport_database_view
{
  char     view[64];			     
  struct   mchash_table* db_viewtbl_p;       
  void*    db_viewmempool_p; 
  uint32_t no_of_view_node_buckets_g;
  uint8_t  uniqueness_flag; 	             
  uint8_t  valid_b;                          
};

struct tsc_vxn_swname_unicast_nwport_view_node
{
  struct   mchash_table_link  view_tbl_lnk;  
  char     swname[64];                       /* valid for swname view. */
  uint16_t service_port;
  uint64_t port_handle;		            
  int32_t  index;
  int32_t  magic;
  uint8_t  heap_b;
};
#define TSC_VXN_SWNAME_UNICAST_NWPORT_VIEW_NODE_TBL_OFFSET offsetof(struct tsc_vxn_swname_unicast_nwport_view_node, view_tbl_lnk)

/* This structure represents view of swname-nw_broadcast ports.This structure holds tsc_vxn_swname_unicast_nwport_view_node */
struct tsc_vxn_swname_broadcast_nwport_database_view
{
  char     view[64];			   
  struct   mchash_table*  db_viewtbl_p;    
  void*    db_viewmempool_p; 
  uint32_t no_of_view_node_buckets_g;
  uint8_t  uniqueness_flag;                
  uint8_t  valid_b;    		           
};
struct tsc_vxn_swname_broadcast_nwport_view_node
{
  struct   mchash_table_link  view_tbl_lnk;  
  char     swname[64];			     /* valid for swname view. */
  uint16_t serviceport;
  uint32_t remote_ip;
  uint32_t nid;
  uint64_t port_handle;                      
  int32_t  index;
  int32_t  magic;
  uint8_t  heap_b;
};
#define TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET offsetof(struct tsc_vxn_swname_broadcast_nwport_view_node, view_tbl_lnk)

struct tsc_vxn_nid_vn_database_view
{
  char     view[64];			
  struct   mchash_table *db_viewtbl_p;	
  void*    db_viewmempool_p;
  uint32_t no_of_view_node_buckets_g;
  uint8_t  uniqueness_flag;        	
  uint8_t  valid_b;                     
};

struct tsc_vxn_nid_vn_view_node
{
  struct   mchash_table_link  view_tbl_lnk;  
  uint32_t nid;
  uint64_t vn_handle;                        /* safe reference to the vn node */
  int32_t  index;
  int32_t  magic;
  uint8_t  heap_b;
};
#define TSC_VXN_TUNID_VN_VIEW_NODE_TBL_OFFSET offsetof(struct tsc_vxn_nid_vn_view_node, view_tbl_lnk)

#define RCUNODE_OFFSET_IN_MCHASH_TBLLNK offsetof( struct mchash_table_link, rcu_head)

/* TSC_VXN Internal views of crm_port initialization */
void    tsc_vxn_get_view_node_mempoolentries(uint32_t* view_node_entries_max,uint32_t* view_node_static_entries);

int32_t tsc_vxn_vmport_portmac_view_init(char *view_name_p);
int32_t tsc_vxn_vmport_portmac_view_uninit();
int32_t tsc_vxn_add_vmport_to_portmac_view(uint64_t crm_port_handle,uint32_t  nid);
int32_t tsc_vxn_del_vmport_from_portmac_view(uint64_t crm_port_handle, uint32_t  nid);
int32_t tsc_vxn_get_vmport_by_dmac_nid(uint8_t*  dst_mac_p, uint32_t  nid, uint64_t* crm_port_handle_p);

int32_t tsc_vxn_vmport_swname_view_init(char *view_name_p);
int32_t tsc_vxn_vmport_swname_view_uninit();
int32_t tsc_vxn_add_vmport_to_swname_view(uint64_t crm_port_handle,  uint32_t nid);
int32_t tsc_vxn_del_vmport_from_swname_view(uint64_t crm_port_handle,uint32_t nid);
int32_t tsc_vxn_get_vmports_by_swname_nid(char *switch_name_p,uint32_t nid,uint32_t* no_of_vmside_ports_p,uint32_t* vmside_ports_p);

int32_t tsc_vxn_unicast_nwport_swname_view_init(char *view_name_p);
int32_t tsc_vxn_unicast_nwport_swname_view_uninit();
int32_t tsc_vxn_add_unicast_nwport_to_swname_view(uint64_t crm_port_handle,  uint16_t service_port);
int32_t tsc_vxn_del_unicast_nwport_from_swname_view(uint64_t crm_port_handle,uint16_t service_port);
int32_t tsc_vxn_get_unicast_nwport_by_swname_sp(char* switch_name_p, uint16_t serviceport, uint32_t* out_port_no_p);

int32_t tsc_vxn_broadcast_nwport_swname_view_init(char *view_name_p);
int32_t tsc_vxn_broadcast_nwport_swname_view_uninit();
int32_t tsc_vxn_add_broadcast_nwport_to_swname_view(uint64_t crm_port_handle,uint16_t service_port,uint32_t nid,uint32_t remote_ip);
int32_t tsc_vxn_del_broadcast_nwport_from_swname_view(uint64_t crm_port_handle,uint16_t service_port,uint32_t nid,uint32_t remote_ip);
int32_t tsc_vxn_get_remote_ips_by_swname_nid_sp(char* switch_name_p,uint32_t service_port,uint32_t nid,uint32_t* no_of_remote_ips_p,uint32_t* remote_ips_p);

int32_t tsc_vxn_get_broadcast_nwports_by_swname_nid_sp(char* switch_name_p,uint32_t service_port,uint32_t nid,uint32_t* no_of_nwside_br_ports_p, uint32_t*  nwside_br_ports_p,uint32_t remote_ip);

int32_t tsc_vxn_vn_nid_view_init(char *view_name_p);
int32_t tsc_vxn_vn_nid_view_uninit();
int32_t tsc_vxn_add_vn_to_nid_view(uint64_t crm_vn_handle,uint32_t nid);
int32_t tsc_vxn_delete_vn_from_nid_view(uint64_t crm_vn_handle, uint32_t nid);
int32_t tsc_vxn_get_vnhandle_by_nid(uint32_t nid,uint64_t* vn_handle_p);

void    tsc_vxn_dmac_vmport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p);
void    tsc_vxn_swname_vmport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p);
void    tsc_vxn_swname_unicast_nwport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p);
void    tsc_vxn_swname_broadcast_nwport_free_view_node_rcu(struct rcu_head *db_view_node_entry_p);
void    tsc_vxn_nid_vn_free_view_node_rcu(struct rcu_head *db_view_node_entry_p);

uint32_t tsc_vxn_get_hashval_bymac(uint8_t *mac_p,uint32_t no_of_buckets);
uint32_t tsc_vxn_get_hashval_byname(char* name_p,uint32_t no_of_buckets);
uint32_t tsc_vxn_get_hashval_by_nid(uint32_t nid,uint32_t no_of_buckets);

#endif
