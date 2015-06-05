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

/* File: tsc_nvm_vxlan.h
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 */

#define SERVICE_PORT_OFFSET 32     /* 16 bits */ 
#define VNI_OFFSET           0     /* 32 bits */

#define TUN_ID_MASK 0x00000000FFFFFFFF

int32_t tsc_nvm_vxlan_init();

int32_t tsc_nvm_vxlan_create_vmport_views(uint64_t crm_port_handle,uint8_t crm_port_type,uint64_t vn_handle);
int32_t tsc_nvm_vxlan_delete_vmport_views(uint64_t crm_port_handle,uint8_t crm_port_type,uint64_t vn_handle);
int32_t tsc_nvm_vxlan_create_nwport_views(uint64_t crm_port_handle,uint8_t crm_port_type,char*  swname_p);
int32_t tsc_nvm_vxlan_delete_nwport_views(uint64_t crm_port_handle,uint8_t crm_port_type,char*  swname_p);

void    tsc_nvm_vxlan_construct_metadata(struct tsc_ofver_plugin_iface* tsc_params_p,uint64_t vn_handle,struct crm_port* crm_port_p);

int32_t tsc_vxn_add_nwport_flows(char* swname_p,struct crm_vxlan_nw* vxlan_nw_p,uint8_t nwport_type,uint32_t port_id,uint64_t dp_handle);
