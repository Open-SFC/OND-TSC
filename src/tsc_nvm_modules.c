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

/* File: tsc_nvm_module.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains registration by the network virtualization modules with tsc infrastructure.
 */

#include "controller.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_controller.h"
#include "tsc_nvm_modules.h"
#include "tsc_vxn_views.h"
/******************************************************************************/
struct tsc_nvm_module_interface tsc_nvm_vxlan_module_interface =
{
  tsc_nvm_vxlan_init,
  tsc_nvm_vxlan_create_vmport_views,
  tsc_nvm_vxlan_delete_vmport_views,
  tsc_nvm_vxlan_create_nwport_views,
  tsc_nvm_vxlan_delete_nwport_views,
  tsc_vxn_get_unicast_nwport_by_swname_sp,
  tsc_vxn_get_remote_ips_by_swname_nid_sp,
  tsc_vxn_get_broadcast_nwports_by_swname_nid_sp,
  tsc_vxn_get_vmport_by_dmac_nid,
  tsc_vxn_get_vmports_by_swname_nid,
  tsc_vxn_add_vn_to_nid_view,
  tsc_vxn_delete_vn_from_nid_view,
  tsc_vxn_get_vnhandle_by_nid,
  tsc_nvm_vxlan_construct_metadata
};

int32_t tsc_nvm_module_init()
{
  int32_t retval = OF_SUCCESS;
  struct  tsc_nvm_module_info nvm_module_info;

  strcpy(nvm_module_info.nvm_module_name,"VXLAN");
  nvm_module_info.nw_type = VXLAN_TYPE;

  retval = tsc_register_nvm_module(&nvm_module_info,&tsc_nvm_vxlan_module_interface);
  if(retval == OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"VXLAN NVM module registration successful.");
  }
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"VXLAN NVM module registration failed");
  }
  return retval;
}
/***************************************************************************************/
