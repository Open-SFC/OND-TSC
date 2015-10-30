/* * Copyright (c) 2012, 2013  Freescale.
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

/* File: tsc_controller.h
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains Flow Manager functionality of the Flow control module.
 */

/* INCLUDE_START ************************************************************/
/* INCLUDE_END **************************************************************/

#ifndef __TSC_CONTROLLER__
#define __TSC_CONTROLLER__

#define TSC_APP_CLASSIFY_TABLE_ID_0            0
#define TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1   1
#define TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2    2
#define TSC_APP_UNICAST_TABLE_ID_3             3
#define TSC_APP_BROADCAST_OUTBOUND_TABLE_ID_4  4
#define TSC_APP_BROADCAST_INBOUND_TABLE_ID_5   5

/* metadata fields and positions. */
#define NW_TYPE_OFFSET      60     /* 04 bits */ 
#define PKT_ORIGIN_OFFSET   56     /* 04 bits */
#define PKT_ORIGIN_MASK     0x0F00000000000000
#define EMPTY               51     /* 12      */
#define TSC_SWITCH_NAME_LEN 64

#define NSC_NSCHAIN_REPOSITORY_NODE_OFFSET   offsetof(struct nschain_repository_entry,nschain_repository_tbl_link)
#define NSC_UCASTPKT_REPOSITORY_NODE_OFFSET  offsetof(struct ucastpkt_outport_repository_entry,ucastpkt_outport_repository_tbl_link)
#define TSC_ZONE_NODE_OFFSET                 offsetof(struct tsc_zone_entry,tsc_zone_tbl_link)

#define PSP_CACHE_LINE_SIZE     64 /* x86 */

#define CACHE_ALIGN __attribute__((aligned(PSP_CACHE_LINE_SIZE)))
#define PAD_CACHE_LINE uchar8_t cachelinepad[0] CACHE_ALIGN;

#define NW_DIRECTION_IN   1
#define NW_DIRECTION_OUT  2

#include "tsc_nsc_core.h" 

struct tsc_ofver_plugin_iface
{
  uint32_t in_port_id;
  uint64_t metadata;     
  uint64_t metadata_mask;

  uint8_t  nw_type;  
 
  uint64_t dp_handle;

  char     local_switch_name[TSC_SWITCH_NAME_LEN];
  char     remote_switch_name[TSC_SWITCH_NAME_LEN];

  uint32_t  no_of_nwside_br_ports;
  uint32_t* nwside_br_ports_p;

  /* VxLan Specific parameters */
  uint32_t vni;
  uint64_t tun_id;
  uint16_t serviceport;
  
  int32_t  pkt_origin_e; 
  uint16_t vlan_id_in;  
  uint16_t vlan_id_out;  
  uint8_t  flow_mod_priority;
  int32_t  pkt_type_e;  /* Unicast or Broadcast */

  uint32_t out_port_no;
  /* Goto Instruction */
  uint8_t next_table_id;
};

enum tsc_pkt_type_e
{
  FCFM_PKT_TYPE_UNICAST,
  FCFM_PKT_TYPE_BROADCAST,
  FCFM_PKT_TYPE_MULTICAST,
};

enum tsc_pkt_origin_e
{
  VM_APPLN,
  VM_NS,
  VM_NS_IN,
  VM_NS_OUT,
  NW_UNICAST_SIDE,
  NW_BROADCAST_SIDE,
};

enum tsc_zone_e
{
  ZONE_LESS  = 1,
  ZONE_LEFT  = 2,
  ZONE_RIGHT = 3,
};


struct tsc_zone_entry
{
  struct    mchash_table_link tsc_zone_tbl_link;
  uint32_t  vm_port_ip;
  char      zone[CRM_MAX_ZONE_SIZE + 1];
  uint8_t   nw_type;
  uint32_t  nid;
  uint32_t  zone_direction; /* only used as a cache */
  uint8_t   heap_b;
  uint32_t  index;
  uint32_t  magic;
};

#define PSP_CACHE_LINE_SIZE     64
#define CACHE_ALIGN __attribute__((aligned(PSP_CACHE_LINE_SIZE)))
#define PAD_CACHE_LINE uchar8_t cachelinepad[0] CACHE_ALIGN;

struct nschain_repository_entry
{
  struct    mchash_table_link nschain_repository_tbl_link;
  char      local_switch_name_p[TSC_SWITCH_NAME_LEN];
  uint64_t  dp_handle;
  uint8_t   nw_type;
  uint8_t   out_nw_type;
  uint32_t  nid;
  uint32_t  out_nid;
  uint16_t  serviceport;
  uint32_t  remote_switch_ip;
  uint32_t  in_port_id;
  uint64_t  metadata;

  uint64_t  metadata_mask;  
  uint8_t   pkt_origin;

  uint8_t   ns_chain_b;
  uint8_t   more_nf_b;
  uint8_t   ns_port_b;

  uint16_t  vlan_id_pkt;
  uint16_t  match_vlan_id;
  uint16_t  next_vlan_id;

  uint8_t   nw_port_b;
  uint32_t  out_port_no;
  uint32_t  tun_dest_ip;    /* only used by list_selectors.c */ 
  uint8_t   next_table_id;

  uint8_t   flow_mod_priority;

  uint8_t   service_chaining_type;
  struct    nsc_selector_node selector;  /* packet fields */

  uint8_t   local_in_mac[6];
  uint8_t   local_out_mac[6];
 
  uint8_t   copy_local_mac_addresses_b;
  uint8_t   copy_original_mac_addresses_b;
  uint32_t  zone;
  /* usedin proactive flow pushing to all the switches. */
  uint8_t   vm_and_service1_not_on_same_switch;

  uint8_t   skip_this_entry; /* Deleted in switch when a service is deleted */

  uint32_t  index;
  uint32_t  magic;
  uint64_t  safe_reference;
  uint8_t   heap_b;
        PAD_CACHE_LINE;
}CACHE_ALIGN;

struct crm_port_tsc_info 
{
  uint64_t switch_handle;
  uint64_t dprm_br1_saferef;
  uint64_t dprm_port_handle;
  
  uint32_t switch_ip;
  uint32_t port_id;
  uint8_t  port_status;
};

struct ucastpkt_outport_repository_entry
{
  struct    mchash_table_link ucastpkt_outport_repository_tbl_link;  
  char      local_switch_name_p[TSC_SWITCH_NAME_LEN];
  char      remote_switch_name_p[TSC_SWITCH_NAME_LEN];
  uint64_t  dp_handle;
  uint8_t   nw_type;
  uint8_t   priority;
  uint32_t  nid;
  uint64_t  tun_id;
  uint64_t  metadata;

  uint64_t  metadata_mask;  
  uint16_t  serviceport;
  uint16_t  idle_time;
  uint8_t   dst_mac[6];
  uint32_t  dst_mac_high;
  uint32_t  dst_mac_low;
  uint8_t   nw_port_b;
  uint32_t  tun_dest_ip;
  uint8_t   port_type;
  uint32_t  out_port_no;
  uint32_t  index;
  uint32_t  magic;
  uint64_t  safe_reference;
  uint8_t   heap_b;
        PAD_CACHE_LINE;  
}CACHE_ALIGN;

struct vmports_repository_entry
{
  char      local_switch_name_p[TSC_SWITCH_NAME_LEN];
  uint8_t   nw_type;
  uint32_t  nid;
  uint64_t  metadata;
  
  uint64_t  metadata_mask; 
  uint32_t  no_of_vmside_ports; 
  uint32_t* vmside_ports_p;
};

struct nwports_brdcast_repository_entry
{
  char      local_switch_name_p[TSC_SWITCH_NAME_LEN];
  uint8_t   nw_type;
  uint32_t  nid;
  uint64_t  metadata;
  uint64_t  metadata_mask;

  uint16_t  serviceport;
  uint32_t  no_of_nwside_br_ports;
  uint32_t* nwside_br_ports_p;
};

int32_t tsc_add_vmport_flows_to_classify_table(uint64_t vn_handle,
                                               char*    swname_p,
                                               uint8_t  crm_port_type,
                                               uint32_t inport_id,
                                               uint16_t vlan_id_in,
                                               uint16_t vlan_id_out,
                                               uint64_t dp_handle
                                              );

int32_t tsc_delete_vmport_flows_from_classify_table(uint8_t  crm_port_type,
                                                    uint32_t inport_id,
                                                    uint64_t dp_handle
                                                   );

int32_t
tsc_classify_table_miss_entry_pkt_rcvd(uint64_t  datapath_handle,
                                       struct of_msg* msg,
                                       struct ofl_packet_in* pkt_in,
                                       struct tsc_ofver_plugin_iface* of_params_p
                                       );
int32_t
tsc_outbound_ns_chain_table_1_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                  struct ofl_packet_in* pkt_in,
                                                  struct of_msg* msg,
                                                  uint32_t in_port_id,
                                                  uint64_t metadata);

int32_t tsc_inbound_ns_chain_table_2_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                         struct ofl_packet_in* pkt_in,
                                                         struct of_msg* msg,
                                                         uint32_t in_port_id,
                                                         uint64_t metadata);

int32_t tsc_add_t3_proactive_flow_for_vm(uint64_t crm_port_handle,
                                         uint64_t crm_vn_handle,
                                         uint64_t dp_handle);

int32_t tsc_unicast_table_3_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                struct ofl_packet_in* pkt_in,
                                                struct of_msg* msg,                     
                                                uint64_t metadata,
                                                uint32_t in_port,
                                                uint32_t tun_src_ip);

int32_t tsc_broadcast_outbound_table_4_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                           struct ofl_packet_in* pkt_in,
                                                           struct of_msg* msg,
                                                           uint64_t metadata,
                                                           uint32_t in_port_id);

int32_t tsc_broadcast_inbound_table_5_miss_entry_pkt_rcvd(uint64_t dp_handle,
                                                          struct ofl_packet_in* pkt_in,
                                                          struct of_msg* msg,
                                                          uint64_t metadata,
                                                          uint32_t in_port_id,
                                                          uint32_t tun_src_ip);

uint32_t nsc_compute_hash_repository_table_1_2(uint32_t saddr,uint32_t daddr,
                                               uint32_t sport,uint32_t dport,
                                               uint32_t protocol,
                                               uint32_t ethernet_type,
                                               uint32_t port_id,
                                               uint64_t dp_handle,
                                               uint16_t vlan_id,
                                               uint32_t hashmask
                                             );

uint32_t nsc_compute_hash_repository_table_3(uint16_t serviceport,
                                             uint32_t dst_mac_high,
                                             uint32_t dst_mac_low,
                                             uint64_t dp_handle,
                                             uint32_t hashmask);


int32_t  tsc_get_nschain_repository_entry(char*    switch_name_p,
                                          uint64_t dp_handle,      
                                          uint8_t  nw_type,
                                          uint32_t nid,
                                          uint32_t in_port_id,
                                          uint64_t metadata,
                                          struct   ofl_packet_in* pkt_in,
                                          uint8_t  pkt_origin,
                                          uint8_t  pkt_dir,
                                          struct   nsc_selector_node* selector_p,
                                          uint32_t hashmask, 
                                          struct   nschain_repository_entry** nschain_repository_node_p_p);

int32_t  tsc_get_ucastpkt_outport_repository_entry(uint64_t dp_handle,
                                                   char*    local_switch_name_p,
                                                   uint8_t  nw_type,
                                                   uint32_t nid,
                                                   uint16_t serviceport,
                                                   uint64_t metadata,
                                                   struct   ofl_packet_in* pkt_in,
                                                   struct   ucastpkt_outport_repository_entry** ucastpkt_outport_repository_node_p_p);

int32_t tsc_get_vmports_repository_entry(char*    local_switch_name_p,
                                         uint8_t  nw_type,
                                         uint32_t nid,
                                         uint64_t metadata,
                                         struct   vmports_repository_entry** vmports_repository_node_p_p);

int32_t tsc_get_nwports_brdcast_repository_entry(char* local_switch_name_p,
                                                 uint8_t  nw_type,
                                                 uint32_t nid,
                                                 uint16_t serviceport,
                                                 uint64_t metadata,
                                                 struct nwports_brdcast_repository_entry** nwports_brdcast_repository_node_p_p);

int32_t tsc_flow_entry_removal_msg_recvd_nschain_tables(uint64_t dp_handle,uint64_t metadata,
                                                        uint64_t cookie,uint8_t reason,uint8_t table_id);

int32_t tsc_flow_entry_removal_msg_recvd_table_3(uint64_t dp_handle,uint64_t metadata,
                                                 uint64_t eth_dst,uint8_t reason);

int32_t tsc_delete_nschain_repository_entry(struct nschain_repository_entry* nschain_repository_node_p,uint8_t nw_type,
                                                     uint32_t nid,uint8_t table_id);

int32_t tsc_delete_nwport_flows_from_classify_table(uint8_t  crm_port_type,
                                                    uint32_t port_id,
                                                    uint64_t dp_handle);

int32_t tsc_delete_nsc_tables_1_2_flows_for_service_vm_deleted(uint64_t saferef_vn,uint64_t crm_port_handle);

int32_t tsc_delete_table_3_flow_entries(uint8_t crm_port_type,uint64_t crm_port_handle,uint64_t dp_handle);
int32_t tsc_delete_table_4_flow_entries(uint8_t crm_port_type,uint64_t crm_port_handle,uint64_t dp_handle);
int32_t tsc_delete_table_5_flow_entries(uint8_t crm_port_type,uint64_t crm_port_handle,uint64_t dp_handle);

int32_t tsc_send_all_flows_to_all_tsas(struct nschain_repository_entry* nschain_repository_entry_p);
int32_t tsc_send_all_flows_to_all_tsas_zone_left(struct nschain_repository_entry* nschain_repository_entry_p);
int32_t tsc_send_all_flows_to_all_tsas_zone_right(struct nschain_repository_entry* nschain_repository_entry_p);

int32_t tsc_get_zone(uint32_t vm_port_ip,uint8_t nw_type,uint32_t nid,char* zone,uint32_t* zone_direction_p);
void    tsc_update_zone_with_direction(uint32_t vm_port_ip,uint8_t nw_type,uint32_t nid,uint32_t zone_direction);
void tsc_reset_zone_direction_upon_modification(char* zone);
#endif /*__TSC_CONTROLLER__*/
