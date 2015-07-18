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

/* File: tsc_nsc_core.h
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This header file contains service chaining core module definitions.
 */

#include "controller.h"
#include "crmapi.h"
#include "crm_vn_api.h"

#define tscaddr_t unsigned long

#define OF_CNTLR_MACHDR_DSTMAC_OFFSET 0
#define OF_CNTLR_MACHDR_SRCMAC_OFFSET 6
#define OF_CNTLR_ETHERNET_TYPE_OFFSET 12

#define NSC_ERROR_DUPLICATE_CN_BIND_NODE -2
#define NSC_CONNECTION_BIND_ENTRY_ALREADY_EXISTS -3
#define NSC_CONNECTION_BIND_ENTRY_NOT_PRESENT -4
#define NSC_ERROR_INVALID_CNBIND_HANDLE -5 
#define NSC_FAILED_TO_ADD_SELECTOR      -6
#define NSC_INVALID_NSCHAINSET_HANDLE   -7
#define NSC_INVALID_NSCHAIN_HANDLE      -8
#define NSC_NSRM_CONFIG_LOOKUP_NOT_REQUIRED_OR_FAILED - 9
#define NSC_INCOMPLETE_NSCHAIN_CONFIGURATION -10
#define NSC_NO_SELECTION_RULE_FOUND    -11 
#define NSC_NO_BYPASS_RULE_FOUND       -12
#define NSC_CHAINSET_NOT_CONFIGURED    -13
#define NSC_NO_NW_SERVICES_FOUND       -14 
#define NSC_INVALID_BYPASS_HANDLE      -15
#define NSC_INVALID_NSCHAIN_NAME       -16 
#define NSC_FAILED_TO_ADD_REPOSITORY_ENTRY -17

#define NSC_CONNECTION_BIND_ENTRY_CREATED 2
#define NSC_CONNECTION_BIND_ENTRY_FOUND   3
#define NSC_NWSERVICES_CONFIGURED         5
#define NSC_NWSERVICES_NOT_CONFIGURED     6
#define NSC_SERVICE_CHAINING_NONE_CONFIGURED_FOR_VN 7
#define NSC_ADDED_REPOSITORY_ENTRY        8

#define SERVICE_CHAINING_NONE 0
#define SERVICE_CHAINING_L2   1
#define SERVICE_CHAINING_L3   2 

//#define NSC_CONNECTION_BIND_TABLE_NO_OF_BUCKETS 256
#define NSC_SELECTOR_NODE_OFFSET offsetof(struct nsc_selector_node,nsc_selector_tbl_link)

#define NSC_MAX_CNBIND_TBL_ENTRIES 512
#define NSC_MAX_CNBIND_TBL_STATIC_ENTRIES 128 

#define NSC_MAX_REPOSITORY_TBL_1_ENTRIES 1024   
#define NSC_MAX_REPOSITORY_TBL_1_STATIC_ENTRIES 512

#define NSC_MAX_REPOSITORY_TBL_2_ENTRIES 1024
#define NSC_MAX_REPOSITORY_TBL_2_STATIC_ENTRIES 512

#define NSC_MAX_REPOSITORY_TBL_3_ENTRIES 1024
#define NSC_MAX_REPOSITORY_TBL_3_STATIC_ENTRIES 512

/* For selectors preparation */
#define PACKET_NORMAL  1
#define PACKET_REVERSE 2

#define SELECTOR_PRIMARY   1
#define SELECTOR_SECONDARY 2

/* FW Policy Actions. */ 
#define NSC_FW_DENIED  0
#define NSC_FW_ALLOWED 1

/* A hash table of selectors is maintained */
struct nsc_selector_node
{
  struct    mchash_table_link nsc_selector_tbl_link;
  
  /* Key fields defining the connection: Mac Addresses, Ethernet Type, Five TUPLE from the packet. */

  uint32_t  src_mac_high; /* Total only 6 bytes */
  uint32_t  src_mac_low;
  uint32_t  dst_mac_high; /* Total only 6 bytes */
  uint32_t  dst_mac_low;

  uint8_t   src_mac[6];
  uint8_t   dst_mac[6];

  uint16_t  ethernet_type;

  uint8_t   protocol;
  uint16_t  src_port;
  uint16_t  dst_port;
  uint8_t   icmp_type;
  uint8_t   icmp_code;
  uint32_t  src_ip;
  uint32_t  dst_ip;

  uint32_t  zone;
  uint8_t   selector_type; /* PRIMARY or SECONDARY */
  uint32_t  hashkey;

  /* This selector */
  uint32_t  index;
  uint32_t  magic;
  uint64_t  safe_reference;

  /* Other selector */
  struct nsc_selector_node* other_selector_p;
  /* cnbind_node */
  struct l2_connection_to_nsinfo_bind_node* cnbind_node_p;
  uint16_t vlan_id_pkt; 
  uint8_t  nsc_entry_present; 
};

struct nw_services_to_apply
{
  uint64_t nschain_object_handle;
  uint64_t nwservice_instance_handle;
  uint64_t nwservice_object_handle;
};

struct  cnbind_nwservice_info
{
  uint8_t  config_lookup_failure;
 
  uint8_t  fw_action; 
  /* This may not be required. */
  struct   mchash_table_link  nsc_cnbind_tbl_link; 

  uint64_t nschainset_object_handle;  /* Copied from the associated L2 Service map record  */
  uint64_t selection_rule_handle; 

  uint64_t nschain_object_handle;
  uint64_t bypass_rule_handle;
                       
  /* Array of safe references to nw_services and the associated  nwservice_objects in the sequence required for packets from the connection receiving side */
  /* Above fields are populated using the required NSRM database lookup  */

  struct   nw_services_to_apply *nw_services_p;
  //struct   nw_services_to_apply *nw_services_reverse_p; /* Locally copied in the reverse order */
  uint8_t  no_of_nwservice_instances;

  //uint32_t no_of_flow_entries_deduced;    /* From this connection bind entry */    
       
  /* We need to include safe references to repository entries as bypass rules may be dynamically updated to include more network service objects to bypass. */
  of_list_t  sfr_repository_entry;
};

/* "no_of_flow_entries_deduced" field  is incremented each time a flow entry is deduced from the bind entry */
/* It is decremented, if a deduced flow entry is deleted due to expiry of hard/idle timeout of the entry. */
/* If it is decremented to 0, the bind entry is deleted */

/* Mempool is allocated for this structure */
struct l2_connection_to_nsinfo_bind_node
{
  struct   nsc_selector_node      selector_1; /* need not be primary   */
  struct   nsc_selector_node      selector_2; /* need not be secondary */
  struct   cnbind_nwservice_info  nwservice_info;

  uint8_t  local_in_mac[6];
  uint8_t  local_out_mac[6];

  uint32_t no_of_flow_entries_deduced; /* Repository entries of service chaining Table_1 and Table_2 */
  uint8_t  skip_this_entry;            /* When a service VM is deleted, this flag is set to TRUE  */
  uint8_t  stale_entry;                /* set to TRUE for deletion of the cnbind node */
  uint8_t  cnbind_sel_refcnt;          /* To delete cnbind node.To be intialized to 0  */
  void*    nsc_cn_bind_mempool_g;      /* To release cnbind node */
  cntlr_lock_t cnbind_node_lock;       /* lock before setting stale_entry flag for deletion */
  
  uint32_t nsc_config_status;
 
  uint8_t  heap_b;
};

struct vn_service_chaining_info
{
  uint8_t  service_chaining_type;    /* NONE/L2/L3 (default) */

  /* Initially NONE and is updated by the notification callback when a l2_servicemap_record is added/modified/deleted for the associated vn.
     When this field is NONE, no further lookup is done and it is assumed that service chaining is not enabled for this virtual network.
     But when this field switches its state from L2 to NONE and vice versa ,we need to delete the following before adding new ones.
     Connection bind entries,Repository entries and the Flows in switches.
     The configuration change of L2 to NONE is allowed only if all the related databases are deleted.  
  */
 
  uint64_t srf_service_map_record;  /* At present L2 service map record. To find the associated  nschainset_object. */
  uint64_t srf_nschainset_object;   /* To find the associated nschain object using packet fields. */

  uint8_t  vn_nsrmdb_lookup_state;  /* A magic number at VN level to track if any nsrm global database is changed. */
  void*    nsc_cn_bind_mempool_g;
  struct   mchash_table* l2_connection_to_nsinfo_bind_table_p;

  void*    nsc_repository_table_1_mempool_g;
  void*    nsc_repository_table_2_mempool_g;
  void*    nsc_repository_table_3_mempool_g;

  struct   mchash_table* nsc_repository_table_1_p;
  struct   mchash_table* nsc_repository_table_2_p;
  struct   mchash_table* nsc_repository_table_3_p;
};
/***********************************************************************************************************************************************
Function: nsc_lookup_l2_nschaining_info
Input Parameters: pkt_in,service_chaining_type
Output Parameters: cn_bind_node_p_p,selector_type_p
Description: If already not already present, creates and returns it.
***********************************************************************************************************************************************/
int32_t nsc_lookup_l2_nschaining_info(struct    ofl_packet_in* pkt_in,
                                      uint8_t   nw_type,
                                      uint32_t  nid,
                                      struct l2_connection_to_nsinfo_bind_node* cn_bind_entry_p_p);
/***********************************************************************************************************************************************
  Function:  nsrm_get_matching_nschain_object
  
  Input Parameters:
  connection_node_p: The caller instantiates one variable and fills the match fields from the received packet.
                     Match fields  to be extracted from the packet . 5 tuple , mac addresses and eth_type.
                     We need to use all these three types of fields to find the matching selection rule for both l2 and l3 service chaining.

  nschainset_object_handle: Safe reference to nschainset_object obtained from VN structure.

  Output parameters:
  matched_selection_rule_handle_p: Safe reference to the matched selection rule.
  nschain_object_handle_p:         Safe reference to the nchain_object contained in the matching selection rule.
*********************************************************************************************************************************************/
int32_t nsrm_get_matching_nschain_object(struct    nsc_selector_node* selector_p,
                                         uint32_t  nid,
                                         uint64_t* selection_rule_handle_p,
                                         uint64_t* nschain_object_handle_p);
/********************************************************************************************************************************************
  Function: nsrm_get_matching_bypass_rule

  Input Parameters:
  connection_node_p: The node that was  passed to the above API  to find the matching bypass rule is passed again to this API.
                     Match fields  to be extracted from the packet . 5 tuple , mac addresses and eth_type.
                     We need to use all the three types of fields to find the matching bypass rule for both l2 and l3 service chaining.
  
  nschain_object_handle:Safe reference to nschain_object obtained from the above API.

  output parameters:
  bypass_rule_handle_p: Safe reference to the matched bypass rule.
*******************************************************************************************************************************************/
int32_t  nsrm_get_matching_bypass_rule(struct    nsc_selector_node* selector_p,
                                       uint64_t  nschain_object_handle,
                                       uint64_t* bypass_rule_handle_p);
/******************************************************************************************************************************************
 Function: nsrm_get_nwservice_instances

 Input parameters:
 nschain_object_handle: SRF to nschain_object.
 bypass_rule_handle:    SRF to bypass rule.

 output parameters:
 no_of_nwservice_objects_p:Number of nwservice_objects configured for the nschain_object.
 nswservice_instance_handle_p: Array of pointers to handles of nwservice_instances.
                               Caller needs to pass a memory pointer which can hold 
                               as many handles to nwservice_instances as the no_of_nwservice_objects.
******************************************************************************************************************************************/
int32_t  nsrm_get_nwservice_instances(uint64_t   nschain_object_handle, 
                                      uint64_t   bypass_rule_handle, 
                                      uint8_t*   no_of_nwservice_objects_p,
                                      struct nw_services_to_apply** ns_instance_p_p);
/****************************************************************************************************************************************/
uint32_t NSC_COMPUTE_HASH_CNBIND_ENTRY(uint32_t saddr,uint32_t daddr,
                                       uint32_t sport,uint32_t dport,
                                       uint32_t protocol,
                                       uint32_t ethernet_type,
                                       uint32_t hashmask
                                      );
int32_t nsc_add_selectors(struct mchash_table* table_p,void* nsc_cn_bind_mempool_g,struct l2_connection_to_nsinfo_bind_node* cn_bind_entry_p);
int32_t nsc_extract_packet_fields(struct ofl_packet_in* pkt_in,struct nsc_selector_node* selector_p,uint8_t direction,uint8_t nsc_copy_mac_addresses_b);

int32_t nsc_get_cnbind_entry(struct   ofl_packet_in* pkt_in,
                             struct   nsc_selector_node* pkt_selector_p,
                             uint8_t  nw_type,
                             uint32_t nid,
                             struct   l2_connection_to_nsinfo_bind_node** cn_bind_node_p_p,
                             uint8_t  *selector_type_p,
                             uint8_t  copy_original_mac_b
                            );

void tsc_mark_cn_bind_entry_as_stale(struct mchash_table* l2_connection_to_nsinfo_bind_table_p,void* nsc_cn_bind_mempool_g,uint32_t index,uint32_t magic);

int32_t nsc_register_nsrm_notifications(void);
/***************************************************************************************************************************************/
int32_t nsc_init();
int32_t nsc_uninit();
int32_t nsc_init_vn_resources(struct crm_virtual_network* vn_entry_p);
int32_t nsc_deinit_vn_resources(struct crm_virtual_network* vn_entry_p);

