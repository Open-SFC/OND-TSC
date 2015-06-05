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

/* File: tsc_nsc_iface.h
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This header file contains definitions of interface to service chaining core module.
 */

#define TSC_PKT_OUTBOUND 1
#define TSC_PKT_INBOUND  2

struct tsa_all_flows_members
{
  uint8_t   selector_type;
  uint8_t   table_no;
  uint8_t   pkt_origin;
  uint64_t  dp_handle;
  uint32_t  inport_id;
  uint16_t  match_vlan_id;
  uint16_t  next_vlan_id;
  uint16_t  vlan_id_pkt;
  uint32_t  out_port_no;
  uint8_t   copy_local_mac_addresses_b;
  uint8_t   copy_original_mac_addresses_b;
  uint8_t   nw_port_b;
  uint32_t  remote_ip;
};      

int32_t nsc_get_l2_nschaining_info(struct  nschain_repository_entry* nschain_repository_entry_p,
                                   struct  ofl_packet_in* pkt_in,
                                   uint8_t pkt_origin);

