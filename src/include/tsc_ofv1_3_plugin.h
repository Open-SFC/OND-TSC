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

/*File: tsc_ofv1_3_plugin.h 
 * Author: ns murthy <b37840@freescale.com>
 * Description: Header file for OF 1.3 specific functions
 *
 */

#include "of_utilities.h"

int32_t tsc_add_tables_to_domain(uint64_t domain_handle);

 
int32_t tsc_ofplugin_v1_3_send_flow_mod_classify_table_vmports(uint64_t dp_handle,
                                                               struct tsc_ofver_plugin_iface* tsc_params_p,
                                                               uint8_t flow_mod_operation);

int32_t tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports(uint64_t dp_handle,
                                                               struct tsc_ofver_plugin_iface* tsc_params_p,
                                                               uint8_t flow_mod_operation);

int32_t tsc_ofplugin_v1_3_del_flows_from_table_3(uint64_t dp_handle,uint8_t *port_macaddr);

int32_t tsc_send_pktout_1_3_msg(uint64_t datapath_handle,uint32_t in_port,
                                uint8_t* packet,uint16_t packet_length);

int32_t tsc_prepare_flwmod_1_3_msg_table_0(uint64_t dp_handle,
                                           uint16_t msg_len,
                                           uint32_t *input_port_id_p,
                                           uint8_t  pkt_type,
                                           uint16_t *vlan_id_p,
                                           uint8_t  vlan_mask_req,
                                           uint16_t *vlan_id_mask_p,
                                           uint64_t metadata,
                                           uint64_t metadata_mask,
                                           uint8_t  goto_table_id
                                          );

int32_t tsc_add_flwmod_1_3_msg_table_1(uint64_t dp_handle,struct nschain_repository_entry* out_bound_nschain_repository_entry_p);
int32_t tsc_add_flwmod_1_3_msg_table_2(uint64_t dp_handle,struct nschain_repository_entry* in_bound_nschain_repository_entry_p);
int32_t tsc_add_flwmod_1_3_msg_table_3(uint64_t dp_handle,struct ucastpkt_outport_repository_entry* ucastpkt_outport_repository_entry_p);
int32_t tsc_add_flwmod_1_3_msg_table_4(uint64_t dp_handle,struct vmports_repository_entry* vmports_repository_entry_p,
                                       struct nwports_brdcast_repository_entry* nwports_brdcast_repository_entry_p);
int32_t tsc_add_flwmod_1_3_msg_table_5(uint64_t dp_handle,struct vmports_repository_entry* vmports_repository_entry_p);

int32_t
tsc_ofplugin_v1_3_table_0_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                              uint64_t domain_handle,
                                              uint64_t datapath_handle,
                                              struct   of_msg *msg,
                                              struct   ofl_packet_in *pkt_in,
                                              void*    cbk_arg1,
                                              void*    cbk_arg2);


int32_t
tsc_ofplugin_v1_3_table_1_miss_entry_pkt_rcvd(int64_t   controller_handle,
                                              uint64_t  domain_handle,
                                              uint64_t  datapath_handle,
                                              struct    of_msg *msg,
                                              struct    ofl_packet_in *pkt_in,
                                              void*     cbk_arg1,
                                              void*     cbk_arg2);

int32_t
tsc_ofplugin_v1_3_nschain_table_1_flow_entry_removed_rcvd(int64_t   controller_handle,
                                                          uint64_t  domain_handle,
                                                          uint64_t  datapath_handle,
                                                          struct    of_msg *msg,
                                                          struct    ofl_flow_removed *flow,
                                                          void*     cbk_arg1,
                                                          void*     cbk_arg2);
int32_t
tsc_ofplugin_v1_3_table_2_miss_entry_pkt_rcvd(int64_t   controller_handle,
                                              uint64_t  domain_handle,
                                              uint64_t  datapath_handle,
                                              struct    of_msg *msg,
                                              struct    ofl_packet_in *pkt_in,
                                              void*     cbk_arg1,
                                              void*     cbk_arg2);


int32_t
tsc_ofplugin_v1_3_nschain_table_2_flow_entry_removed_rcvd(int64_t   controller_handle,
                                                          uint64_t  domain_handle,
                                                          uint64_t  datapath_handle,
                                                          struct    of_msg *msg,
                                                          struct    ofl_flow_removed *flow,
                                                          void*     cbk_arg1,
                                                          void*     cbk_arg2);

int32_t
tsc_ofplugin_v1_3_table_3_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                              uint64_t domain_handle,
                                              uint64_t datapath_handle,
                                              struct   of_msg *msg,
                                              struct   ofl_packet_in *pkt_in,
                                              void*    cbk_arg1,
                                              void*    cbk_arg2);

int32_t
tsc_ofplugin_v1_3_fc_table_flow_entry_removed_rcvd(int64_t   controller_handle,
                                                   uint64_t  domain_handle,
                                                   uint64_t  datapath_handle,
                                                   struct    of_msg *msg,
                                                   struct    ofl_flow_removed *flow,
                                                   void*     cbk_arg1,
                                                   void*     cbk_arg2);

int32_t
tsc_ofplugin_v1_3_table_4_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                              uint64_t domain_handle,
                                              uint64_t datapath_handle,
                                              struct   of_msg *msg,
                                              struct   ofl_packet_in *pkt_in,
                                              void*    cbk_arg1,
                                              void*    cbk_arg2);

int32_t
tsc_ofplugin_v1_3_table_5_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                              uint64_t domain_handle,
                                              uint64_t datapath_handle,
                                              struct   of_msg *msg,
                                              struct   ofl_packet_in *pkt_in,
                                              void*    cbk_arg1,
                                              void*    cbk_arg2);
int32_t
tsc_ofplugin_v1_3_table_6_miss_entry_pkt_rcvd(int64_t  controller_handle,
                                              uint64_t domain_handle,
                                              uint64_t datapath_handle,
                                              struct   of_msg *msg,
                                              struct   ofl_packet_in *pkt_in,
                                              void*    cbk_arg1,
                                              void*    cbk_arg2);

int32_t
tsc_ofplugin_v1_3_dp_ready_event(uint64_t  controller_handle,
                                 uint64_t  domain_handle,
                                 uint64_t  datapath_handle,
                                 void*     cbk_arg1,
                                 void*     cbk_arg2);


  int32_t
tsc_ofplugin_v1_3_dp_detach_event(uint64_t  controller_handle,
    uint64_t  domain_handle,
    uint64_t  datapath_handle,
    void      *cbk_arg1,
    void      *cbk_arg2);


int32_t tsc_ofplugin_v1_3_del_flows_from_table_4_5(uint64_t dp_handle,
                                                   struct tsc_ofver_plugin_iface* tsc_params_p,
                                                   uint8_t table_id);

int32_t tsc_ofplugin_v1_3_del_flows_from_table_1_2(uint64_t dp_handle,uint32_t *port_id_p,uint8_t table_id);

int32_t tsc_ofplugin_v1_3_del_table_1_2_cnbind_flow_entries(struct nschain_repository_entry* nschain_repository_node_p,
                                                            uint8_t table_id);

int32_t tsc_del_flowmod_1_3_all_flows_in_a_table(uint64_t dp_handle,uint8_t table_id);
