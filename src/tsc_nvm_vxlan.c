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

/* File: tsc_nvm_vxlan.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:VXLAN specific code.
 */

#include "controller.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "tsc_controller.h"
#include "tsc_nvm_vxlan.h"
#include "tsc_vxn_views.h"
#include "tsc_ofv1_3_plugin.h"

extern struct tsc_vxn_swname_vmport_database_view swname_vmport_view;

int32_t tsc_nvm_vxlan_init(void)
{
  int32_t retval,status = OF_SUCCESS;
  
  do
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Entered VXLAN NVM module to initialize");
    /* Tunid View of Virtual Networks */
    retval = tsc_vxn_vn_nid_view_init(TSC_VXN_TUNNID_VIEW_OF_VN);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to initialize tunid view of vn");
      status = OF_FAILURE;
      break;
    }

    /* DMAC View of VM and VM-NS ports - nid is a qualifier */
    retval = tsc_vxn_vmport_portmac_view_init(TSC_VXN_DSTMAC_VIEW_OF_VMPORTS);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to initialize dmac view of vmports");
      status = OF_FAILURE;
      break;
    }
    /* Switch Name View of VM and VM-NS ports - nid is a qualifier */
    retval = tsc_vxn_vmport_swname_view_init(TSC_VXN_SWNAME_VIEW_OF_VMPORTS);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to initialize swname view of vmports");
      status = OF_FAILURE;
      break;
    }
    /* Switch Name View of NW Unicast ports. service_port is a qualifier */  
    retval = tsc_vxn_unicast_nwport_swname_view_init(TSC_VXN_SWNAME_VIEW_OF_UNICAST_NWPORTS);  
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to initialize swname view of nw_unicast ports");
      status = OF_FAILURE;
      break;
    }
    retval = tsc_vxn_broadcast_nwport_swname_view_init(TSC_VXN_SWNAME_VIEW_OF_BROADCAST_NWPORTS);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to initialize swname view of nw_broadcast ports");
      status = OF_FAILURE;
      break;
    }
  }while(0); 

  if(status != OF_SUCCESS)
  {
    tsc_vxn_vn_nid_view_uninit();
    tsc_vxn_vmport_portmac_view_uninit();
    tsc_vxn_vmport_swname_view_uninit();
    tsc_vxn_unicast_nwport_swname_view_uninit();
    tsc_vxn_broadcast_nwport_swname_view_uninit();
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Successfully initialized VXLAN NVM module");
  }
  return status;
}

int32_t tsc_nvm_vxlan_create_vmport_views(uint64_t crm_port_handle,
                                          uint8_t  crm_port_type,
                                          uint64_t vn_handle)
{
  struct  crm_virtual_network* vn_entry_p;
  struct  crm_vxlan_nw vxlan_nw;
 
  int32_t retval;
  int32_t status = OF_SUCCESS;

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);

  if(retval != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unable to fetch vn_entry");
    return OF_FAILURE;
  }

  vxlan_nw = vn_entry_p->nw_params.vxlan_nw;

  do
  {
    /* Portmac view: Get MAC Address from crm port structure. */
    retval = tsc_vxn_add_vmport_to_portmac_view(crm_port_handle,vxlan_nw.nid);

    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add vmside port to the portmac view");
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Added vmside port to the portmac view");

    /* Switch Name view: Get Switch Name from crm_port_handle. */
    retval =  tsc_vxn_add_vmport_to_swname_view(crm_port_handle,vxlan_nw.nid);
    
    if(retval != OF_SUCCESS)
    {  
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to add vmside port to the switch view");
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Added vmside port to the switch view");
  }while(0);

  return status;
}

int32_t tsc_nvm_vxlan_delete_vmport_views(uint64_t crm_port_handle,
                                          uint8_t  crm_port_type,
                                          uint64_t vn_handle)
{
  struct  crm_virtual_network* vn_entry_p;
  struct  crm_vxlan_nw vxlan_nw;

  int32_t retval;
  int32_t status = OF_SUCCESS;

  retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);

  if(retval != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unable to fetch vn_entry");
    return OF_FAILURE;
  }

  vxlan_nw = vn_entry_p->nw_params.vxlan_nw;

  do
  {
    /* Portmac view: Get MAC Address from crm port structure. */
    retval = tsc_vxn_del_vmport_from_portmac_view(crm_port_handle,vxlan_nw.nid);

    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to delete vmside port from the portmac view");
      status = OF_FAILURE;
      break;
    }    
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Deleted vmside port from the portmac view");
    
    /* Switch Name view: Get Switch Name from crm_port_handle. */
    retval = tsc_vxn_del_vmport_from_swname_view(crm_port_handle,vxlan_nw.nid);
    if(retval != OF_SUCCESS)
    {              
      OF_LOG_MSG(OF_LOG_TSC,OF_LOG_ERROR,"Failed to delete vmside port from the switch view");
      status = OF_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC,OF_LOG_DEBUG,"Deleted vmside port from the switch view");
  }while(0);  

  return status;
}
int32_t tsc_nvm_vxlan_create_nwport_views(uint64_t crm_port_handle,
                                          uint8_t  crm_port_type,
                                          char*    swname_p)
{
  int32_t retval;
  struct  crm_vxlan_nw  vxlan_nw;  
  struct  crm_port*     crm_port_node_p;


  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "crm nwport handle %llx", crm_port_handle);
 

  CNTLR_RCU_READ_LOCK_TAKE(); 
  
  retval = crm_get_port_byhandle(crm_port_handle,&crm_port_node_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Invalid crm port handle");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }

  vxlan_nw = (crm_port_node_p->nw_params.vxlan_nw);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nw service port =%x", (crm_port_node_p->nw_params).vxlan_nw.service_port);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "switch name =%s",swname_p);

  CNTLR_RCU_READ_LOCK_RELEASE();

  if(crm_port_type == NW_UNICAST_PORT)
  {
    retval = tsc_vxn_add_unicast_nwport_to_swname_view(crm_port_handle,
                                                       vxlan_nw.service_port);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add nw unicast port to the view");
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Successfully added the nw unicast port to the view");
  }
  else if(crm_port_type == NW_BROADCAST_PORT)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " vni       = %x", vxlan_nw.nid);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " remote_ip = %x", vxlan_nw.remote_ip);

    retval = tsc_vxn_add_broadcast_nwport_to_swname_view(crm_port_handle,
                                                         vxlan_nw.service_port,
                                                         0, // vxlan_nw_p->nid,   /* TBD Send 0 as input from configuration itself */
                                                         vxlan_nw.remote_ip);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to add nw broadcast port to the view");
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Successfully added the nw broadcast port to the view");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unsupported Network port type"); 
    return OF_FAILURE; 
  } 
  return OF_SUCCESS;
}

int32_t tsc_nvm_vxlan_delete_nwport_views(uint64_t crm_port_handle,
                                          uint8_t  crm_port_type,
                                          char*    swname_p)
{
  int32_t retval;
  struct  crm_vxlan_nw* vxlan_nw_p = NULL;
  struct  crm_port*     crm_port_node_p;

  retval = crm_get_port_byhandle(crm_port_handle,&crm_port_node_p);
  if(retval != OF_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Invalid crm port handle");
    return OF_FAILURE;
  }

  vxlan_nw_p = &(crm_port_node_p->nw_params.vxlan_nw);

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nw service port =%d", vxlan_nw_p->service_port);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "switch name =%s",swname_p);

  if(crm_port_type == NW_UNICAST_PORT)
  {
    retval = tsc_vxn_del_unicast_nwport_from_swname_view(crm_port_handle,
                                                         vxlan_nw_p->service_port);

    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete unicast nwport from views");
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Successfully deleted the unicast nwport from views");
  }
  else if(crm_port_type == NW_BROADCAST_PORT)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " vni = %x", vxlan_nw_p->nid);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " remote_ip = %x", vxlan_nw_p->remote_ip);

    retval = tsc_vxn_del_broadcast_nwport_from_swname_view(crm_port_handle,
                                                           vxlan_nw_p->service_port,
                                                           0, //vxlan_nw_p->nid,
                                                           vxlan_nw_p->remote_ip);
    if(retval != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to delete broadcast nwport from views");
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Successfully deleted the broadcast nwport from views");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unsupported Network port type");
    return OF_FAILURE;
  }
  return OF_SUCCESS;
}

void tsc_nvm_vxlan_construct_metadata(struct tsc_ofver_plugin_iface* tsc_params_p,uint64_t vn_handle,struct crm_port* crm_port_p)  
{
  uint64_t metadata;
  struct   crm_virtual_network* vn_entry_p;
  struct   crm_vxlan_nw* vxlan_nw_p = NULL;
  int32_t  retval = OF_SUCCESS;   
 
  if(crm_port_p == NULL)
  {
    retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);
    if(retval == CRM_SUCCESS)
    {
      vxlan_nw_p = &vn_entry_p->nw_params.vxlan_nw;
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Unable to fetch vn_entry");
    } 
  }
  else
  {
    vxlan_nw_p = &(crm_port_p->nw_params.vxlan_nw);
  }
  
  tsc_params_p->serviceport = vxlan_nw_p->service_port;
  tsc_params_p->vni         = vxlan_nw_p->nid;
  tsc_params_p->nw_type     = VXLAN_TYPE;
 
  metadata = 0;
  metadata = (((uint64_t)(vxlan_nw_p->service_port)) << SERVICE_PORT_OFFSET);
  metadata = metadata | (vxlan_nw_p->nid)
                      | (((uint64_t)(tsc_params_p->nw_type)) << NW_TYPE_OFFSET); 

  tsc_params_p->metadata = metadata;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Constructed metadata = %llx",tsc_params_p->metadata);
}

/* TBD Assuming that network ports are added before adding VM_NS ports */
/* VM ports can be added at any time */

int32_t tsc_vxn_add_nwport_flows(char*    swname_p,
                                 struct   crm_vxlan_nw* vxlan_nw_p,
                                 uint8_t  nwport_type,
                                 uint32_t port_id,
                                 uint64_t dp_handle)
{
  uint32_t  hashkey;
  struct    tsc_vxn_swname_vmport_view_node *view_node_entry_p = NULL;
  struct    crm_port *port_info_p = NULL;
  struct    crm_virtual_network* vn_entry_p;
  uint64_t  metadata;

  uchar8_t  offset;
  struct    tsc_ofver_plugin_iface* tsc_params_p;
  int32_t   status = OF_SUCCESS;

  CNTLR_RCU_READ_LOCK_TAKE();

  hashkey = tsc_vxn_get_hashval_byname(swname_p,swname_vmport_view.no_of_view_node_buckets_g);
  offset = TSC_VXN_SWNAME_VMPORT_VIEW_NODE_TBL_OFFSET;
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"entered 1");
  MCHASH_BUCKET_SCAN(swname_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p,
                                     struct tsc_vxn_swname_vmport_view_node *, offset)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"switch name = %s",view_node_entry_p->swname);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"switch name = %s",swname_p);

    if(!strcmp(view_node_entry_p->swname, swname_p))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"entered 2");
      status = crm_get_port_byhandle(view_node_entry_p->port_handle, &port_info_p);
      if(status == OF_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"entered 3");

        status = crm_get_vn_byhandle(port_info_p->saferef_vn,&vn_entry_p);
        if(status == OF_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"entered 4")
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn_entry_p->nw_type = %d",vn_entry_p->nw_type);
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn_entry_p->nw_params.vxlan_nw.nid = %x",
                                               vn_entry_p->nw_params.vxlan_nw.nid);
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vn_entry_p->nw_params.vxlan_nw.service_port = %d",
                                               vn_entry_p->nw_params.vxlan_nw.service_port); 
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vxlan_nw_p->service_port = %d",vxlan_nw_p->service_port);
       

          if((vn_entry_p->nw_type == VXLAN_TYPE) &&
             (vn_entry_p->nw_params.vxlan_nw.service_port == vxlan_nw_p->service_port))
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"entered 5")
            /* Add a flow for the unicast port added. It  may work for broadcast also. */
            tsc_params_p = (struct tsc_ofver_plugin_iface *) malloc(sizeof (struct tsc_ofver_plugin_iface));
            strcpy(tsc_params_p->local_switch_name,swname_p);

            tsc_params_p->serviceport = vxlan_nw_p->service_port;
            tsc_params_p->vni         = vxlan_nw_p->nid;
            tsc_params_p->nw_type     = VXLAN_TYPE;

            metadata = 0;
            metadata = (((uint64_t)(vxlan_nw_p->service_port)) << SERVICE_PORT_OFFSET);
            metadata = metadata | (vxlan_nw_p->nid)
                                | (((uint64_t)(tsc_params_p->nw_type)) << NW_TYPE_OFFSET);

            tsc_params_p->metadata = metadata;

            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Constructed metadata = %llx",tsc_params_p->metadata);

            tsc_params_p->metadata = (tsc_params_p->metadata |
                                   (((uint64_t)(nwport_type)) << PKT_ORIGIN_OFFSET));
            tsc_params_p->metadata_mask = 0xffffffffffffffff;
        
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"metadata = %llx",tsc_params_p->metadata);   
         
            tsc_params_p->pkt_origin_e = nwport_type;
            tsc_params_p->dp_handle    = dp_handle;
            tsc_params_p->in_port_id   = port_id;

          #if 1  
            if(port_info_p->crm_port_type == VMNS_PORT) 
            {
              tsc_params_p->vlan_id_in  = port_info_p->vlan_id_in;
              tsc_params_p->vlan_id_out = port_info_p->vlan_id_out;

            }
            else
            {
              tsc_params_p->vlan_id_in  = 0; 
              tsc_params_p->vlan_id_out = 0; 
            }  
           #endif

            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vm port type = %d",port_info_p->crm_port_type);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vm port id = %d",port_info_p->port_id);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nwport type = %d",nwport_type);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"nw port id = %d",port_id);
          
            status = tsc_ofplugin_v1_3_send_flow_mod_classify_table_nwports(dp_handle,tsc_params_p,OFPFC_ADD);
          }
        }
      }
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return OF_SUCCESS;
}
/**********************************************************************************************************/
