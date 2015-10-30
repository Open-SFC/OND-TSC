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

/* File: tsc_nsc_notify.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains implemenation of notification callbacks from nsrm database.
 */
#include "controller.h"
#include "dobhash.h"
#include "cntrlappcmn.h"
#include "dprm.h"
#include "crmapi.h"
#include "crm_vm_api.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_controller.h"
#include "nsrm.h"

extern  uint8_t   nsrmdb_current_state_g;
extern  uint32_t  vn_nsc_info_offset_g;

void nsc_l2_service_map_notifications_add_cbk(uint8_t   notification_type,
                                              uint64_t  nsrm_handle,
                                              union     nsrm_l2nw_service_map_notification_data notification_data,
                                              void      *callback_arg1,
                                              void      *callback_arg2)
{
  struct   vn_service_chaining_info* vn_nsc_info_in_p;
  struct   vn_service_chaining_info* vn_nsc_info_out_p;
  struct   crm_virtual_network*      vn_entry_p;
  int32_t  retval;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"received notification callback - L2 service map record is added");

  if(notification_data.add_del.vn_in_handle == notification_data.add_del.vn_out_handle)
  {    
    retval = crm_get_vn_byhandle(notification_data.add_del.vn_in_handle,&vn_entry_p); 
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "VN doesn't exist ");
      return;
    }
 
    vn_nsc_info_in_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

    vn_nsc_info_in_p->service_chaining_type  = SERVICE_CHAINING_L2;
    vn_nsc_info_in_p->vn_nsrmdb_lookup_state = 0;
    vn_nsc_info_in_p->srf_service_map_record = notification_data.add_del.l2nw_service_map_handle;  
    vn_nsc_info_in_p->srf_nschainset_object  = notification_data.add_del.nschainset_object_handle;
    vn_nsc_info_in_p->nw_direction           = NW_DIRECTION_IN; 
    vn_nsc_info_in_p->no_of_networks         = 1; 
    vn_nsc_info_in_p->vn_nsc_info_p          = vn_nsc_info_in_p;  
  }
  else
  {
    retval = crm_get_vn_byhandle(notification_data.add_del.vn_in_handle,&vn_entry_p);
    if(retval != CRM_SUCCESS)
    {
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "VN doesn't exist ");
        return;
    }
    vn_nsc_info_in_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

    vn_nsc_info_in_p->service_chaining_type  = SERVICE_CHAINING_L2;
    vn_nsc_info_in_p->vn_nsrmdb_lookup_state = 0;
    vn_nsc_info_in_p->srf_service_map_record = notification_data.add_del.l2nw_service_map_handle;
    vn_nsc_info_in_p->srf_nschainset_object  = notification_data.add_del.nschainset_object_handle;
    vn_nsc_info_in_p->nw_direction           = NW_DIRECTION_IN; 
    vn_nsc_info_in_p->no_of_networks         = 2; 
    vn_nsc_info_in_p->vn_nsc_info_p          = vn_nsc_info_in_p;  

    retval = crm_get_vn_byhandle(notification_data.add_del.vn_out_handle,&vn_entry_p);
    if(retval != CRM_SUCCESS)
    {
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "VN doesn't exist ");
        return;
    }
 
    vn_nsc_info_out_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));
    vn_nsc_info_out_p->no_of_networks         = 2; 
    vn_nsc_info_out_p->nw_direction           = NW_DIRECTION_OUT; 
    vn_nsc_info_out_p->vn_nsc_info_p          = vn_nsc_info_in_p;  
  }
}

void nsc_l2_service_map_notifications_del_cbk(uint8_t   notification_type,
                                              uint64_t  nsrm_handle,
                                              union     nsrm_l2nw_service_map_notification_data notification_data,
                                              void      *callback_arg1,
                                              void      *callback_arg2)
{
  struct  vn_service_chaining_info* vn_nsc_info_p;
  struct  crm_virtual_network*      vn_entry_p;
  int32_t retval;
 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"received notification callback - L2 service map record is deleted");

  retval = crm_get_vn_byhandle(notification_data.add_del.vn_in_handle,&vn_entry_p);
  if(retval != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "VN doesn't exist ");
    return;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

  vn_nsc_info_p->vn_nsc_info_p          = NULL;
  vn_nsc_info_p->service_chaining_type  = 0;
  vn_nsc_info_p->vn_nsrmdb_lookup_state = 0;
  vn_nsc_info_p->srf_service_map_record = 0;
  vn_nsc_info_p->srf_nschainset_object  = 0;

  retval = crm_get_vn_byhandle(notification_data.add_del.vn_out_handle,&vn_entry_p);
  if(retval != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "VN doesn't exist ");
    return;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));

  vn_nsc_info_p->vn_nsc_info_p          = NULL;
  vn_nsc_info_p->service_chaining_type  = 0;
  vn_nsc_info_p->vn_nsrmdb_lookup_state = 0;
  vn_nsc_info_p->srf_service_map_record = 0;
  vn_nsc_info_p->srf_nschainset_object  = 0;

}

/*GPSYS this function is modified with 2 ports support*/
void nsc_l2_nwservice_object_launched_cbk(uint8_t   notification_type,
                                          uint64_t  nwservice_object_saferef,
                                          union     nsrm_nwservice_object_notification_data notification_data,
                                          void      *callback_arg1,
                                          void      *callback_arg2)
{
  struct  crm_port* port_info_p = NULL;
  int32_t retval = OF_SUCCESS;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nsc_l2_nwservice_object_launched_cbk is called");
  
  if(notification_data.launch.no_of_ports == 1)
  {  
    CNTLR_RCU_READ_LOCK_TAKE(); 
    retval = crm_get_port_byhandle(notification_data.launch.port1_saferef,&port_info_p);
    port_info_p->vlan_id_in  = notification_data.launch.vlan_id_in;
    port_info_p->vlan_id_out = notification_data.launch.vlan_id_out;

    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "switch name = %s",port_info_p->switch_name);
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "vlan_id_in  = %d",notification_data.launch.vlan_id_in);
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "vlan_id_out = %d",notification_data.launch.vlan_id_out);
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "port_id     = %d",port_info_p->port_id);

    if(retval == CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Before adding VM_NS ports to classification table");
      tsc_add_vmport_flows_to_classify_table(port_info_p->saferef_vn,
                                             port_info_p->switch_name,
                                             port_info_p->crm_port_type,
                                             port_info_p->port_id,
                                             notification_data.launch.vlan_id_in,
                                             notification_data.launch.vlan_id_out,
                                             port_info_p->system_br_saferef);
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "After  adding VM_NS ports to classification table");    
    }
    else
    {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to get the VM_NS Port");
      printf("\r\n Failed to get VM_NS port of type = %d",port_info_p->crm_port_type);
    }
    CNTLR_RCU_READ_LOCK_RELEASE();
  }
  else if(notification_data.launch.no_of_ports == 2)
  {
    CNTLR_RCU_READ_LOCK_TAKE();
    retval = crm_get_port_byhandle(notification_data.launch.port1_saferef,&port_info_p);
    if(retval == CRM_SUCCESS)
    {
      if(port_info_p->crm_port_type == VMNS_IN_PORT)
      {
        port_info_p->vlan_id_in  = notification_data.launch.vlan_id_in;
        port_info_p->vlan_id_out = 0;
      }
      else
      {
        port_info_p->vlan_id_in  = 0;
        port_info_p->vlan_id_out = notification_data.launch.vlan_id_out;
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "switch name = %s",port_info_p->switch_name);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "vlan_id_in  = %d",notification_data.launch.vlan_id_in);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "vlan_id_out = %d",notification_data.launch.vlan_id_out);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "port_id     = %d",port_info_p->port_id);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "port_type   = %d",port_info_p->crm_port_type);

      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Before adding VM_NS_IN or VM_NS_OUT port to classification table");
      tsc_add_vmport_flows_to_classify_table(port_info_p->saferef_vn,
                                             port_info_p->switch_name,
                                             port_info_p->crm_port_type,
                                             port_info_p->port_id,
                                             port_info_p->vlan_id_in,
                                             port_info_p->vlan_id_out,
                                             port_info_p->system_br_saferef);
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "After  adding VM_NS_IN or VM_NS_OUT ports to classification table");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to get VM_NS port of type = %d",port_info_p->crm_port_type);
      printf("\r\n Failed to get VM_NS port of type = %d",port_info_p->crm_port_type);
    }
    
    retval = crm_get_port_byhandle(notification_data.launch.port2_saferef,&port_info_p);
    if(retval == CRM_SUCCESS)
    {
      if(port_info_p->crm_port_type == VMNS_IN_PORT)
      {
        port_info_p->vlan_id_in  = notification_data.launch.vlan_id_in;
        port_info_p->vlan_id_out = 0;
      }
      else
      {
        port_info_p->vlan_id_in   = 0;
        port_info_p->vlan_id_out  = notification_data.launch.vlan_id_out;
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "switch name = %s",port_info_p->switch_name);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "vlan_id_in  = %d",notification_data.launch.vlan_id_in);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "vlan_id_out = %d",notification_data.launch.vlan_id_out);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "port_id     = %d",port_info_p->port_id);
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG, "port_type   = %d",port_info_p->crm_port_type);

      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Before adding VM_NS_IN or VM_NS_OUT port to classification table");
      tsc_add_vmport_flows_to_classify_table(port_info_p->saferef_vn,
                                             port_info_p->switch_name,
                                             port_info_p->crm_port_type,
                                             port_info_p->port_id,
                                             port_info_p->vlan_id_in,
                                             port_info_p->vlan_id_out,
                                             port_info_p->system_br_saferef);
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "After  adding VM_NS_IN or VM_NS_OUT ports to classification table");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to get the VM_NS port of type = %d",port_info_p->crm_port_type);
      printf("\r\n Failed to get the VM_NS port of type = %d",port_info_p->crm_port_type);
    }

    CNTLR_RCU_READ_LOCK_RELEASE();
  }
  /* IMPORTANT TBD:  The problem with this approach is that,by this time datapath might have discconected from the OF controller temporarily. */
  /* We may need to queue such data and add the flows whenever the datapath connects back. */
  /* A network service instance may be launched at any time. */
  /* Alternately the administrator shall try to re-launch the service on the alloted switch if the launch is not successful.*/         
}

void nsc_l2_nwservice_object_de_launched_cbk(uint8_t   notification_type,
                                             uint64_t  nwservice_object_saferef,
                                             union     nsrm_nwservice_object_notification_data notification_data,
                                             void      *callback_arg1,
                                             void      *callback_arg2)
{
  struct  crm_port* port_info_p = NULL;
  int32_t retval = OF_SUCCESS;

  //printf("\r\nService De-luanch callback is called");
  return OF_SUCCESS;  /* Call the appropriate functions in port deletion path. */
}

void  nsrm_nwservice_object_notifications_cbk(uint8_t   notification_type,
                                              uint64_t  saferef,
                                              union     nsrm_nwservice_object_notification_data notification_data,
                                              void      *callback_arg1,
                                              void      *callback_arg2)
{
  if((notification_type != NSRM_NWSERVICE_OBJECT_LAUNCHED) && (notification_type == NSRM_NWSERVICE_OBJECT_DE_LAUNCHED))
  {  
    nsrmdb_current_state_g =  (nsrmdb_current_state_g + 1);
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrmdb_current_state_g = %d",nsrmdb_current_state_g);
  }
}


void  nsrm_nschain_object_notifications_cbk(uint8_t   notification_type,
                                            uint64_t  saferef,
                                            union     nsrm_nschain_object_notification_data notification_data,
                                            void      *callback_arg1,
                                            void      *callback_arg2)
{
  nsrmdb_current_state_g =  (nsrmdb_current_state_g + 1);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrmdb_current_state_g = %d",nsrmdb_current_state_g);
}

void  nsrm_nschainset_object_notifications_cbk(uint8_t   notification_type,
                                               uint64_t  saferef,
                                               union     nsrm_nschainset_object_notification_data notification_data,
                                               void      *callback_arg1,
                                               void      *callback_arg2)
{
  nsrmdb_current_state_g =  (nsrmdb_current_state_g + 1);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrmdb_current_state_g = %d",nsrmdb_current_state_g);
}

void  nsrm_selection_rule_notifications_cbk(uint8_t   notification_type,
                                            uint64_t  saferef,
                                            union     nsrm_nschain_selection_rule_notification_data notification_data,
                                            void      *callback_arg1,
                                            void      *callback_arg2)
{
  nsrmdb_current_state_g =  (nsrmdb_current_state_g + 1);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrmdb_current_state_g = %d",nsrmdb_current_state_g);
}

void  nsrm_bypass_rule_notifications_cbk(uint8_t   notification_type,
                                         uint64_t  saferef,
                                         union     nsrm_nwservice_bypass_rule_notification_data notification_data,
                                         void      *callback_arg1,
                                         void      *callback_arg2)
{
  nsrmdb_current_state_g =  (nsrmdb_current_state_g + 1);
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrmdb_current_state_g = %d",nsrmdb_current_state_g);
}

void  nsrm_register_zone_change_notification_cbk(uint8_t   notification_type,
                                         uint64_t  saferef,
                                         union     nsrm_zone_notification_data notification_data,
                                         void      *callback_arg1,
                                         void      *callback_arg2)
{
  OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"nsrm_register_zone_change_notification_cbk called");
  printf("\r\n nsrm_register_zone_change_notification_cbk: Received notification for a zone is deleted or modified");
  tsc_reset_zone_direction_upon_modification(notification_data.add_del.zone_name_p);
}    
#if 0

   TBD
   Provide facility for Applications like Openstack to receive notifications from Traffic steering module.
   They register with their callback funcions.

   TSA notifies whenever the total traffic hits either the configured upper or lower thresholds.It may either bring up
   new service VMs or shutdown some service VMs.

   A. Total number of connections.
   B. Total number of bytes of trafffic. (This needs support from Switches).

#endif

int32_t nsc_register_nsrm_notifications(void)
{
  int32_t   retval;
  uint32_t  callback_arg1 = 1;
  uint32_t  callback_arg2 = 1;

  do
  {
    callback_arg1 = 1;
    callback_arg2 = 1;

    retval =  nsrm_register_l2nw_service_map_notifications(NSRM_L2NW_SERVICE_MAP_ADDED,
                                                           nsc_l2_service_map_notifications_add_cbk,
                                                           (void *)&callback_arg1,
                                                           (void *)&callback_arg2);
    if(retval == NSRM_FAILURE)
      break;


    retval =  nsrm_register_l2nw_service_map_notifications(NSRM_L2NW_SERVICE_MAP_DELETED,
                                                           nsc_l2_service_map_notifications_del_cbk,
                                                           (void *)&callback_arg1,
                                                           (void *)&callback_arg2);
    if(retval == NSRM_FAILURE)
      break;
 
    retval = nsrm_register_nwservice_object_notifications(NSRM_NWSERVICE_OBJECT_LAUNCHED,
                                                          nsc_l2_nwservice_object_launched_cbk,
                                                          (void*)&callback_arg1,
                                                          (void*)&callback_arg2);

    if(retval == NSRM_FAILURE)
      break; 


    retval = nsrm_register_nwservice_object_notifications(NSRM_NWSERVICE_OBJECT_DE_LAUNCHED,
                                                         nsc_l2_nwservice_object_de_launched_cbk,
                                                         (void*)&callback_arg1,
                                                         (void*)&callback_arg2);

    if(retval == NSRM_FAILURE)
      break;

    retval = nsrm_register_nwservice_object_notifications(NSRM_NWSERVICE_OBJECT_ALL,
                                                          nsrm_nwservice_object_notifications_cbk,
                                                          (void *)&callback_arg1,
                                                          (void *)&callback_arg2);
 
    if(retval != NSRM_SUCCESS)
       break;

    retval =  nsrm_register_nschain_object_notifications(NSRM_NSCHAIN_OBJECT_ALL,
                                                         nsrm_nschain_object_notifications_cbk,
                                                         (void *)&callback_arg1,
                                                         (void *)&callback_arg2);

    if(retval != NSRM_SUCCESS)
      break;


    retval =  nsrm_register_nschainset_object_notifications(NSRM_NSCHAIN_SET_OBJECT_ALL,
                                                            nsrm_nschainset_object_notifications_cbk,
                                                            (void *)&callback_arg1,
                                                            (void *)&callback_arg2);
 
    if(retval != NSRM_SUCCESS)
      break;
 
  
    retval =  nsrm_register_nschain_selection_rule_notifications(NSRM_NSCHAIN_SELECTION_RULE_ALL,
                                                                nsrm_selection_rule_notifications_cbk,
                                                                (void *)&callback_arg1,
                                                                (void *)&callback_arg2);

    if(retval != NSRM_SUCCESS)
      break;
 
  
    retval =  nsrm_register_nwservice_bypass_rule_notifications(NSRM_NWSERVICE_BYPASS_RULE_ALL,
                                                               nsrm_bypass_rule_notifications_cbk,
                                                               (void *)&callback_arg1,
                                                               (void *)&callback_arg2);

    if(retval != NSRM_SUCCESS)
      break;

#if 0
    retval = nsrm_register_zone_notifications(NSRM_ZONE_MODIFIED_IN_NSCHAINSET_OBJECT,
                                              nsrm_register_zone_change_notification_cbk, 
                                              (void *)&callback_arg1,
                                              (void *)&callback_arg2);

    if(retval != NSRM_SUCCESS)
      break; 

    retval = nsrm_register_zone_notifications(NSRM_ZONE_DELETED_FROM_NSCHAINSET_OBJECT ,
                                              nsrm_register_zone_change_notification_cbk,
                                              (void *)&callback_arg1,
                                              (void *)&callback_arg2);

    if(retval != NSRM_SUCCESS)
      break;
#endif
    retval = nsrm_register_zone_notifications(NSRM_ZONE_ALL,
                                              nsrm_register_zone_change_notification_cbk, 
                                              (void *)&callback_arg1,
                                              (void *)&callback_arg2);

    if(retval != NSRM_SUCCESS)
      break; 


  }while(0);
  return retval;
}
