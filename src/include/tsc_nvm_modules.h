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

/* File: tsc_nvm_modules.h
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains definitions for registration by network virualization modules.
*/

/* INCLUDE_START ************************************************************/
#include "tsc_nvm_vxlan.h"
#include "crm_port_api.h"  
/* INCLUDE_END **************************************************************/

#ifndef __TSC_NVM_MODULES__
#define __TSC_NVM_MODULES__

#define TSC_NVM_MODULE_NAME_MAX_LENGTH 8

struct tsc_nvm_module_info
{
  char     nvm_module_name[TSC_NVM_MODULE_NAME_MAX_LENGTH + 1];
  uint8_t  nw_type; /* This is used as an index into tsc_nvm_modules[] */
                    /* Allowed values at present are VXLAN_TYPE,NVGRE_TYPE */
                    /* Shall be between SUPPORTED_NW_TYPE_MIN and SUPPORTED_NW_TYPE_MAX */ 
};
/******************************************************************************
* * * * * * * *  NVM Module Registration Function Prototypes * * * * * * * * * 
******************************************************************************/
typedef int32_t (*tsc_nvm_module_init_p)();
typedef int32_t (*tsc_nvm_module_create_vmport_views_p)(uint64_t,uint8_t,uint64_t);
typedef int32_t (*tsc_nvm_module_delete_vmport_views_p)(uint64_t,uint8_t,uint64_t);
typedef int32_t (*tsc_nvm_module_create_nwport_views_p)(uint64_t,uint8_t,char*);
typedef int32_t (*tsc_nvm_module_delete_nwport_views_p)(uint64_t,uint8_t,char*);

typedef int32_t (*tsc_nvm_module_get_unicast_nwport_p)(char*,uint16_t,uint32_t*);
typedef int32_t (*tsc_nvm_module_get_remote_ips_p)(char*, uint32_t,uint32_t,uint32_t*,uint32_t*);
typedef int32_t (*tsc_nvm_module_get_broadcast_nwports_p)(char*, uint32_t,uint32_t,uint32_t*,uint32_t*,uint32_t);
typedef int32_t (*tsc_nvm_module_get_vmport_by_dmac_nid_p)(uint8_t*,uint32_t,uint64_t*); 
typedef int32_t (*tsc_nvm_module_get_vmports_by_swname_nid_p)(char*,uint32_t,uint32_t*,uint32_t*);

typedef int32_t (*tsc_nvm_module_add_vn_to_nid_view_p)(uint64_t,uint32_t);
typedef int32_t (*tsc_nvm_module_delete_vn_from_nid_view_p)(uint64_t,uint32_t);
typedef int32_t (*tsc_nvm_module_get_vnhandle_by_nid_p)(uint32_t,uint64_t *);

typedef void    (*tsc_nvm_module_construct_metadata_p)(struct tsc_ofver_plugin_iface*,uint64_t vn_handle,struct crm_port* crm_port_p);

struct tsc_nvm_module_interface
{
  tsc_nvm_module_init_p                   nvm_module_init;
  tsc_nvm_module_create_vmport_views_p    nvm_module_create_vmport_views;
  tsc_nvm_module_delete_vmport_views_p    nvm_module_delete_vmport_views;
  tsc_nvm_module_create_nwport_views_p    nvm_module_create_nwport_views;
  tsc_nvm_module_delete_nwport_views_p    nvm_module_delete_nwport_views;

  tsc_nvm_module_get_unicast_nwport_p           nvm_module_get_unicast_nwport;
  tsc_nvm_module_get_remote_ips_p               nvm_module_get_remote_ips;
  tsc_nvm_module_get_broadcast_nwports_p        nvm_module_get_broadcast_nwports;
  tsc_nvm_module_get_vmport_by_dmac_nid_p       nvm_module_get_vmport_by_dmac_nid;
  tsc_nvm_module_get_vmports_by_swname_nid_p    nvm_module_get_vmports_by_swname_nid;

  tsc_nvm_module_add_vn_to_nid_view_p       nvm_module_add_vn_to_nid_view;
  tsc_nvm_module_delete_vn_from_nid_view_p  nvm_module_delete_vn_from_nid_view;
  tsc_nvm_module_get_vnhandle_by_nid_p      nvm_module_get_vnhandle_by_nid;

  tsc_nvm_module_construct_metadata_p       nvm_module_construct_metadata;
};
/******************************************************************************
 * * * * * * * * * * * * * * * * Function Prototypes * * * * * * * * * * * * * 
*******************************************************************************/
int32_t tsc_register_nvm_module(struct tsc_nvm_module_info*       nvm_module_info_p,
                                struct tsc_nvm_module_interface*  nvm_module_interface_p);

int32_t tsc_nvm_module_init();
#endif 
