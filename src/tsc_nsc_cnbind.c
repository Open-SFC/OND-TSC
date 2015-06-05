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

/* File: tsc_nsc_cnbind.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file contains connection bind entry implementation.
 */

#include "controller.h"
#include "dobhash.h"
#include "cntrlappcmn.h"
#include "dprm.h"
#include "crmapi.h"
#include "crm_vn_api.h"  
#include "crm_port_api.h"
#include "tsc_controller.h"
#include "tsc_nvm_modules.h"

extern uint32_t vn_nsc_info_offset_g;
extern struct   tsc_nvm_module_interface tsc_nvm_modules[SUPPORTED_NW_TYPE_MAX + 1];
extern uint32_t nsc_no_of_cnbind_table_buckets_g;

extern  cntlr_lock_t global_mac_lock;
uint8_t local_in_mac[6]  = {02,01,01,01,01,02};
uint8_t local_out_mac[6] = {06,01,01,01,01,02};

/*****************************************************************************************************************************************
Function: nsc_get_cnbind_entry
Input Parameters: 
	pkt_in:            Miss packet received from a datapath via controller.
	pkt_selector_p:    Contains the selector fields extracted from the packet. 
	nw_type:           VXLAN or NVGRE 
	nid:               Network Identifier (VNI for VXLAN network type)

    cn_bind_node_p_p:  Double pointer to copy the cnbind_entry to be returned.
	selector_type_p:   PRIMARY or SECONDARY SELECTOR that matched with the packet selector fields.
   
Description: 
        1.The input selector fields are searched and matched with the L2 network specific database to find the matching selector.
        2. A. If a matching selector is found,the associated cn_bind entry is returned using the input double pointer.
           B. The type of the selector that matched is also returned.Type can PRIMARY or SECONDARY.
        3. A. If a matching selector is not found, the primary and the secondary selectors are extracted from the packet.
           B. Memory is allocated for the cn_bind entry which includes the two selectors.
           C. Primary selector type is returned along with the cn_bind entry created. 
*****************************************************************************************************************************************/
int32_t nsc_get_cnbind_entry(struct   ofl_packet_in* pkt_in,
                             struct   nsc_selector_node* pkt_selector_p,
                             uint8_t  nw_type,
                             uint32_t nid,
                             struct   l2_connection_to_nsinfo_bind_node** cn_bind_node_p_p,
                             uint8_t  *selector_type_p,
                             uint8_t  copy_original_mac_b
                            )
{
  struct l2_connection_to_nsinfo_bind_node *cn_bind_node_entry_p;
  struct vn_service_chaining_info* vn_nsc_info_p;
  struct crm_virtual_network*      vn_entry_p;

  uint64_t vn_handle;
  uint32_t hashmask,hashkey,offset;
  uint8_t  entry_found = 0,ii;
  int32_t  retval;
  uint8_t  heap_b;
  struct   nsc_selector_node *selector_p,*selector2_p,*selector_node_scan_p;

  hashmask = nsc_no_of_cnbind_table_buckets_g;

  hashkey  = NSC_COMPUTE_HASH_CNBIND_ENTRY(pkt_selector_p->src_ip,pkt_selector_p->dst_ip,
                                           pkt_selector_p->src_port,pkt_selector_p->dst_port,
                                           pkt_selector_p->protocol,
                                           pkt_selector_p->ethernet_type,
                                           hashmask
                                          );
  
  CNTLR_RCU_READ_LOCK_TAKE();

  retval = tsc_nvm_modules[nw_type].nvm_module_get_vnhandle_by_nid(nid,&vn_handle);
  if(retval == OF_SUCCESS)
    retval = crm_get_vn_byhandle(vn_handle,&vn_entry_p);

  if(retval != OF_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return OF_FAILURE;
  }

  vn_nsc_info_p = (struct vn_service_chaining_info *)(*(tscaddr_t*)((uint8_t *)vn_entry_p + vn_nsc_info_offset_g));  /* add offset to vn addr to fetch service chaining info */

  retval = mempool_get_mem_block(vn_nsc_info_p->nsc_cn_bind_mempool_g,(uchar8_t **)&cn_bind_node_entry_p,&heap_b);
  if(retval != MEMPOOL_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE(); 
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"Failed to create mempool entry for cn_bind_node_entry");
    return OF_FAILURE;
  }
  
  cn_bind_node_entry_p->heap_b = heap_b;
  offset = NSC_SELECTOR_NODE_OFFSET;
  MCHASH_BUCKET_SCAN(vn_nsc_info_p->l2_connection_to_nsinfo_bind_table_p,hashkey,selector_node_scan_p,struct nsc_selector_node *, offset)
  {
     if((selector_node_scan_p->cnbind_node_p->stale_entry == 1) || (selector_node_scan_p->cnbind_node_p->skip_this_entry == 1)) 
       continue;  
    
     if((pkt_selector_p->src_ip   != selector_node_scan_p->src_ip )  ||
        (pkt_selector_p->dst_ip   != selector_node_scan_p->dst_ip )  ||
        (pkt_selector_p->protocol != selector_node_scan_p->protocol) ||
        (pkt_selector_p->src_port != selector_node_scan_p->src_port) ||
        (pkt_selector_p->dst_port != selector_node_scan_p->dst_port) ||

        (pkt_selector_p->ethernet_type != selector_node_scan_p->ethernet_type)
     )
     {
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSM Selector is not matching. selector type 1:PRIMARY 2:SECONDARY  = %d",selector_node_scan_p->selector_type);
       continue;
     }
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"NSM A matching Selector is found. selector type 1:PRIMARY 2:SECONDARY  = %d",selector_node_scan_p->selector_type);
    
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_ip       = %x,%x",pkt_selector_p->src_ip,selector_node_scan_p->src_ip);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_ip       = %x,%x",pkt_selector_p->dst_ip,selector_node_scan_p->dst_ip);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_protocol = %x,%x",pkt_selector_p->protocol,selector_node_scan_p->protocol);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_port     = %x,%x",pkt_selector_p->src_port,selector_node_scan_p->src_port);
     OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_port     = %x,%x",pkt_selector_p->dst_port,selector_node_scan_p->dst_port);
 
     /* The presence of other selector is obvious as both get created at once. */
     *selector_type_p   = selector_node_scan_p->selector_type; /* PRIMARY or SECONDARY */
     *cn_bind_node_p_p  = selector_node_scan_p->cnbind_node_p;
     entry_found = 1;
     //printf("\r\n In nsc_get_cnbind_entry and found a matching selector"); 
     if(copy_original_mac_b == TRUE)
     {
       pkt_selector_p->src_mac_low    = (selector_node_scan_p->src_mac_low);
       pkt_selector_p->src_mac_high   = (selector_node_scan_p->src_mac_high);

       pkt_selector_p->dst_mac_low    = (selector_node_scan_p->dst_mac_low);
       pkt_selector_p->dst_mac_high   = (selector_node_scan_p->dst_mac_high);

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac_low  = %x",selector_node_scan_p->src_mac_low);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac_high = %x",selector_node_scan_p->src_mac_high);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_low  = %x",selector_node_scan_p->dst_mac_low);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_high = %x",selector_node_scan_p->dst_mac_high);

       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac_low  = %x",pkt_selector_p->src_mac_low);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac_high = %x",pkt_selector_p->src_mac_high);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_low  = %x",pkt_selector_p->dst_mac_low);
       OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_high = %x",pkt_selector_p->dst_mac_high);

       for(ii=0;ii<6;ii++)
       {
         pkt_selector_p->src_mac[ii] = selector_node_scan_p->src_mac[ii];
         pkt_selector_p->dst_mac[ii] = selector_node_scan_p->dst_mac[ii];
       }
    
       for(ii=0;ii<6;ii++)
       {
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_mac = %x",pkt_selector_p->src_mac[ii]);
         OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac = %x",pkt_selector_p->dst_mac[ii]);
       }
     }
  }

  CNTLR_RCU_READ_LOCK_RELEASE();

  if(entry_found == 1)
  {
    pkt_selector_p->magic  = selector_node_scan_p->magic;
    pkt_selector_p->index  = selector_node_scan_p->index;
    pkt_selector_p->safe_reference = selector_node_scan_p->safe_reference;
    pkt_selector_p->cnbind_node_p  = selector_node_scan_p->cnbind_node_p;

    mempool_release_mem_block(vn_nsc_info_p->nsc_cn_bind_mempool_g,(uchar8_t*)cn_bind_node_entry_p,cn_bind_node_entry_p->heap_b);
    return NSC_CONNECTION_BIND_ENTRY_FOUND;
  }
  else
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No matching Selector is found.Populating the second selector also.");
    //printf("\r\n In nsc_get_cnbind_entry and NOT found a matching selector,creating selectors");
    CNTLR_LOCK_INIT(cn_bind_node_entry_p->cnbind_node_lock); 

    cn_bind_node_entry_p->cnbind_sel_refcnt = 0;   /* To release cnbind entry */
    cn_bind_node_entry_p->no_of_flow_entries_deduced = 0;
    cn_bind_node_entry_p->nsc_cn_bind_mempool_g = vn_nsc_info_p->nsc_cn_bind_mempool_g; /* For deletion purpose */

    selector_p   = &(cn_bind_node_entry_p->selector_1);
    *selector_p  = *pkt_selector_p;
    selector_p->selector_type = *selector_type_p = (uint8_t)SELECTOR_PRIMARY;
    selector_p->hashkey       = hashkey;
    selector_p->cnbind_node_p = cn_bind_node_entry_p;
    selector_p->cnbind_node_p->stale_entry = 0;
    selector_p->cnbind_node_p->skip_this_entry = 0;
    
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"selector_type : %d",selector_p->selector_type);
   
    /* prepare the other selector */
    selector2_p = &(cn_bind_node_entry_p->selector_2);
    nsc_extract_packet_fields(pkt_in,selector2_p,PACKET_REVERSE,TRUE);

    if((selector_p->protocol == OF_IPPROTO_ICMP) && (selector_p->src_port == 0x08) && (selector_p->dst_port == 0x00))
    {
      selector2_p->src_port = 0x00;
      selector2_p->dst_port = 0x00;
    }
    
    hashmask = nsc_no_of_cnbind_table_buckets_g;
    /* Fields in this selector are extracted in reverse order */
    hashkey  = NSC_COMPUTE_HASH_CNBIND_ENTRY(selector2_p->src_ip,selector2_p->dst_ip,
                                             selector2_p->src_port,selector2_p->dst_port,
                                             selector2_p->protocol,
                                             selector2_p->ethernet_type,
                                             hashmask
                                           );

    selector2_p->selector_type = (uint8_t)SELECTOR_SECONDARY;
    selector2_p->hashkey = hashkey;
    *cn_bind_node_p_p = cn_bind_node_entry_p;  /* Selectors are not yet added to hash table */
    selector2_p->cnbind_node_p = cn_bind_node_entry_p;
    selector_p->other_selector_p  = selector2_p;
    selector2_p->other_selector_p = selector_p;

    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"selector_type : %d",selector2_p->selector_type);
    
    CNTLR_LOCK_TAKE(global_mac_lock);
    local_in_mac[5]++;
    if(local_in_mac[5] == 0)
      local_in_mac[4]++;
    
    local_out_mac[5]++;
    if(local_out_mac[5] == 0)
      local_out_mac[4]++;
    CNTLR_LOCK_RELEASE(global_mac_lock);

    for(ii=0;ii<6;ii++)
    {
      cn_bind_node_entry_p->local_in_mac[ii]  = local_in_mac[ii];
      cn_bind_node_entry_p->local_out_mac[ii] = local_out_mac[ii];
    }
    
    for(ii=0;ii<6;ii++)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_mac = %x",cn_bind_node_entry_p->local_out_mac[ii]);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"local_mac = %x",cn_bind_node_entry_p->local_in_mac[ii]);
    }
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"No existing selector is matched.Both the Selectors are filled but not added to the hash table");

    return NSC_CONNECTION_BIND_ENTRY_CREATED;
  }
}
/*****************************************************************************************************************************
Function: nsc_add_selectors
Input Parameters:
        l2_connection_to_nsinfo_bind_table_p: Hash Table to which the selectors are to be added. 
        nsc_cn_bind_mempool_g: Mempool from which the cnbind entry is to be deleted if the selectors addition fails.
        cn_bind_entry_p: The two selectors to be added are members of this cnbind entry.
Description:
        1.After a new cnbind entry is created, this function is called to add the two associated selectors to the hashtable
          defined for the L2 network.
****************************************************************************************************************************/
int32_t nsc_add_selectors(struct mchash_table* l2_connection_to_nsinfo_bind_table_p,
                          void* nsc_cn_bind_mempool_g,                        
                          struct l2_connection_to_nsinfo_bind_node* cn_bind_entry_p)
{
  struct nsc_selector_node *selector1_p,*selector2_p;
  uchar8_t* hashobj_p = NULL;
  uint32_t  magic,index,offset;
  int32_t   retval_1 = OF_SUCCESS;
  int32_t   retval_2 = OF_SUCCESS;
  uint8_t   status_b  = FALSE;

  do
  { 
    /* Add selector 1 to the hash table. */
    selector1_p  =  &(cn_bind_entry_p->selector_1);
    offset = NSC_SELECTOR_NODE_OFFSET;
    hashobj_p = (uchar8_t *)selector1_p + offset;
    
    MCHASH_APPEND_NODE(l2_connection_to_nsinfo_bind_table_p,selector1_p->hashkey,selector1_p,index,magic,hashobj_p,status_b);
    if(status_b == FALSE)
    {
      retval_1 = NSC_FAILED_TO_ADD_SELECTOR; 
      break;
    }
    selector1_p->magic = magic;
    selector1_p->index = index;
   
    (selector1_p->safe_reference) = magic;
    (selector1_p->safe_reference) = (((selector1_p->safe_reference) << 32) | (index));

    /* Add selector 2 to the hash table  */
    selector2_p = &(cn_bind_entry_p->selector_2);
    hashobj_p = (uchar8_t *)selector2_p + NSC_SELECTOR_NODE_OFFSET; 
    MCHASH_APPEND_NODE(l2_connection_to_nsinfo_bind_table_p,selector2_p->hashkey,selector2_p,index,magic,hashobj_p,status_b);
    if(status_b == FALSE)
    {
      retval_2 = NSC_FAILED_TO_ADD_SELECTOR;
      break;
    }
    selector2_p->magic = magic;
    selector2_p->index = index;
    (selector2_p->safe_reference) = magic;
    (selector2_p->safe_reference) = (((selector2_p->safe_reference) << 32) | (index));
  }while(0);

  if(retval_2 == NSC_FAILED_TO_ADD_SELECTOR)
  {
    /* Delete SELECTOR_1 from Hash Table. */
    offset =  NSC_SELECTOR_NODE_OFFSET;
    status_b = FALSE;
    MCHASH_DELETE_NODE_BY_REF(l2_connection_to_nsinfo_bind_table_p,selector1_p->index,selector1_p->magic,
                              struct nsc_selector_node *,offset,status_b);

  }
  if((retval_1 != OF_SUCCESS) || (retval_2 != OF_SUCCESS))
  {
    /* Free cnbind entry */

    /* Free Network Service Info before freeing cnbind entry itself */
    cn_bind_entry_p->nwservice_info.no_of_nwservice_instances = 0;
    if(cn_bind_entry_p->nwservice_info.nw_services_p != NULL)
      free(cn_bind_entry_p->nwservice_info.nw_services_p);

    retval_1 = mempool_release_mem_block(nsc_cn_bind_mempool_g,(uchar8_t*)cn_bind_entry_p,cn_bind_entry_p->heap_b);
    if(retval_1 == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "cnbind entry memory block released successfully!.");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "failed to release cnbind entry memory block");
    }
    retval_1 = OF_FAILURE;
  }
  else
    retval_1 = OF_SUCCESS;

  return retval_1;
}
/******************************************************************************************************************************
Function: nsc_extract_packet_fields
Input Parameters: 
	pkt_in:     Miss packet received from a datapath via controller.
        direction:  PACKET_NORMAL indicates that the packet selector fields shall be extracted as they are in the packet. 
                    PACKET_REVERSE indicates that while copying from the packet, the source and destination fields shall be reversed.                
Output Parametrs: 
	selector_p  The extracted selector fields are copied to this selector argument.

Description:
        1.This function extracts selector fields from the packet into the input argument selector_p.
        2.The extraction is based on the input argument direction as explained.
******************************************************************************************************************************/
int32_t nsc_extract_packet_fields(struct ofl_packet_in* pkt_in,struct nsc_selector_node* selector_p,uint8_t direction,uint8_t nsc_copy_mac_addresses_b)
{
  uint8_t*  pkt_data = pkt_in->packet;
  uint32_t  ii,ipv4_hdr_len;
  uint32_t  temp;
  uint16_t  offset;
  //uint8_t   index; 

  int32_t   retval = OF_SUCCESS;

  if(pkt_in->packet_length < 34)  /* packet length shall atleast be that of (IP Header + MAC header) */
    return OF_FAILURE;

  //index = 0;

  offset = OF_CNTLR_ETHERNET_TYPE_OFFSET;
  temp = (ntohl(*(unsigned int*)(pkt_data + offset)));

  selector_p->vlan_id_pkt = 0;
  while((temp & 0xff000000) == 0x81000000)
  {
    if(offset == OF_CNTLR_ETHERNET_TYPE_OFFSET)
    {
      selector_p->vlan_id_pkt = (unsigned short)(temp & 0x00000fff);
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"vlan_id_pkt = %d\n",selector_p->vlan_id_pkt); 
    }
    offset = offset + 4;
    temp = (ntohl(*(unsigned int*)(pkt_data + offset)));
  }
  selector_p->ethernet_type = (temp >> 16);

  if(selector_p->ethernet_type == 0x0806)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Unexpected ARP Response received");
    return OF_FAILURE;   /* ARP Packet */
  }   

  ipv4_hdr_len = OF_CNTLR_IPV4_HDR_LEN((pkt_data + offset - OF_CNTLR_ETHERNET_TYPE_OFFSET));
  
  offset = offset + 2;

  if(direction == PACKET_NORMAL)
  {

    if((nsc_copy_mac_addresses_b == FALSE) && (selector_p->vlan_id_pkt != 0)) 
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Not Extracting MAC Addresses for packets received from VM_NS with VLAN ID");
    }
    else 
    {
      selector_p->dst_mac_high  =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET));
      selector_p->dst_mac_low   =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET + 2));
      selector_p->src_mac_high  =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_SRCMAC_OFFSET));
      selector_p->src_mac_low   =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_SRCMAC_OFFSET + 2));

      for(ii=0;ii<6;ii++)
      {
        selector_p->dst_mac[ii]   =  (pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET)[ii];
        selector_p->src_mac[ii]   =  (pkt_data + OF_CNTLR_MACHDR_SRCMAC_OFFSET)[ii];
      }
    } 
      selector_p->src_ip    = ntohl(*(uint32_t*)(pkt_data + offset + OF_CNTLR_IPV4_SRC_IP_OFFSET));
      selector_p->dst_ip    = ntohl(*(uint32_t*)(pkt_data + offset + OF_CNTLR_IPV4_DST_IP_OFFSET));
      selector_p->protocol  = *(pkt_data + offset + OF_CNTLR_IPV4_PROTO_OFFSET);
        
    if((selector_p->protocol == OF_IPPROTO_TCP) || (selector_p->protocol == OF_IPPROTO_UDP ))
    {
      selector_p->src_port  = ntohs(*(uint16_t*)(pkt_data + offset + ipv4_hdr_len));
      selector_p->dst_port  = ntohs(*(uint16_t*)(pkt_data + offset + ipv4_hdr_len+2));
    }
    else if(selector_p->protocol == OF_IPPROTO_ICMP)
    {
      selector_p->src_port = *(pkt_data + offset + ipv4_hdr_len);
      selector_p->dst_port = *(pkt_data + offset + ipv4_hdr_len + OF_CNTLR_ICMPV4_CODE_OFFSET);

      selector_p->icmp_type = *(pkt_data + offset + ipv4_hdr_len);
      selector_p->icmp_code = *(pkt_data + offset + ipv4_hdr_len + OF_CNTLR_ICMPV4_CODE_OFFSET);
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Unsupported IP Protocol = %x",selector_p->protocol);      
      retval = OF_FAILURE;   
    }   
  }
  else /* Opposite direction selector */
  {
    if((nsc_copy_mac_addresses_b == FALSE) && (selector_p->vlan_id_pkt != 0))
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Not Extracting MAC Addresses for packets received from VM_NS with VLAN ID");
    }
    else
    { 
      selector_p->src_mac_high  =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET));
      selector_p->src_mac_low   =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET + 2));
      selector_p->dst_mac_high  =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_SRCMAC_OFFSET));
      selector_p->dst_mac_low   =  ntohl(*(uint32_t*)(pkt_data + OF_CNTLR_MACHDR_SRCMAC_OFFSET + 2));

      for(ii=0;ii<6;ii++)
      {
        selector_p->src_mac[ii]   =  (pkt_data + OF_CNTLR_MACHDR_DSTMAC_OFFSET)[ii];
        selector_p->dst_mac[ii]   =  (pkt_data + OF_CNTLR_MACHDR_SRCMAC_OFFSET)[ii];
      }
    }
    selector_p->dst_ip    = ntohl(*(uint32_t*)(pkt_data + offset +  OF_CNTLR_IPV4_SRC_IP_OFFSET));
    selector_p->src_ip    = ntohl(*(uint32_t*)(pkt_data + offset +  OF_CNTLR_IPV4_DST_IP_OFFSET));
    selector_p->protocol  = *(pkt_data + offset + OF_CNTLR_IPV4_PROTO_OFFSET);
  
    if((selector_p->protocol == OF_IPPROTO_TCP) || (selector_p->protocol == OF_IPPROTO_UDP ))
    {
      selector_p->dst_port  = ntohs(*(uint16_t*)(pkt_data + offset  + ipv4_hdr_len));
      selector_p->src_port  = ntohs(*(uint16_t*)(pkt_data + offset  + ipv4_hdr_len+2));
    }
    else if(selector_p->protocol == OF_IPPROTO_ICMP)
    {
      selector_p->src_port = *(pkt_data + offset + ipv4_hdr_len);
      selector_p->dst_port = *(pkt_data + offset + ipv4_hdr_len + OF_CNTLR_ICMPV4_CODE_OFFSET);

      selector_p->icmp_type = *(pkt_data + offset + ipv4_hdr_len);
      selector_p->icmp_code = *(pkt_data + offset + ipv4_hdr_len + OF_CNTLR_ICMPV4_CODE_OFFSET);
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Unsupported IP Protocol = %x",selector_p->protocol);
      retval = OF_FAILURE;
    }
  }
  #if 0
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"ethernet_type = %x",selector_p->ethernet_type);
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst_mac_high = %x dst_mac_low = %x src_mac_high = %x src_mac_low = %x",
                                                         selector_p->dst_mac_high,selector_p->dst_mac_low,
                                                         selector_p->src_mac_high,selector_p->src_mac_low);

  for(ii=0;ii<6;ii++)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"dst mac = %x",selector_p->dst_mac[ii]);
  }
  for(ii=0;ii<6;ii++)
  {
    OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src mac = %x",selector_p->src_mac[ii]);
  }

  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"src_ip = %x dst_ip = %x src_prt=%d dst_prt=%d protcolo=%d",
                                                               selector_p->src_ip,selector_p->dst_ip,
                                                               selector_p->src_port,selector_p->dst_port,
                                                               selector_p->protocol);
    
  OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"icmp_type = %d icmp_code = %d",selector_p->icmp_type,selector_p->icmp_code);
  
  #endif

  return retval;
}
/****************************************************************************************************************************************************************************
Function: tsc_mark_cn_bind_entry_as_stale
Input Parameters:
        l2_connection_to_nsinfo_bind_table_p:Hash Table from which the selectors are to be deleted. 
        nsc_cn_bind_mempool_g: Mempool from which the cnbind entry is to be deleted.  
        index,magic:Identify the cnbind entry to be marked and deleted. 

Description:
        1.This function marks the cnbind entry as a stale entry so that new repsoitory entries are not deduced from it.
        2.The two selectors are deleted from the hash table.
        3.Memory allocated for cnbind entry is de-allocated.
***************************************************************************************************************************************************************************/
void tsc_mark_cn_bind_entry_as_stale(struct mchash_table* l2_connection_to_nsinfo_bind_table_p,void* nsc_cn_bind_mempool_g,uint32_t index,uint32_t magic)
{
  struct   nsc_selector_node* selector_p;
  uint32_t offset;
  int32_t  status_b = FALSE;  

  do
  {
    MCHASH_ACCESS_NODE(l2_connection_to_nsinfo_bind_table_p,index,magic,selector_p,status_b);
    if(status_b != TRUE)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR,"No selector entry found");
      printf("\r\n NSMDEL 100 No selector entry found ");
      break;
    }
  
    CNTLR_LOCK_TAKE(selector_p->cnbind_node_p->cnbind_node_lock);
    if(selector_p->cnbind_node_p->no_of_flow_entries_deduced == 0)
    {
      CNTLR_LOCK_RELEASE(selector_p->cnbind_node_p->cnbind_node_lock);
      break;
    }
    
    if((--(selector_p->cnbind_node_p->no_of_flow_entries_deduced)) == 0)
    {
      selector_p->cnbind_node_p->stale_entry = 1;
      printf(" \r\n Stale entry is marked");
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG,"Mark the cn_bind_entry as a stale entry so that it is not used further.");
    }

    printf("flstodel %d",selector_p->cnbind_node_p->no_of_flow_entries_deduced);
    
    if(selector_p->cnbind_node_p->stale_entry == 1)
    {
      CNTLR_LOCK_RELEASE(selector_p->cnbind_node_p->cnbind_node_lock);
      offset =  NSC_SELECTOR_NODE_OFFSET;
      status_b = FALSE;
      MCHASH_DELETE_NODE_BY_REF(l2_connection_to_nsinfo_bind_table_p,selector_p->other_selector_p->index,selector_p->other_selector_p->magic,
                                struct nsc_selector_node *,offset,status_b);

      if(status_b == TRUE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Secondary Selector entry deleted successfully from cnbind table");
        printf(" NSMDEL 104 ");
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Secondary Selector entry deletion failed from cnbind table");
        printf(" NSMDEL 105 ");
      }
      offset =  NSC_SELECTOR_NODE_OFFSET; 
      status_b = FALSE;
      MCHASH_DELETE_NODE_BY_REF(l2_connection_to_nsinfo_bind_table_p,selector_p->index,selector_p->magic,
                                struct nsc_selector_node *,offset,status_b); 
      if(status_b == TRUE)
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "Primary Selector entry deleted successfully from cnbind table");
        printf(" NSMDEL 106 ");
      }
      else
      {
        OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Primary Selector entry deletion failed from cnbind table");
        printf(" NSMDEL 107 ");
      }
    }  
    else
    {
      CNTLR_LOCK_RELEASE(selector_p->cnbind_node_p->cnbind_node_lock);
    }
  }while(0);    
}
/****************************************************************************************************************************************************************************/
void tsc_nsc_cnbind_sel_free(struct rcu_head* sel_p)
{
  
  struct   nsc_selector_node* selector_p;
  struct   l2_connection_to_nsinfo_bind_node* cn_bind_node_entry_p;
  uint32_t offset;
  int32_t  retval;

  offset = NSC_SELECTOR_NODE_OFFSET;
  selector_p = (struct nsc_selector_node *) (((char *)sel_p - (2*(sizeof(struct mchash_dll_node *)))) - offset);

  cn_bind_node_entry_p = selector_p->cnbind_node_p;

  if((++(cn_bind_node_entry_p->cnbind_sel_refcnt)) == 2)
  {
    //printf("\r\n Both the selectors deleted. Delete cnbind entry.");
    /* Free Network Service Info before freeing cnbind entry itself */
    cn_bind_node_entry_p->nwservice_info.no_of_nwservice_instances = 0;
    if(cn_bind_node_entry_p->nwservice_info.nw_services_p != NULL)
      free(cn_bind_node_entry_p->nwservice_info.nw_services_p);

    retval = mempool_release_mem_block(cn_bind_node_entry_p->nsc_cn_bind_mempool_g,(uchar8_t*)cn_bind_node_entry_p,cn_bind_node_entry_p->heap_b);
    if(retval == MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_DEBUG, "cnbind entry memory block released succussfully!.");
      printf("\r\ncnbind entry is released");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_TSC, OF_LOG_ERROR, "Failed to release cnbind entry memory block");
      printf("\r\nFailed to release cnbind entry memory block");
    }
  }
}
/****************************************************************************************************************************************************************************/
uint32_t NSC_COMPUTE_HASH_CNBIND_ENTRY(uint32_t saddr,uint32_t daddr,
                                       uint32_t sport,uint32_t dport,
                                       uint32_t protocol,
                                       uint32_t ethernet_type,
                                       uint32_t hashmask
                                      )
{
  uint32_t hashkey;
  
  CNTLR_DOBBS_WORDS4(saddr,daddr,((sport << 16) | dport),((protocol << 16) | ethernet_type),/* src_mac_high,src_mac_low,dst_mac_high,dst_mac_low,*/ 0x12345678,hashmask,hashkey);
  return hashkey;
}
/***************************************************************************************************************************************************************************/
uint32_t nsc_compute_hash_repository_table_1_2(uint32_t saddr,uint32_t daddr,
                                               uint32_t sport,uint32_t dport,
                                               uint32_t protocol,
                                               uint32_t ethernet_type,
                                               uint32_t port_id,
                                               uint64_t dp_handle,
                                               uint16_t vlan_id,   
                                               uint32_t hashmask
                                              )
{
  uint32_t hashkey;
  uint32_t dp_handle_low  = (uint32_t)(dp_handle);
  uint32_t dp_handle_high = (uint32_t)(dp_handle >> 32);
  uint32_t vlanid = (uint32_t)vlan_id;  

 CNTLR_DOBBS_WORDS8(saddr,daddr,((sport << 16) | dport),((protocol << 16) | ethernet_type),
                    port_id,dp_handle_low,dp_handle_high,vlanid,
                    0x12345678,hashmask,hashkey);
 return hashkey;
}
/***************************************************************************************************************************************************************************/
uint32_t nsc_compute_hash_repository_table_3(uint16_t serviceport,
                                             uint32_t dst_mac_high,
                                             uint32_t dst_mac_low,
                                             uint64_t dp_handle,
                                             uint32_t hashmask)
{
  uint32_t hashkey;
  uint32_t dp_handle_low  = (uint32_t)(dp_handle);
  uint32_t dp_handle_high = (uint32_t)(dp_handle >> 32);
  uint32_t service_port   = (uint32_t)serviceport;
  
  CNTLR_DOBBS_WORDS5(dst_mac_high,dst_mac_low,service_port,
                     dp_handle_low,dp_handle_high,
                     0x12345678,hashmask,hashkey);
 return hashkey;
}
/*************************************************************************************************************************************************************************/

