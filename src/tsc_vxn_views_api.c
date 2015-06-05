/*
 *
 * Copyright  2012, 2013  Freescale Semiconductor
 *
 *
 * Licensed under the Apache License, version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
*/
/*
 *
 * File name: tsc_vxn_views_api.c
 * Author: ns murthy(b37840@freescale.com), Varun Kumar Reddy <B36461@freescale.com>
 * Date:   03/05/2014
 * Description:internal views  on crm database for vxlan nvm.
*/

#include "controller.h"
#include "cntlr_epoll.h"
#include "cntrl_queue.h"
#include "of_utilities.h"
#include "of_msgapi.h"
#include "of_multipart.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "crmldef.h"
#include "tsc_vxn_views.h"
#include "dprmldef.h"

extern struct tsc_vxn_dmac_vmport_database_view dmac_vmport_view;
extern struct tsc_vxn_swname_vmport_database_view swname_vmport_view;
extern struct tsc_vxn_swname_unicast_nwport_database_view swname_unicast_nwport_view;
extern struct tsc_vxn_swname_broadcast_nwport_database_view swname_broadcast_nwport_view;
extern struct tsc_vxn_nid_vn_database_view nid_vn_view;

uint32_t tsc_vxn_get_hashval_bymac(uint8_t *mac_p,uint32_t no_of_buckets);

/************************************************************************************
* Function:tsc_vxn_add_vmport_to_portmac_view
* Description:
*   This function adds the view nod to dmac_vmport view. this view maintains
*   the dmac,nid and port_handle info. form crm_port_handle it gets the dmac
*   and adds to the view db.
***********************************************************************************/
int32_t  tsc_vxn_add_vmport_to_portmac_view(uint64_t crm_port_handle,uint32_t  nid)
{
  uint32_t  index,magic;
  int32_t   status = CRM_SUCCESS;
  uint32_t  hashkey;
  uint8_t   status_b,ii;
  uchar8_t* hashobj_p=NULL;
  struct    tsc_vxn_dmac_vmport_view_node *view_node_entry_p=NULL;
  struct    tsc_vxn_dmac_vmport_view_node *view_node_scan_p=NULL;
  struct    crm_port* port_info_p = NULL;
  struct    crm_port* port_scan_info_p = NULL;

  uchar8_t  heap_b;

  CNTLR_RCU_READ_LOCK_TAKE();
    
  status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
  if(status != CRM_SUCCESS)
  {
   OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
   CNTLR_RCU_READ_LOCK_RELEASE();
   return CRM_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nid = %x",nid);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Adding portname = %s",port_info_p->port_name);

  hashkey = tsc_vxn_get_hashval_bymac(port_info_p->vmInfo.vm_port_mac, dmac_vmport_view.no_of_view_node_buckets_g);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "portname = %s",port_info_p->port_name);

  MCHASH_BUCKET_SCAN(dmac_vmport_view.db_viewtbl_p, hashkey, view_node_scan_p, struct tsc_vxn_dmac_vmport_view_node*, TSC_VXN_DMAC_VMPORT_VIEW_NODE_TBL_OFFSET)
  {
    if((nid == view_node_scan_p->nid) && (memcmp(port_info_p->vmInfo.vm_port_mac, view_node_scan_p->dst_mac,6) == 0))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view_node_scan_p->nid = %x",view_node_scan_p->nid);
      
 
      status = crm_get_port_byhandle(view_node_scan_p->port_handle, &port_scan_info_p);
      if(status == CRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "scan node portname = %s",port_scan_info_p->port_name);
        if((strcmp(port_info_p->port_name, port_scan_info_p->port_name) == 0))
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Port view already added");
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
        else
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Duplicate view node "); 
       
          for(ii = 0;ii < 6;ii++)
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," %02x",port_info_p->vmInfo.vm_port_mac[ii]);
          for(ii = 0;ii < 6;ii++)
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," %02x",view_node_scan_p->dst_mac[ii]);
          CNTLR_RCU_READ_LOCK_RELEASE();       
          return CRM_FAILURE;
        }
      }
    }
    continue;
  }
 
  do
  { 
    status = mempool_get_mem_block(dmac_vmport_view.db_viewmempool_p, (uchar8_t**)&view_node_entry_p,&heap_b);
    if(status != MEMPOOL_SUCCESS)
    {
      status = CRM_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port_name:%s",port_info_p->port_name);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "tunnid:%x",nid);
    for(ii = 0;ii < 6;ii++)
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG," %02x",port_info_p->vmInfo.vm_port_mac[ii]);
  
    memcpy(view_node_entry_p->dst_mac,(port_info_p->vmInfo.vm_port_mac),6);
    view_node_entry_p->nid = nid;
    view_node_entry_p->port_handle = crm_port_handle;
    hashobj_p = (uchar8_t *)view_node_entry_p + TSC_VXN_DMAC_VMPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_APPEND_NODE(dmac_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p, index, magic, hashobj_p, status_b);
  
    if(FALSE == status_b)
    {
      status = CRM_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "entered");
    view_node_entry_p->magic = magic;
    view_node_entry_p->index = index;
    view_node_entry_p->heap_b=heap_b;
    status = CRM_SUCCESS;
 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " view node added");
  }while(0);
 
  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}
/************************************************************************************
* Function:tsc_vxn_del_vmport_from_portmac_view
* Description:
*   This function deletes the view node from the dmac_vmport view. 
***********************************************************************************/
int32_t  tsc_vxn_del_vmport_from_portmac_view(uint64_t crm_port_handle, uint32_t  nid)
{ 
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b;
  uint8_t  *port_mac_p=NULL;
  struct   tsc_vxn_dmac_vmport_view_node *view_node_entry_p=NULL;
  struct   crm_port *port_info_p=NULL;
  uchar8_t offset;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }
    port_mac_p = port_info_p->vmInfo.vm_port_mac;
    hashkey = tsc_vxn_get_hashval_bymac(port_mac_p, dmac_vmport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
    offset = TSC_VXN_DMAC_VMPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(dmac_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_dmac_vmport_view_node *, offset)
    {
      if((view_node_entry_p->nid==nid)&&(!memcmp((view_node_entry_p->dst_mac), (port_info_p->vmInfo.vm_port_mac),6)))
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found to delete");
        MCHASH_DELETE_NODE_BY_REF(dmac_vmport_view.db_viewtbl_p, view_node_entry_p->index, view_node_entry_p->magic,struct tsc_vxn_dmac_vmport_view_node *,offset,status_b);
        if(status_b == TRUE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node  deleted successfully from the view db");
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
        else
        {
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_FAILURE;
        }
        break;
      }
    }
  }while(0); 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node not found to delete");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/******************************************************************************
* Function : tsc_vxn_get_vmport_by_dmac_nid
* Description:
*   This function returns the crm_port info based on destination mac and tunnel
*   id form dmac_vmport view.
******************************************************************************/
int32_t  tsc_vxn_get_vmport_by_dmac_nid(uint8_t*  dst_mac_p, 
                                        uint32_t  nid, 
                                        uint64_t* crm_port_handle_p) 
{
  uint32_t hashkey;
  struct   tsc_vxn_dmac_vmport_view_node *view_node_entry_p=NULL;
  uchar8_t  offset;

  if(dst_mac_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Invalid parameters");
    return CRM_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " %x:%x:%x:%x:%x:%x",dst_mac_p[0],dst_mac_p[1],dst_mac_p[2],dst_mac_p[3],dst_mac_p[4],dst_mac_p[5]);  
  
  CNTLR_RCU_READ_LOCK_TAKE();  
  
  hashkey = tsc_vxn_get_hashval_bymac(dst_mac_p, dmac_vmport_view.no_of_view_node_buckets_g);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
  
  offset = TSC_VXN_DMAC_VMPORT_VIEW_NODE_TBL_OFFSET;
  MCHASH_BUCKET_SCAN(dmac_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_dmac_vmport_view_node *, offset)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, " %x:%x:%x:%x:%x:%x",dst_mac_p[0],dst_mac_p[1],dst_mac_p[2],dst_mac_p[3],dst_mac_p[4],dst_mac_p[5]);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "%x",nid);
    if((view_node_entry_p->nid== nid)&&(!memcmp((view_node_entry_p->dst_mac),(dst_mac_p),6)))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found . copy crm port handle");
      *crm_port_handle_p = view_node_entry_p->port_handle;
      CNTLR_RCU_READ_LOCK_RELEASE();
      return CRM_SUCCESS;
    }
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no view node");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/************************************************************************************
* Function:tsc_vxn_add_vmport_to_swname_view
* Description:
*   This function adds the view nod to swname_vmport view. this view maintains
*   the swname,nid and port_handle info. form crm_port_handle it gets the
*   swname  and adds to the view db.
***********************************************************************************/
int32_t tsc_vxn_add_vmport_to_swname_view(uint64_t crm_port_handle,uint32_t  nid)
{
  uint32_t index,magic;
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b;
  uchar8_t *hashobj_p=NULL;
  struct   tsc_vxn_swname_vmport_view_node *view_node_entry_p=NULL;
  struct   crm_port  *port_info_p=NULL;
  uchar8_t  heap_b;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }

    /* TBD  Check for duplicate node with the same swname and nid */

    status = mempool_get_mem_block(swname_vmport_view.db_viewmempool_p, (uchar8_t**)&view_node_entry_p,&heap_b);
    if(status != MEMPOOL_SUCCESS)
    {
      status = CRM_FAILURE;
      break;
    }
    hashkey = tsc_vxn_get_hashval_byname(port_info_p->switch_name, swname_vmport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);

    strcpy(view_node_entry_p->swname, port_info_p->switch_name);
    view_node_entry_p->nid = nid;
    view_node_entry_p->port_handle = crm_port_handle;
    hashobj_p = (uchar8_t *)view_node_entry_p + TSC_VXN_SWNAME_VMPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_APPEND_NODE(swname_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p, index, magic, hashobj_p, status_b);
    if(FALSE == status_b)
    {
      status = CRM_FAILURE;
      break;
    }
    view_node_entry_p->magic = magic;
    view_node_entry_p->index = index;
    view_node_entry_p->heap_b=heap_b;
    status = CRM_SUCCESS;
  }while(0);
  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}
/************************************************************************************
* Function:tsc_vxn_del_vmport_from_swname_view
* Description:
*   This function deletes the view node from the dmac_swname view. 
***********************************************************************************/
int32_t  tsc_vxn_del_vmport_from_swname_view(uint64_t crm_port_handle, uint32_t  nid)
{ 
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b;
  struct   tsc_vxn_swname_vmport_view_node *view_node_entry_p=NULL;
  struct   crm_port  *port_info_p=NULL;
  uchar8_t  offset;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }
    hashkey = tsc_vxn_get_hashval_byname(port_info_p->switch_name, swname_vmport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
      offset = TSC_VXN_SWNAME_VMPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(swname_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_vmport_view_node *, offset)
    {
      if((!strcmp(view_node_entry_p->swname, port_info_p->switch_name))&&(view_node_entry_p->nid== nid))
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found to delete");
          MCHASH_DELETE_NODE_BY_REF(swname_vmport_view.db_viewtbl_p, view_node_entry_p->index, view_node_entry_p->magic,struct tsc_vxn_swname_vmport_view_node *,offset,status_b);
        if(status_b == TRUE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node  deleted successfully from the view db");
            CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
        else
        {
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_FAILURE;
        }
        break;
      }
    }
  }while(0); 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node not found to delete");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/******************************************************************************
* Function : tsc_vxn_get_vmports_by_swname_nid
* Description:
*   This function  based on switch name and tunnel
*   id from dmac_vmport view gives the no of vmsideports and ids
******************************************************************************/
int32_t tsc_vxn_get_vmports_by_swname_nid(char *switch_name_p,
                                      uint32_t  nid,
                                      uint32_t* no_of_vmside_ports_p,
                                      uint32_t* vmside_ports_p)
{
  int32_t  status=OF_SUCCESS;
  uint32_t hashkey;
  struct   tsc_vxn_swname_vmport_view_node *view_node_entry_p=NULL;
  struct   crm_port  *port_info_p=NULL;
  uchar8_t  offset;
  uint8_t  index=0,no_of_vmside_ports=0;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "switch_name_p:%s,no_of_vmside_ports:%d,vmside_ports_p:%x",switch_name_p,*no_of_vmside_ports_p,vmside_ports_p);

  if((switch_name_p ==  NULL)||((*no_of_vmside_ports_p)==0)||(vmside_ports_p==NULL))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Invalid parameters");
    return CRM_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_TAKE();  
  hashkey = tsc_vxn_get_hashval_byname(switch_name_p, swname_vmport_view.no_of_view_node_buckets_g);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
  offset = TSC_VXN_SWNAME_VMPORT_VIEW_NODE_TBL_OFFSET;
  MCHASH_BUCKET_SCAN(swname_vmport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_vmport_view_node *, offset)
  {
    if(!strcmp(view_node_entry_p->swname, switch_name_p)&&(view_node_entry_p->nid== nid))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found . copy crm port info");
      status = crm_get_port_byhandle(view_node_entry_p->port_handle, &port_info_p);
      if(status!=CRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
        break;
      }
      if((no_of_vmside_ports<=(*no_of_vmside_ports_p)) && (port_info_p->crm_port_type != VM_GUEST_PORT))
      {
        no_of_vmside_ports++;
        vmside_ports_p[index]=port_info_p->port_id;
        index++;
      }
      else
      {
        break;
      }
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port_name:%s,port_type:%d,vm_name:%s,max:%s,br_name:%s",port_info_p->port_name,port_info_p->crm_port_type,port_info_p->vmInfo.vm_name,port_info_p->vmInfo.vm_port_mac,port_info_p->br1_name);
    }
  }
  if(no_of_vmside_ports>0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no of view nodes found:%d",no_of_vmside_ports);
    *no_of_vmside_ports_p = no_of_vmside_ports;
    CNTLR_RCU_READ_LOCK_RELEASE();
    return CRM_SUCCESS;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no view node");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/************************************************************************************
* Function:tsc_vxn_add_nwport_to_unicast_port_view
* Description:
*   This function adds the view node to swname_network unicast seriveport view.
*   this view maintains the swname,service port and port_handle info. from crm_port_handle
*   it gets the swname  and adds to the view db.
***********************************************************************************/
int32_t  tsc_vxn_add_unicast_nwport_to_swname_view(uint64_t crm_port_handle, uint16_t  service_port)
{
  uint32_t index,magic;
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b = FALSE;
  uchar8_t *hashobj_p=NULL;
  struct   tsc_vxn_swname_unicast_nwport_view_node *view_node_entry_p=NULL;
  struct   tsc_vxn_swname_unicast_nwport_view_node *view_node_scan_p=NULL;
  struct   crm_port* port_info_p = NULL;
  struct   crm_port* port_scan_info_p = NULL;
  uchar8_t  heap_b;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }

    hashkey = tsc_vxn_get_hashval_byname(port_info_p->switch_name, swname_unicast_nwport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "portname = %s",port_info_p->port_name);
    /** Scan  hash table for the name */

    MCHASH_BUCKET_SCAN(swname_unicast_nwport_view.db_viewtbl_p, hashkey, view_node_scan_p, struct tsc_vxn_swname_unicast_nwport_view_node*, TSC_VXN_SWNAME_UNICAST_NWPORT_VIEW_NODE_TBL_OFFSET)
    {
      if((service_port == (view_node_scan_p->service_port)) &&
         (!strcmp(port_info_p->switch_name,view_node_scan_p->swname))
        )
      {
        status = crm_get_port_byhandle(view_node_scan_p->port_handle, &port_scan_info_p);
        if(status == CRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "scan node portname = %s",port_scan_info_p->port_name);
          if((strcmp(port_info_p->port_name, port_scan_info_p->port_name) == 0))
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Port view already added");
            CNTLR_RCU_READ_LOCK_RELEASE();
            return CRM_SUCCESS;
          }
          else
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Duplicate view node ");
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port switch name = %s",port_info_p->switch_name); 
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port scan switch name = %s",port_scan_info_p->switch_name); 
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "service port = %d",service_port); 
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port scan service port = %d",view_node_scan_p->service_port); 
            CNTLR_RCU_READ_LOCK_RELEASE();
            return CRM_FAILURE;
          }
        }
      }
      continue;
    }

    status = mempool_get_mem_block(swname_unicast_nwport_view.db_viewmempool_p, (uchar8_t**)&view_node_entry_p,&heap_b);
    if(status != MEMPOOL_SUCCESS)
    {
      status = CRM_FAILURE;
      break;
    }

    strcpy(view_node_entry_p->swname, port_info_p->switch_name);
    view_node_entry_p->service_port = service_port;
    view_node_entry_p->port_handle = crm_port_handle;
    hashobj_p = (uchar8_t *)view_node_entry_p + TSC_VXN_SWNAME_UNICAST_NWPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_APPEND_NODE(swname_unicast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, index, magic, hashobj_p, status_b);

    if(status_b  == FALSE)
    {
      status = CRM_FAILURE;
      break;
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "unicast port added to sw view");
    view_node_entry_p->magic = magic;
    view_node_entry_p->index = index;
    view_node_entry_p->heap_b=heap_b;
    status = CRM_SUCCESS;
  }while(0);
  
  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}
/************************************************************************************
* Function:tsc_vxn_del_nwport_from_unicast_port_view
* Description:
*   This function deletes the view node from the swname-nwserviceport view. 
***********************************************************************************/
int32_t  tsc_vxn_del_unicast_nwport_from_swname_view(uint64_t crm_port_handle, uint16_t  service_port)
{ 
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b;
  struct   tsc_vxn_swname_unicast_nwport_view_node *view_node_entry_p=NULL;
  struct   crm_port  *port_info_p=NULL;
  uchar8_t  offset;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }
    hashkey = tsc_vxn_get_hashval_byname(port_info_p->switch_name, swname_unicast_nwport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
    offset = TSC_VXN_SWNAME_UNICAST_NWPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(swname_unicast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_unicast_nwport_view_node *, offset)
    {
      if((view_node_entry_p->service_port== service_port)&&
          (!strcmp(view_node_entry_p->swname, port_info_p->switch_name)))
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found to delete");
          MCHASH_DELETE_NODE_BY_REF(swname_unicast_nwport_view.db_viewtbl_p, view_node_entry_p->index, view_node_entry_p->magic,struct tsc_vxn_unicast_nwport_view_node *,offset,status_b);
        if(status_b == TRUE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node  deleted successfully from the view db");
            CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
        else
        {
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_FAILURE;
        }
        break;
      }
    }
  }while(0); 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node not found to delete");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/******************************************************************************
* Function : tsc_vxn_get_unicast_nwport_by_swname_sp
* Description:
*   This function  based on switch name and service port
*   from swname_network unicast view gives the port id
******************************************************************************/
int32_t  tsc_vxn_get_unicast_nwport_by_swname_sp(char* switch_name_p, uint16_t serviceport, uint32_t* out_port_no_p)
{
  int32_t  status = OF_SUCCESS;
  uint32_t hashkey;
  struct   tsc_vxn_swname_unicast_nwport_view_node* view_node_entry_p = NULL;
  struct   crm_port* port_info_p = NULL;
  uchar8_t offset;

  /* caller shall take RCU Locks */

  if((switch_name_p ==  NULL)||(out_port_no_p==NULL))
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Invalid parameters");
    return CRM_FAILURE;
  }

  // CNTLR_RCU_READ_LOCK_TAKE();  
  hashkey = tsc_vxn_get_hashval_byname(switch_name_p, swname_vmport_view.no_of_view_node_buckets_g);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
  offset = TSC_VXN_SWNAME_UNICAST_NWPORT_VIEW_NODE_TBL_OFFSET;
  MCHASH_BUCKET_SCAN(swname_unicast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_unicast_nwport_view_node *, offset)
  {
    if((view_node_entry_p->service_port == serviceport)&&
       (!strcmp(view_node_entry_p->swname, switch_name_p)))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found . copy crm port info");
      status = crm_get_port_byhandle(view_node_entry_p->port_handle, &port_info_p);
      if(status!=CRM_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
        break;
      }
       (*out_port_no_p)=port_info_p->port_id;
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port_name:%s,port_type:%d,vm_name:%s,max:%s,br_name:%s,portd:%d",port_info_p->port_name,port_info_p->crm_port_type,port_info_p->vmInfo.vm_name,port_info_p->vmInfo.vm_port_mac,port_info_p->br1_name,port_info_p->port_id);

    //CNTLR_RCU_READ_LOCK_RELEASE();
    return CRM_SUCCESS;
    }
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no view node");
  //CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/************************************************************************************
* Function:tsc_vxn_add_nwport_to_broadcast_port_view
* Description:
*   This function adds the view node to swname_network broadcast seriveport view.
*   this view maintains the swname,service port and port_handle info. from crm_port_handle
*   it gets the swname  and adds to the view db.
**********************************************************************************/
int32_t tsc_vxn_add_broadcast_nwport_to_swname_view(uint64_t crm_port_handle, 
                                                    uint16_t service_port, 
                                                    uint32_t nid,
                                                    uint32_t remote_ip)

{
  uint32_t index,magic;
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b = FALSE;
  uchar8_t *hashobj_p=NULL;
  struct   tsc_vxn_swname_broadcast_nwport_view_node *view_node_entry_p=NULL;
  struct   tsc_vxn_swname_broadcast_nwport_view_node *view_node_scan_p=NULL;
  struct   crm_port* port_info_p=NULL;
  struct   crm_port* port_scan_info_p=NULL;
  uchar8_t  heap_b;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "entered");
  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }

    hashkey = tsc_vxn_get_hashval_byname(port_info_p->switch_name, swname_broadcast_nwport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);

    /** Scan  hash table for the name */
    MCHASH_BUCKET_SCAN(swname_broadcast_nwport_view.db_viewtbl_p, hashkey, view_node_scan_p, struct tsc_vxn_swname_broadcast_nwport_view_node*, TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET)
    {
      if((service_port == (view_node_scan_p->serviceport)) &&
         (nid == view_node_scan_p->nid)&& 
         (remote_ip == view_node_scan_p->remote_ip)&&
         (!strcmp(port_info_p->switch_name,view_node_scan_p->swname))
        )
      {
        status = crm_get_port_byhandle(view_node_scan_p->port_handle, &port_scan_info_p);
        if(status == CRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "scan node portname = %s",port_scan_info_p->port_name);
          if((strcmp(port_info_p->port_name, port_scan_info_p->port_name) == 0))
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Port view already added");
            CNTLR_RCU_READ_LOCK_RELEASE();
            return CRM_SUCCESS;
          }
          else
          {
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Duplicate view node ");
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port switch name = %s",port_info_p->switch_name);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port scan switch name = %s",port_scan_info_p->switch_name);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port service port = %d",service_port);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port scan service port = %d",view_node_scan_p->serviceport);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "nid = %d",nid);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port scan nid = %d",view_node_scan_p->nid);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port remote_ip = %d",remote_ip);
            OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port scan remote_ip = %d",view_node_scan_p->remote_ip);
            CNTLR_RCU_READ_LOCK_RELEASE();
            return CRM_FAILURE;
          }
        }
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Duplicate view node ");
        CNTLR_RCU_READ_LOCK_RELEASE();
        return CRM_FAILURE;
      }
      continue;
    }

    status = mempool_get_mem_block(swname_broadcast_nwport_view.db_viewmempool_p, (uchar8_t**)&view_node_entry_p,&heap_b);
    if(status != MEMPOOL_SUCCESS)
    {
      status = CRM_FAILURE;
      break;
    }

    strcpy(view_node_entry_p->swname, port_info_p->switch_name);
    view_node_entry_p->nid= nid;
    view_node_entry_p->serviceport = service_port;
    view_node_entry_p->remote_ip = remote_ip;
    view_node_entry_p->port_handle = crm_port_handle;
    hashobj_p = (uchar8_t *)view_node_entry_p + TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_APPEND_NODE(swname_broadcast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, index, magic, hashobj_p, status_b);
    if(status_b == FALSE)
    {
      status = CRM_FAILURE;
      break;
    }
    view_node_entry_p->magic = magic;
    view_node_entry_p->index = index;
    view_node_entry_p->heap_b=heap_b;
    status = CRM_SUCCESS;
  }while(0);
  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}
/************************************************************************************
* Function:tsc_vxn_del_nwport_from_broadcast_port_view
* Description:
*   This function deletes the view node from the swname-nwserviceport view. 
***********************************************************************************/
int32_t tsc_vxn_del_broadcast_nwport_from_swname_view(uint64_t crm_port_handle,uint16_t service_port,uint32_t nid,uint32_t remote_ip)
{ 
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b;
  struct   tsc_vxn_swname_broadcast_nwport_view_node *view_node_entry_p=NULL;
  struct   crm_port  *port_info_p=NULL;
  uchar8_t  offset;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = crm_get_port_byhandle(crm_port_handle, &port_info_p);
    if(status!=CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
      break;
    }
    hashkey = tsc_vxn_get_hashval_byname(port_info_p->switch_name, swname_broadcast_nwport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
    offset = TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(swname_broadcast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_broadcast_nwport_view_node *, offset)
    {
      if((view_node_entry_p->serviceport== service_port) &&
          (view_node_entry_p->remote_ip == remote_ip)&&
          (view_node_entry_p->nid == nid)&&
          (!strcmp(view_node_entry_p->swname, port_info_p->switch_name)))
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found to delete");
          MCHASH_DELETE_NODE_BY_REF(swname_broadcast_nwport_view.db_viewtbl_p, view_node_entry_p->index, view_node_entry_p->magic,struct tsc_vxn_broadcast_nwport_view_node *,offset,status_b);
        if(status_b == TRUE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node  deleted successfully from the view db");
            CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
        else
        {
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_FAILURE;
        }
        break;
      }
    }
  }while(0); 
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node not found to delete");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/*****************************************************************************************
 * Function :tsc_vxn_get_remote_ips_by_swname_nid_sp
 * Description:
 * This function is  based on switch name and service port
 * from swname_network broadcast view returns remote ips
 ****************************************************************************************/
int32_t tsc_vxn_get_remote_ips_by_swname_nid_sp(char* switch_name_p,uint32_t service_port,
                                                uint32_t  nid,
                                                uint32_t* no_of_remote_ips_p,
                                                uint32_t* remote_ips_p)
{
  uint32_t hashkey;
  struct   tsc_vxn_swname_broadcast_nwport_view_node *view_node_entry_p = NULL;
  uchar8_t offset;
  uint8_t  index=0,no_of_remote_ips = 0;

  /* Caller shall take RCU Locks */
  if(switch_name_p ==  NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Invalid Switch Name");
    return CRM_FAILURE;
  }
  
  if(remote_ips_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Invalid remote_ips_p");
    return CRM_FAILURE;
  }

  hashkey = tsc_vxn_get_hashval_byname(switch_name_p, swname_vmport_view.no_of_view_node_buckets_g);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
  offset = TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET;

  MCHASH_BUCKET_SCAN(swname_broadcast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_broadcast_nwport_view_node *, offset)
  {
    if((!strcmp(view_node_entry_p->swname, switch_name_p))&&
       (view_node_entry_p->serviceport == service_port)&&
       (view_node_entry_p->nid == nid))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found . copy remote_ip info");
      if(no_of_remote_ips <= (*no_of_remote_ips_p))
      {
        no_of_remote_ips++;
        remote_ips_p[index] = view_node_entry_p->remote_ip;
        index++;
      }
      else
      {
        break;
      }
    }
  }                      
  if(no_of_remote_ips > 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no of view nodes found:%d",no_of_remote_ips);
    *no_of_remote_ips_p = no_of_remote_ips;
    return CRM_SUCCESS;
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no view node");
  return CRM_FAILURE;
}    
/*****************************************************************************************
* Function : tsc_vxn_get_broadcast_nwports_by_swname_nid_sp
* Description:
*   This function is based on switch name and service port
*   from swname_network broadcast view returns port ids
*   if input remote_ip is non zero it is also matched and returns a single matching port
*****************************************************************************************/
int32_t tsc_vxn_get_broadcast_nwports_by_swname_nid_sp(char* switch_name_p,uint32_t service_port,
                                                       uint32_t  nid,
                                                       uint32_t* no_of_nwside_br_ports_p,
                                                       uint32_t* nwside_br_ports_p,
                                                       uint32_t remote_ip)
{
  int32_t  status=OF_SUCCESS;
  uint32_t hashkey;
  struct   tsc_vxn_swname_broadcast_nwport_view_node *view_node_entry_p = NULL;
  struct   crm_port  *port_info_p = NULL;
  uchar8_t offset;
  uint8_t  index=0,no_of_nwside_br_ports=0;

  /* Caller shall take RCU Locks */

  if(switch_name_p ==  NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Invalid Switch Name");
    return CRM_FAILURE;
  }
  
  if(nwside_br_ports_p == NULL)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Invalid nwside_br_ports");
    return CRM_FAILURE;
  }
  //CNTLR_RCU_READ_LOCK_TAKE();  

  /* For Broadcast ports we use nid = 0 , otherwise we may need to obtain nid from vn_entry. */
  /* While adding to view, we add nid = 0 */
 
  hashkey = tsc_vxn_get_hashval_byname(switch_name_p, swname_vmport_view.no_of_view_node_buckets_g);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
  offset = TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET;

  if(remote_ip == 0)
  {
    MCHASH_BUCKET_SCAN(swname_broadcast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_broadcast_nwport_view_node *, offset)
    {
      if((!strcmp(view_node_entry_p->swname, switch_name_p))&&
          (view_node_entry_p->serviceport == service_port)&&
          (view_node_entry_p->nid == nid))
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found . copy crm port info");
        status = crm_get_port_byhandle(view_node_entry_p->port_handle, &port_info_p);
        if(status!=CRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
          break;
        }
        if(no_of_nwside_br_ports <= (*no_of_nwside_br_ports_p))
        {
          no_of_nwside_br_ports++;
          nwside_br_ports_p[index] = port_info_p->port_id;
          index++;
        }
        else
        {
          break;
        }
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port_name:%s,port_type:%d,vm_name:%s,max:%s,br_name:%s,portd:%d",port_info_p->port_name,port_info_p->crm_port_type,port_info_p->vmInfo.vm_name,port_info_p->vmInfo.vm_port_mac,port_info_p->br1_name,port_info_p->port_id);
      }
    }
  } 
  else
  {
    hashkey = tsc_vxn_get_hashval_byname(switch_name_p, swname_vmport_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
    offset = TSC_VXN_SWNAME_BROADCAST_NWPORT_VIEW_NODE_TBL_OFFSET;

    MCHASH_BUCKET_SCAN(swname_broadcast_nwport_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_swname_broadcast_nwport_view_node *, offset)
    {
      if((!strcmp(view_node_entry_p->swname, switch_name_p))&&
                 (view_node_entry_p->serviceport == service_port)&&
                 (view_node_entry_p->nid == nid)&& 
                 (view_node_entry_p->remote_ip == remote_ip))
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found . copy crm port info");
        status = crm_get_port_byhandle(view_node_entry_p->port_handle, &port_info_p);
        if(status!=CRM_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Failed to get crm port information");
          break;
        }
        if(no_of_nwside_br_ports <= (*no_of_nwside_br_ports_p))
        {
          no_of_nwside_br_ports++;
          nwside_br_ports_p[index] = port_info_p->port_id;
          index++;
        }
        else
        {
          break;
        }
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "port_name:%s,port_type:%d,vm_name:%s,max:%s,br_name:%s,portd:%d",port_info_p->port_name,port_info_p->crm_port_type,port_info_p->vmInfo.vm_name,port_info_p->vmInfo.vm_port_mac,port_info_p->br1_name,port_info_p->port_id);
      }
    }
  }    
  if(no_of_nwside_br_ports > 0)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no of view nodes found:%d",no_of_nwside_br_ports);
    *no_of_nwside_br_ports_p = no_of_nwside_br_ports;
    return CRM_SUCCESS;
  }  
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no view node");
  return CRM_FAILURE;
}
/****************************************************************************************/
uint32_t tsc_vxn_get_hashval_bymac(uint8_t *mac_p,uint32_t no_of_buckets)
{
  uint32_t hashkey,param1,param2,param3,param4;

  param1 = (uint32_t)(mac_p[0]);
  param2 = (uint32_t)(mac_p[1]);
  param3 = (uint32_t)(*((uint32_t *)(&mac_p[2])));
  param4 = 1;
  DPRM_COMPUTE_HASH(param1,param2,param3,param4,no_of_buckets,hashkey);
  return hashkey;
}
uint32_t tsc_vxn_get_hashval_byname(char* name_p,uint32_t no_of_buckets)
{
  uint32_t hashkey,param1,param2,param3,param4;
  char name_hash[16];
  memset(name_hash,0,16);
  strncpy(name_hash,name_p,16);
  param1 = *(uint32_t *)(name_hash +0);
  param2 = *(uint32_t *)(name_hash +4);
  param3 = *(uint32_t *)(name_hash +8);
  param4 = *(uint32_t *)(name_hash +12);
  CRM_COMPUTE_HASH(param1,param2,param3,param4,no_of_buckets,hashkey);
  return hashkey;
}
uint32_t tsc_vxn_get_hashval_by_nid(uint32_t nid,uint32_t no_of_buckets)
{
  uint32_t hashkey,param1,param2,param3,param4;

  param1 = nid;
  param2 = nid;
  param3 = 1;
  param4 = 1;
  DPRM_COMPUTE_HASH(param1,param2,param3,param4,no_of_buckets,hashkey);
  return hashkey;
}
/************************************************************************************
* Function:tsc_vxn_add_vn_to_nid_view
* Description:
*   This function adds the view nod to nid_vn view. this view maintains
*   the nid and vn_handle info. 
***********************************************************************************/
int32_t tsc_vxn_add_vn_to_nid_view(uint64_t crm_vn_handle,uint32_t nid)
{
  uint32_t index,magic;
  int32_t  status=CRM_SUCCESS;
  uint32_t hashkey;
  uint8_t  status_b;
  uchar8_t *hashobj_p=NULL;
  struct   tsc_vxn_nid_vn_view_node *view_node_entry_p=NULL;
  uchar8_t heap_b;


  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Entered add vn to nid view");
  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    status = mempool_get_mem_block(nid_vn_view.db_viewmempool_p, (uchar8_t**)&view_node_entry_p,&heap_b);
    if(status != MEMPOOL_SUCCESS)
    {
      status = CRM_FAILURE;
      break;
    }
    hashkey = tsc_vxn_get_hashval_by_nid(nid,nid_vn_view.no_of_view_node_buckets_g);
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);

    view_node_entry_p->nid = nid;
    view_node_entry_p->vn_handle = crm_vn_handle;
    hashobj_p = (uchar8_t *)view_node_entry_p + TSC_VXN_TUNID_VN_VIEW_NODE_TBL_OFFSET;
    MCHASH_APPEND_NODE(nid_vn_view.db_viewtbl_p, hashkey, view_node_entry_p, index, magic, hashobj_p, status_b);
    if(FALSE == status_b)
    {
      status = CRM_FAILURE;
      break;
    }
    view_node_entry_p->magic = magic;
    view_node_entry_p->index = index;
    view_node_entry_p->heap_b=heap_b;
 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Successfully added vn to nid view");
    status = CRM_SUCCESS;
  }while(0);
  
  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}
/************************************************************************************
* Function:tsc_vxn_delete_vn_from_nid_view
* Description:
*   This function deletes the view node from the nid_vn view.
***********************************************************************************/
int32_t  tsc_vxn_delete_vn_from_nid_view(uint64_t crm_vn_handle, uint32_t nid)
{
  uint32_t hashkey;
  uint8_t  status_b;
  struct   tsc_vxn_nid_vn_view_node *view_node_entry_p=NULL;
  uchar8_t offset;

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "entered");

  CNTLR_RCU_READ_LOCK_TAKE();

  do
  {
    hashkey = tsc_vxn_get_hashval_by_nid(nid,nid_vn_view.no_of_view_node_buckets_g);
    
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "hashkey = %d",hashkey);
    offset = TSC_VXN_TUNID_VN_VIEW_NODE_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(nid_vn_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_nid_vn_view_node *, offset)
    {
      if(view_node_entry_p->nid == nid)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node found to delete");
        MCHASH_DELETE_NODE_BY_REF(nid_vn_view.db_viewtbl_p, view_node_entry_p->index, view_node_entry_p->magic,struct tsc_vxn_nid_vn_view_node *,offset,status_b);
        if(status_b == TRUE)
        {
          OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node  deleted successfully from the view db");
            CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
        else
        {
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_FAILURE;
        }
        break;
      }
    }
  }while(0);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "view node not found to delete");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/******************************************************************************
* Function : tsc_vxn_get_vnhandle_by_nid
* Description:
******************************************************************************/
int32_t tsc_vxn_get_vnhandle_by_nid(uint32_t nid,uint64_t* vn_handle_p)
{
  uint32_t hashkey;
  struct   tsc_vxn_nid_vn_view_node *view_node_entry_p = NULL;
  uchar8_t offset;

  /* NSM TBD The locks need to be removed as the caller takse care of locks */
  
  CNTLR_RCU_READ_LOCK_TAKE();

  hashkey = tsc_vxn_get_hashval_by_nid(nid,nid_vn_view.no_of_view_node_buckets_g);

  offset = TSC_VXN_TUNID_VN_VIEW_NODE_TBL_OFFSET;

  MCHASH_BUCKET_SCAN(nid_vn_view.db_viewtbl_p, hashkey, view_node_entry_p, struct tsc_vxn_nid_vn_view_node *, offset)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "%x",nid);
    if(view_node_entry_p->nid == nid)
    {
      *vn_handle_p = view_node_entry_p->vn_handle;
      CNTLR_RCU_READ_LOCK_RELEASE();
      return CRM_SUCCESS;
    }
  }
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "no vxlan-vn view node found for nid = %x",nid);
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/******************************************************************************/ 
