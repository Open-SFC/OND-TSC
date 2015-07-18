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
 **/

/*File: tsc_init.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains initialization of the Flow Control modules.
 */

/* INCLUDE_START ************************************************************/
#include "controller.h"
#include "of_utilities.h"
#include "of_msgapi.h"
#include "cntrl_queue.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "dprmldef.h"
#include "ttprmgdef.h"
#include "crmapi.h"
#include "crmldef.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "nsrm.h"
#include "nsrmldef.h"
#include "port_status_mgr.h"
#include "tsc_controller.h"
#include "tsc_ofplugin_v1_3_ldef.h"
#include "tsc_ofv1_3_plugin.h"
#include "tsc_nvm_modules.h"
#include "tsc_nvm_vxlan.h"
/* INCLUDE_END **************************************************************/

/* dc_vs_ttp: Following macros for Table type pattern. */

#define CNTLR_TSC_TTP_TABLES_CNT 6

/* Table id0: Table 0 */
#define CNTLR_DC_VS_TTP_ID0_NAME "CLASSIFICATION-TABLE-0"
#define CNTLR_DC_VS_TTP_ID0_COLUMNS 0
#define CNTLR_DC_VS_TTP_ID0_ROWS    0

/* Table id1: Table 1 */
#define CNTLR_DC_VS_TTP_ID1_NAME "NS-TABLE-1"
#define CNTLR_DC_VS_TTP_ID1_COLUMNS 0
#define CNTLR_DC_VS_TTP_ID1_ROWS    0

/* Table id2: Table 2 */
#define CNTLR_DC_VS_TTP_ID2_NAME "NS-TABLE-2"
#define CNTLR_DC_VS_TTP_ID2_COLUMNS 0
#define CNTLR_DC_VS_TTP_ID2_ROWS    0

/* Table id3: Table 3 */
#define CNTLR_DC_VS_TTP_ID3_NAME "UNICAST-TABLE-3"
#define CNTLR_DC_VS_TTP_ID3_COLUMNS 0
#define CNTLR_DC_VS_TTP_ID3_ROWS    0

/* Table id4: Table 4 */
#define CNTLR_DC_VS_TTP_ID4_NAME "BRDOUT-TABLE-4"
#define CNTLR_DC_VS_TTP_ID4_COLUMNS 0
#define CNTLR_DC_VS_TTP_ID4_ROWS    0

/* Table id5: Table 5 */
#define CNTLR_DC_VS_TTP_ID5_NAME "BRDIN-TABLE-5"
#define CNTLR_DC_VS_TTP_ID5_COLUMNS 0
#define CNTLR_DC_VS_TTP_ID5_ROWS    0

#define  DC_VS_TTP_CLASSIFICATION_TABLE_ID  0
#define  DC_VS_TTP_NS1_TABLE_ID             1
#define  DC_VS_TTP_NS2_TABLE_ID             2
#define  DC_VS_TTP_UNICAST_TABLE_ID         3
#define  DC_VS_TTP_BRDOUT_TABLE_ID          4
#define  DC_VS_TTP_BRDIN_TABLE_ID           5

extern struct tsc_bintree_node* tsc_bintree_root;

extern struct tsc_nvm_module_interface tsc_nvm_modules[SUPPORTED_NW_TYPE_MAX + 1];

char vm_ipaddr[18] = {};
char vm_zone[10]   = {};

struct   dprm_distributed_forwarding_domain_info tsc_domain_info;
uint64_t tsc_domain_handle;

int32_t tsc_test_app_init(uint64_t tsc_domain_handle);


void tsc_domain_event_callback(uint32_t notification_type,
                               uint64_t tsc_domain_handle,
                               struct   dprm_domain_notification_data notification_data,
                               void     *callback_arg1,
                               void     *callback_arg2);

void crm_vn_notifications_cbk(uint32_t notification_type,
                              uint64_t vn_handle,
                              struct   crm_vn_notification_data notification_data,
                              void     *callback_arg1,
                              void     *callback_arg2);

void crm_port_notifications_cbk(uint32_t notification_type,
                                uint64_t vn_handle,
                                struct   crm_port_notification_data notification_data,
                                void     *callback_arg1,
                                void     *callback_arg2);

int32_t tsc_handle_crm_port_addition(uint32_t notification_type,
                                     uint64_t vn_handle,
                                     struct   crm_port_notification_data notification_data,
                                     void     *callback_arg1,
                                     void     *callback_arg2);

int32_t tsc_handle_crm_port_deletion(uint32_t notification_type,
                                     uint64_t vn_handle,
                                     struct   crm_port_notification_data notification_data,
                                     void     *callback_arg1,
                                     void     *callback_arg2);

int32_t tsc_get_zone_info(void);
int32_t tsc_init_of_plugins(uint64_t tsc_domain_handle);
int32_t tsc_uninit_of_plugins(uint64_t tsc_domain_handle);
int32_t tsc_app_tbls_reg(void);
/********************************************************************************************************************
Function Name: tsc_module_init
Input  Parameters: 
	DPRM_DOMAIN_ALL_NOTIFICATIONS: Receive all the domain notifications.
        tsc_domain_event_callback:     Callback function to receive domain notifications.
Output Parameters: NONE
Description:
        1. This function initializes the Traffic Steering Controller (TSC) module. 
	2. Traffic Steering Controller module registers itself as the main TTP (DATA_CENTER_VIRTUAL_SWITCH_TTP)
           with TTP infrastructure of controller and registers 6 OF tables.
        3. It registers with DPRM module of Controller infrastructure to receive DOMAIN notifications.	
        4. A domain with name "TSC_DOMAIN" is created for the TSC Application. 
********************************************************************************************************************/
int32_t tsc_module_init()
{
  uint64_t ttprm_handle;
  int32_t  ret_val = OF_SUCCESS;

  /* ttprm registration */
  ret_val = ttprm_register("DATA_CENTER_VIRTUAL_SWITCH_TTP", &ttprm_handle);
  if(ret_val != CNTLR_TTP_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to register DCVS-TTP with ttprm.");
    return OF_FAILURE;
  }
  /* TSC  main ttp tables regisration with ttprm**/
  ret_val = tsc_app_tbls_reg();
  if(ret_val != CNTLR_TTP_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_SAMPLE_APP, OF_LOG_ERROR, "Failed to register TSC-TTP Tables with ttprm");
    return OF_FAILURE;
  }

/* Add TSC_DOMAIN under dc_vs_ttp to OF Controller infrastructure for Flow Control Application */

  strcpy(tsc_domain_info.name,"TSC_DOMAIN");
  strcpy(tsc_domain_info.expected_subject_name_for_authentication,"tsc_domain@abc.com");
  strcpy(tsc_domain_info.ttp_name,"DATA_CENTER_VIRTUAL_SWITCH_TTP");
  tsc_domain_info.number_of_tables = 6;
  
  ret_val = dprm_register_forwarding_domain_notifications(DPRM_DOMAIN_ALL_NOTIFICATIONS,
                                                          tsc_domain_event_callback,
                                                          &tsc_domain_info, NULL);
  if(ret_val != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Registering callback to forward domain TSC_DOMAIN failed....");
    return OF_FAILURE;
  }

 /* Add TSC_DOMAIN under dc_vs_ttp to OF Controller infrastructure for Flow Control Application */
 
  ret_val = dprm_create_distributed_forwarding_domain(&tsc_domain_info,&tsc_domain_handle);
  if(ret_val != DPRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to Add TSC_DOMAIN to the OF Controller infrastructure");
    return OF_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Added TSC_DOMAIN to the OF Controller infrastructure: domain handle=%llx ",tsc_domain_handle);
  
  tsc_get_zone_info();

  return OF_SUCCESS;
}
/**************************************************************************************************************************
Function Name : tsc_domain_event_callback
Input Parameters:
	notification_type: Domian notification type 
        tsc_domain_handle: Handle to the "TSC_DOMAIN" internal structure.
        notification_data: Data related to the notification received.
Output Parameters:None
Description:Upon receiving the notification, this function initializes all the required modules as listed in the function.
*************************************************************************************************************************/
void tsc_domain_event_callback(uint32_t notification_type,
                               uint64_t tsc_domain_handle,
                               struct   dprm_domain_notification_data notification_data,
                               void     *callback_arg1,
                               void     *callback_arg2)
{
  int32_t status = OF_SUCCESS;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"ENTRY");
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"notification_type :%d",notification_type);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,
             "Domain name = %s", notification_data.domain_name);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,
             "TTP name = %s", notification_data.ttp_name);
  if(strcmp(notification_data.ttp_name, "DATA_CENTER_VIRTUAL_SWITCH_TTP"))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"notification is NOT for data center ttp.");
    return;
  }

  switch(notification_type)
  {
    case DPRM_DOMAIN_ADDED:
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"FC DOMAIN ADD notification event ");
  
      status = tsc_init_of_plugins(tsc_domain_handle);
      if(status != OF_SUCCESS) 
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to initialize Open Flow plugins for Flow Control Application: status=%d",status);
        return;
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully initilized OF Plugins for Flow Control Application:");


      status = crm_init();
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"\r\n CRM module initialization failed: status=%d",status);
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"CRM module initialization is successful");

      status = psm_module_init();
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"\r\n PSM module initialization failed: status=%d",status);
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"PSM module initialization is successful");
 

      status = nsrm_init();
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"\r\n NSRM module initialization failed: status=%d",status);
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"NSRM module initialization is successful");

      status = tsc_nvm_module_init();
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"\r\n Failed to initialize Network Virtualization Modules: status=%d",status);
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully initilized Network Virtualization Modules");

      status = crm_register_vn_notifications(CRM_VN_ALL_NOTIFICATIONS,
                                             crm_vn_notifications_cbk,
                                             (void *)&callback_arg1,
                                             (void *)&callback_arg2);
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Failed to register with CRM module to receive vn notifications");
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully registered with CRM module to receive vn notifications");

      status = crm_register_port_notifications(CRM_PORT_ALL_NOTIFICATIONS,
                                               crm_port_notifications_cbk,
                                               (void *)&callback_arg1,
                                               (void *)&callback_arg2);
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"\r\n Failed to register with CRM module to receive port notifications");
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully registered with CRM module to receive port notifications");

      status = nsc_init();
      if(status != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"\r\n NSC module initialization failed: status=%d",status);
        return; 
      }
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"NSC module initialization is successful");
      
      //nsrm_test();
     
      break;
    }
  }
  return; 
} 
/*******************************************************************************************************
Function Name: tsc_init_of_plugins
Description:
	Registers to receive the OF Packet-in messages for tables 1-5 from all the connected datapaths. 
        Registers to receive DATAPATH READY event.
        At present, only OF Version 1.3 is supported.
*******************************************************************************************************/
int32_t tsc_init_of_plugins(uint64_t tsc_domain_handle)
{
  int32_t status;

  /* Registers to receive pkt-in messages for the required fc application tables */
  status = tsc_ofplugin_v1_3_init(tsc_domain_handle);
  if(status != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"OF ver 1.3 plugin initialization failed ");
    return OF_FAILURE;
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"OF ver 1.3 plugin initialization successful");
    return OF_SUCCESS;
  }
}
/******************************************************************************************************/
int32_t tsc_uninit_of_plugins(uint64_t tsc_domain_handle)
{
  tsc_ofplugin_v1_3_uninit(tsc_domain_handle);
  return OF_SUCCESS;
}    
/*******************************************************************************************************
Function Name: crm_port_notifications_cbk
Input Parameters:
	notification_type: Add,Delete,Modify
 	crm_port_handle  : Handle to the crm port for which notification is received.
        notification_data: Other data associated with the port notification. 
Output Parameters: NONE
Description:Appropriate functions are called for each of the port notification types.
*******************************************************************************************************/
void crm_port_notifications_cbk(uint32_t notification_type,
                                uint64_t crm_port_handle,
                                struct   crm_port_notification_data notification_data,
                                void     *callback_arg1,
                                void     *callback_arg2)
{
  int32_t  retval;
  uint32_t temp_id;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"crm_port_notifications_cbk() is called: notification type = %d",notification_type);

  if(notification_type == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_PORT_ALL_NOTIFICATIONS");
    return;
  }

  if(notification_type == CRM_PORT_MODIFIED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_PORT_MODIFIED"); /* Only id is changed.Some body manually deleted the port and added again with the same name. */
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port type     = %d",notification_data.crm_port_type);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port name     = %s",notification_data.port_name);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw type       = %d",notification_data.nw_type);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"old port id   = %d",notification_data.old_port_id);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"new port id   = %d",notification_data.new_port_id);

    /* Delete flows using old_port_id. */
   
    temp_id = notification_data.new_port_id;
    notification_data.new_port_id = notification_data.old_port_id;
 
    retval = tsc_handle_crm_port_deletion(notification_type,
                                          crm_port_handle,
                                          notification_data,
                                          callback_arg1,
                                          callback_arg2);
    if(retval == OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully handled CRM PORT DELETION as part of CRM PORT modificastion.");

      notification_data.new_port_id = temp_id;
    
      retval = tsc_handle_crm_port_addition(notification_type,
                                            crm_port_handle,
                                            notification_data,
                                            callback_arg1,
                                            callback_arg2);
      if(retval == OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully handled CRM PORT ADDITION as part of CRM PORT modification.");
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Failed to Handle CRM PORT ADDITION as part of CRM PORT modification.");
      }
      
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Failed to handle CRM PORT DELETION as part of CRM PORT modification.");
    }
    return;
  }

  if(notification_type == CRM_PORT_ADDED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_PORT_ADDED");
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port type = %d",notification_data.crm_port_type);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port name = %s",notification_data.port_name);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw type   = %d",notification_data.nw_type);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port id   = %d",notification_data.new_port_id);

    if(notification_data.new_port_id == 0)
      return;

    retval = tsc_handle_crm_port_addition(notification_type,
                                          crm_port_handle,
                                          notification_data,
                                          callback_arg1,
                                          callback_arg2);
    if(retval == OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully handled CRM PORT ADDITION.");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Failed to handle CRM PORT ADDITION.");
    }   
    return;
  }

  if(notification_type == CRM_PORT_DELETE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_PORT_DELETE");
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port type = %d",notification_data.crm_port_type);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port name = %s",notification_data.port_name);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw type   = %d",notification_data.nw_type);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"port id   = %d",notification_data.new_port_id);

    if(notification_data.new_port_id == 0)
      return;
    
    retval = tsc_handle_crm_port_deletion(notification_type,
                                          crm_port_handle,
                                          notification_data,
                                          callback_arg1,
                                          callback_arg2);
    if(retval == OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully handled CRM PORT DELETION.");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Failed to handle CRM PORT DELETION.");
    }
    return;
  }
}
/**************************************************************************************************************************
Function Name: crm_vn_notifications_cbk
Input Parameters:
        notification_type: Add,Delete,Modify
        vn_handle        : Handle to the Virtual Network for which notification is received.
        notification_data: Other data associated with the VN notification.
Output Parameters: NONE
Description:Appropriate functions are called for each of the VN notification types.
            CRM_VN_ADDED:   An internal view of the Virtual Networks based on network id is created for each Network Type.
            CRM_VN_DELETED: The created view database is deleted.
            Databases for cnbind_entries and repository entries related to L2 Service Chaining and Flow Control are 
            implemented per L2 Virtual Network.Creation and initialization of these databases is done.
**************************************************************************************************************************/
void crm_vn_notifications_cbk(uint32_t notification_type,
                              uint64_t vn_handle,
                              struct   crm_vn_notification_data notification_data,
                              void     *callback_arg1,
                              void     *callback_arg2)
{
  int32_t  retval;
  struct   crm_virtual_network* vn_entry_p;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"crm_vn_notifications_cbk() is called: notification type = %d",notification_type);

  if(notification_type == 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_ALL_NOTIFICATIONS");
    return;
  }

  if(notification_type == CRM_VN_ADDED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_ADDED");
  
    retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"crm_vn_notifications_cbk - Unable to fetch vn_entry");
      return; 
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"fetched vn_entry nw_type = %d",notification_data.nw_type);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vnhandled MURTHY = %llx",vn_handle);
    }
    retval = tsc_nvm_modules[notification_data.nw_type].nvm_module_add_vn_to_nid_view(vn_handle,notification_data.tun_id);
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to add vn to the tunid view");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Added vn to the tunid view");
    }

    /* This API is common for all nw_types */
    retval = nsc_init_vn_resources(vn_entry_p);  

     if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to initialize VN resources.");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully initialized vn resources");
    }
    return;  
  }
  if(notification_type == CRM_VN_DELETE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_DELETE");
   
     retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"crm_vn_notifications_cbk - Unable to fetch vn_entry");
      return;
    }

    retval = tsc_nvm_modules[notification_data.nw_type].nvm_module_delete_vn_from_nid_view(vn_handle,notification_data.tun_id);
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to delete vn from the tunid view");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Deleted vn to the tunid view");
    }

    retval = nsc_deinit_vn_resources(vn_entry_p);

    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to uninitialize vn resources.");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully uninitialized vn resources");
    }
    return;
  }
  
  /* As other VN notifications are not used at present, returning from the callback */
  /* Comment the return statement in next line to process other vn notifications. */

  return;  

  if(notification_type == CRM_VN_MODIFIED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_MODIFIED");
    return;
  }
  if(notification_type == CRM_VN_VIEW_ADDED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_VIEW_ADDED");
    return;
  }
  if(notification_type == CRM_VN_VIEW_DELETE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_VIEW_DELETE");
    return;
  }
  if(notification_type == CRM_VN_ATTRIBUTE_ADDED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_ATTRIBUTE_ADDED");
    return;
  }
  if(notification_type == CRM_VN_ATTRIBUTE_DELETE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_ATTRIBUTE_DELETE");
    return;
  }
  if(notification_type == CRM_VN_SUBNET_ADDED)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_SUBNET_ADDED");
    return;
  }
  if(notification_type == CRM_VN_SUBNET_DELETE)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"CRM_VN_SUBNET_DELETE");
    return;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"crm_vn_notifications_cbk - End of notifications");
  return;
}
/**************************************************************************************************************************
Function Name: tsc_handle_crm_port_addition 
Input Parameters:
        notification_type: Add
        crm_port_handle  : Handle to the CRM Port for which notification is received.
        notification_data: Other data associated with the CRM Port Add notification.
Output Parameters: NONE
Description:
            1.Internal views are created separately for the CRM VM Ports and CRM Network Ports Added.
            2.CRM Port based flows are added proactively to the classification Table 0.   
**************************************************************************************************************************/
int32_t tsc_handle_crm_port_addition(uint32_t notification_type,
                                     uint64_t crm_port_handle,
                                     struct   crm_port_notification_data notification_data,
                                     void     *callback_arg1,
                                     void     *callback_arg2)
{
 
  struct crm_port* crm_port_node_p;

  int32_t retval = OF_FAILURE;;
  uint8_t nw_type;

  nw_type = notification_data.nw_type;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw_type = %d",nw_type);
  if((nw_type < SUPPORTED_NW_TYPE_MIN) ||(nw_type > SUPPORTED_NW_TYPE_MAX))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Un Supported NW Type");
    return OF_FAILURE;
  }

  if((notification_data.crm_port_type == VM_PORT)      || (notification_data.crm_port_type == VMNS_PORT) ||
     (notification_data.crm_port_type == VMNS_IN_PORT) || (notification_data.crm_port_type == VMNS_OUT_PORT)) 
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Calling nvm_module_create_vmport_views()");  
    retval = tsc_nvm_modules[nw_type].nvm_module_create_vmport_views(crm_port_handle,
                                                                     notification_data.crm_port_type,
                                                                     notification_data.crm_vn_handle);

    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add vmport views for the nw type = %d",nw_type);
      return OF_FAILURE;  
    }
      
    /* Add Flows to classify_table_0 proactively */ 

    if(notification_data.crm_port_type == VM_PORT) /* Call this API for VM-NS ports from Launch API */   
    {
      retval = tsc_add_vmport_flows_to_classify_table(notification_data.crm_vn_handle,
                                                      notification_data.swname,
                                                      notification_data.crm_port_type,
                                                      notification_data.new_port_id,                  
                                                      0,0,   
                                                      notification_data.dp_handle);
      if(retval != OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add vmport flows to classify table for the nw type = %d",nw_type);
      }
     
      /* Add a proactive flow to Table 3 to reach this VM within local compute node */ 
      /* TBD May be it is required for VM_NS also */
      retval = tsc_add_t3_proactive_flow_for_vm(crm_port_handle,notification_data.crm_vn_handle,notification_data.dp_handle);
     
    }
    tsc_delete_table_4_flow_entries(notification_data.crm_port_type,crm_port_handle,notification_data.dp_handle);
    tsc_delete_table_5_flow_entries(notification_data.crm_port_type,crm_port_handle,notification_data.dp_handle);

  }
  else /* Network Port */
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Calling nvm_module_create_nwport_views()");
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"crm nwport handle = %llx",crm_port_handle);
 
    retval = crm_get_port_byhandle(crm_port_handle,&crm_port_node_p);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Invalid crm port handle");
      return OF_FAILURE;
    }
    
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nw service port =%x", (crm_port_node_p->nw_params).vxlan_nw.service_port);
    retval = tsc_nvm_modules[nw_type].nvm_module_create_nwport_views(crm_port_handle,
                                                                     notification_data.crm_port_type,
                                                                     notification_data.swname); 
     
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add nwport to nwport views. port_type = ",notification_data.crm_port_type);
      return OF_FAILURE; 
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully added the nwport to nwport views. port_type = ",notification_data.crm_port_type);
  

    retval = tsc_vxn_add_nwport_flows(notification_data.swname,
                                      &(notification_data.nw_params.vxlan_nw),
                                      notification_data.crm_port_type,
                                      notification_data.new_port_id,
                                      notification_data.dp_handle);
  
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add nwport flows. port_type = ",notification_data.crm_port_type);
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully added nwport flows. port_type = ",notification_data.crm_port_type);
  }
  return OF_SUCCESS;   
}
/**************************************************************************************************************************
Function Name: tsc_handle_crm_port_deletion
Input Parameters:
        notification_type: Delete
        crm_port_handle  : Handle to the CRM Port for which notification is received.
        notification_data: Other data associated with the CRM Port Delete notification.
Output Parameters: NONE
Description:
            1.Internal views are deleted separately for the CRM VM Ports and CRM Network Ports Deleted.
            2.CRM Port based flows are deleted proactively from the classification Table 0.
**************************************************************************************************************************/
int32_t tsc_handle_crm_port_deletion(uint32_t notification_type,
                                     uint64_t crm_port_handle,
                                     struct   crm_port_notification_data notification_data,
                                     void     *callback_arg1,
                                     void     *callback_arg2)
{
  int32_t  retval = OF_FAILURE;
  uint8_t  nw_type;
  uint8_t  table_id;

  nw_type = notification_data.nw_type;

  if((nw_type < SUPPORTED_NW_TYPE_MIN) ||(nw_type > SUPPORTED_NW_TYPE_MAX)) 
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unsupported NW Type");
    return OF_FAILURE;
  }

  if((notification_data.crm_port_type == VM_PORT) || (notification_data.crm_port_type == VMNS_PORT) ||
     (notification_data.crm_port_type == VMNS_IN_PORT) || (notification_data.crm_port_type == VMNS_OUT_PORT))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Calling nvm_module_delete_vmport_views()");
    retval = tsc_nvm_modules[nw_type].nvm_module_delete_vmport_views(crm_port_handle,
                                                                     notification_data.crm_port_type,
                                                                     notification_data.crm_vn_handle);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete vmport views for the nw type = %d",nw_type);
    }
        
    /* Table_0:  Delete flow entries from classification table using port id as match field.*/
 
    retval = tsc_delete_vmport_flows_from_classify_table(notification_data.crm_port_type,
                                                         notification_data.new_port_id,
                                                         notification_data.dp_handle);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete  vmport flows from classify table for the nw type = %d",nw_type);
    }

    tsc_delete_table_4_flow_entries(notification_data.crm_port_type,crm_port_handle,notification_data.dp_handle);    
    tsc_delete_table_5_flow_entries(notification_data.crm_port_type,crm_port_handle,notification_data.dp_handle);
    
    if(notification_data.crm_port_type == VM_PORT)
    {    
      table_id = TSC_APP_OUTBOUND_NS_CHAIN_TABLE_ID_1;
      tsc_ofplugin_v1_3_del_flows_from_table_1_2(notification_data.dp_handle,&notification_data.new_port_id,table_id);

      table_id = TSC_APP_INBOUND_NS_CHAIN_TABLE_ID_2;
      tsc_ofplugin_v1_3_del_flows_from_table_1_2(notification_data.dp_handle,&notification_data.new_port_id,table_id);
    }
    else if((notification_data.crm_port_type == VMNS_PORT) || (notification_data.crm_port_type == VMNS_IN_PORT) ||
                                                              (notification_data.crm_port_type == VMNS_OUT_PORT))
    {
      //printf("\r\nCalling tsc_delete_nsc_tables_1_2_flows_for_service_vm_deleted()");
      retval = tsc_delete_nsc_tables_1_2_flows_for_service_vm_deleted(notification_data.crm_vn_handle,crm_port_handle);
      if(retval == OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Successfully deleted all the matching Table_1 and Table_2 flow entries");
        printf("\r\nSuccessfully deleted all the matching Table_1 and Table_2 flow entries");
      }
      else
      {
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Failed to delete  some or all of the matching Table_1 and Table_2 flow entries");
        printf("\r\nFailed to delete all or some of the matching Table_1 and Table_2 flow entries");
      }
    }
    tsc_delete_table_3_flow_entries(notification_data.crm_port_type,crm_port_handle,notification_data.dp_handle); 
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Calling nvm_module_delete_nwport_views()");
    retval = tsc_nvm_modules[nw_type].nvm_module_delete_nwport_views(crm_port_handle,
                                                                     notification_data.crm_port_type,
                                                                     notification_data.swname);

    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete the nwport from nwport views. port_type = ",notification_data.crm_port_type);
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully deleted the nwport from nwport views. port_type = ",notification_data.crm_port_type);

    /* Table_0:  Delete flow entries from classification table using port id as match field.*/ 
    retval = tsc_delete_nwport_flows_from_classify_table(notification_data.crm_port_type,
                                                         notification_data.new_port_id,
                                                         notification_data.dp_handle);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete the nwport flows. port_type = ",notification_data.crm_port_type);
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Successfully deleted the nwport flows. port_type = ",notification_data.crm_port_type);
    }

    tsc_delete_table_4_flow_entries(notification_data.crm_port_type,crm_port_handle,notification_data.dp_handle);
  }
  return retval;
}
/**************************************************************************************************************************
 * Function Name:tsc_app_tbls
 * Description:  Registers TSC tables with ttprm
 * ***************************************************************************************************************************/
int32_t tsc_app_tbls_reg(void)
{
  int32_t ret_val = CNTLR_TTP_SUCCESS;
  uint32_t table_id;
  struct  ttprm_table_entry *ttp_table_node_p = NULL;

  OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, "entered");

  for(table_id=0; table_id < CNTLR_TSC_TTP_TABLES_CNT; table_id++)
  {
    /** Create memory for the table */
    ret_val = alloc_ttp_tbl_mem_block(&ttp_table_node_p);
    if(ret_val!=CNTLR_TTP_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_SAMPLE_APP, OF_LOG_ERROR, "tsc ttp table allocation failed!.");
      ret_val = CNTLR_TTP_FAILURE;
      break;
    }

    switch(table_id)
    {
      /* Table id 0:In Port Classification Table */
      case DC_VS_TTP_CLASSIFICATION_TABLE_ID:
           ttp_table_node_p->table_id = DC_VS_TTP_CLASSIFICATION_TABLE_ID;
           strcpy(ttp_table_node_p->table_name,CNTLR_DC_VS_TTP_ID0_NAME);
           ttp_table_node_p->max_columns    = CNTLR_DC_VS_TTP_ID0_COLUMNS;
           ttp_table_node_p->max_rows       = CNTLR_DC_VS_TTP_ID0_ROWS;
           ttp_table_node_p->is_wc_present=FALSE;
           ttp_table_node_p->is_lpm_present=FALSE;
           ttp_table_node_p->no_of_wcs=0;
           ttp_table_node_p->no_of_lpms=0;

           break;

      /*Table id1: NS1 table */
      case DC_VS_TTP_NS1_TABLE_ID:
           ttp_table_node_p->table_id = DC_VS_TTP_NS1_TABLE_ID;
           strcpy(ttp_table_node_p->table_name,CNTLR_DC_VS_TTP_ID1_NAME);

           ttp_table_node_p->max_columns    = CNTLR_DC_VS_TTP_ID1_COLUMNS;
           ttp_table_node_p->max_rows       = CNTLR_DC_VS_TTP_ID1_ROWS;

           ttp_table_node_p->is_wc_present=FALSE;
           ttp_table_node_p->is_lpm_present=FALSE;
           ttp_table_node_p->no_of_wcs=0;
           ttp_table_node_p->no_of_lpms=0;

           break;
      
      /* Table id2: NS2 Table */
      case DC_VS_TTP_NS2_TABLE_ID:
           ttp_table_node_p->table_id = DC_VS_TTP_NS2_TABLE_ID;
           strcpy(ttp_table_node_p->table_name,CNTLR_DC_VS_TTP_ID2_NAME);

           ttp_table_node_p->max_columns    = CNTLR_DC_VS_TTP_ID2_COLUMNS;
           ttp_table_node_p->max_rows       = CNTLR_DC_VS_TTP_ID2_ROWS;

           ttp_table_node_p->is_wc_present=FALSE;
           ttp_table_node_p->is_lpm_present=FALSE;
           ttp_table_node_p->no_of_wcs  =0;
           ttp_table_node_p->no_of_lpms =0;

           break;

      case DC_VS_TTP_UNICAST_TABLE_ID:
           ttp_table_node_p->table_id = DC_VS_TTP_UNICAST_TABLE_ID;
           strcpy(ttp_table_node_p->table_name,CNTLR_DC_VS_TTP_ID3_NAME);

           ttp_table_node_p->max_columns    = CNTLR_DC_VS_TTP_ID3_COLUMNS;
           ttp_table_node_p->max_rows       = CNTLR_DC_VS_TTP_ID3_ROWS;

           ttp_table_node_p->is_wc_present=FALSE;
           ttp_table_node_p->is_lpm_present=FALSE;
           ttp_table_node_p->no_of_wcs  = 0;
           ttp_table_node_p->no_of_lpms = 0;

           break;
          
      case DC_VS_TTP_BRDOUT_TABLE_ID:
           ttp_table_node_p->table_id = DC_VS_TTP_BRDOUT_TABLE_ID;
           strcpy(ttp_table_node_p->table_name,CNTLR_DC_VS_TTP_ID4_NAME);

           ttp_table_node_p->max_columns    = CNTLR_DC_VS_TTP_ID4_COLUMNS;
           ttp_table_node_p->max_rows       = CNTLR_DC_VS_TTP_ID4_ROWS;

           ttp_table_node_p->is_wc_present=FALSE;
           ttp_table_node_p->is_lpm_present=FALSE;
           ttp_table_node_p->no_of_wcs= 0;
           ttp_table_node_p->no_of_lpms=0;

           break;

      case DC_VS_TTP_BRDIN_TABLE_ID:
           ttp_table_node_p->table_id = DC_VS_TTP_BRDIN_TABLE_ID;
           strcpy(ttp_table_node_p->table_name,CNTLR_DC_VS_TTP_ID5_NAME);

           ttp_table_node_p->max_columns  = CNTLR_DC_VS_TTP_ID5_COLUMNS;
           ttp_table_node_p->max_rows     = CNTLR_DC_VS_TTP_ID5_ROWS;

           ttp_table_node_p->is_wc_present=FALSE;
           ttp_table_node_p->is_lpm_present=FALSE;
           ttp_table_node_p->no_of_wcs=0;
           ttp_table_node_p->no_of_lpms=0;

           break;
    }

    ret_val = add_table_to_ttprm("DATA_CENTER_VIRTUAL_SWITCH_TTP", &ttp_table_node_p);
    if(ret_val != CNTLR_TTP_SUCCESS)
    {
        OF_LOG_MSG(OF_LOG_SAMPLE_APP, OF_LOG_ERROR, "add table to ttprm  failed.");
        ret_val = CNTLR_TTP_FAILURE;
        break;
    }
  }
  return ret_val;
}
/***************************************************************************************************************************
Function Name:cntlr_shared_lib_init
Description:
	1. TSC module is implemented as a shared library.
        2. This function is called by the shared library loader.
	3. This function calls the API tsc_module_init() to initialize TSC module.         
***************************************************************************************************************************/  
void cntlr_shared_lib_init(void)
{
  int32_t retval;
  do
  {
    retval =  tsc_module_init();  
    if(retval != OF_SUCCESS)
    {
      puts("\r\n Flow Control Module Initialization failed");
    }
    else
    {
      puts("\r\n Flow Control Module Initialization is successful");
    }

  }while(0);
}
/***************************************************************************************************************************
Function Name:cntlr_shared_lib_deinit
Description:
        1. TSC module is implemented as a shared library.
        2. This function is called by the shared library loader.
        3. This function calls the API xxx() to de-initialize the TSC module.
***************************************************************************************************************************/
void cntlr_shared_lib_deinit()
{
  nsrm_uninit();
  psm_module_uninit();  
  crm_deregister_vn_notifications(CRM_VN_ALL_NOTIFICATIONS,crm_vn_notifications_cbk);  
  crm_deregister_port_notifications(CRM_PORT_ALL_NOTIFICATIONS,crm_port_notifications_cbk);
  crm_uninit();
  tsc_uninit_of_plugins(tsc_domain_handle);
  nsc_uninit();
  printf("\r\n TSC Application is de-initialized");
}
/***************************************************************************************************************************/
/* Reads the Application VM zone information from the file /home/user/tsc.conf                                             */    
/***************************************************************************************************************************/
int32_t tsc_get_zone_info(void)
{
  char line[150];
  char *token       = NULL;
  FILE *fp;
  uint32_t src_ip,zone,ii;
 
  fp = fopen("/usr/local/ondir_platform/2.0/lib/apps/tsc.conf","rt");
  if(fp == NULL)
  {
    printf("\r\n tsc.conf file is not found. Assuming all the Application VMs are ZONE_LESS .");
    return OF_SUCCESS;
  }
  printf("\r\n");
  
  tsc_bintree_root = NULL;
  tsc_bintree_root = tsc_bintree_insert(tsc_bintree_root,0,0);

  while(fgets(line, 150, fp) != NULL)
  {
    if((line[0] == '#') || ((line[0] == '/') && (line[1] == '*')))
      continue;
    if(strlen(line) == 1) /* skip blank lines and comments */
      continue;

    token = (char *)strtok(line,"=");

    if(!strncmp(token,"vmip",strlen("vmip")))
    {
      //printf(" \r\n token   = %s",token);
      token = (char *)strtok(NULL," "); 
      strncpy(vm_ipaddr,token,strlen(token));
      //printf(" vm_ipaddr = %s",vm_ipaddr); 
      src_ip = inet_network(vm_ipaddr);
    }
    else
    {
      continue;
    }
    token = (char *)strtok(NULL,"="); 
    if(!(strncmp(token,"zone",strlen("zone"))))
    {
      token = (char *)strtok(NULL,"\t");
      strncpy(vm_zone,token,strlen(token));
      if(!(strncmp(vm_zone,"left",strlen("left"))))
        zone = ZONE_LEFT;
      else if(!(strncmp(vm_zone,"right",strlen("right"))))
        zone = ZONE_RIGHT;
      else
        zone = ZONE_LESS;
    }
    else
    {
      continue;
    }  
    printf("\r\n vm_ip   = %x",src_ip);
    printf(" zone    = %x",zone);
    /* Add to Binary tree */
  
    tsc_bintree_insert(tsc_bintree_root,src_ip,zone);

    for(ii=0;ii<18;ii++)
       vm_ipaddr[ii] = 0;
    for(ii=0;ii<6;ii++)
       vm_zone[ii] = 0;
  }
  fclose(fp);
  tsc_bintree_list(tsc_bintree_root);
  return OF_SUCCESS;
}
/********************************************************************************************************************************/









