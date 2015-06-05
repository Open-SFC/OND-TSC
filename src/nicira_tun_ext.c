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

/*!\file of_utilities.c
 * Author     : ns murthy
 * Date       : 
 * Description: 
 * This file contains defintions of different utility functions 
 * that used to set nicira extended match fileds and actions.
 */
#include "controller.h"
#include "of_utilities.h"
#include "nicira_tun_ext.h"

/*########################### nicira specific Match field routines #################################*/
int32_t
of_set_ipv4_tun_src(struct of_msg *msg,
                    ipv4_addr_t   *ipv4_addr,
                    uint8_t       is_mask_set,
                    ipv4_addr_t   *ipv4_addr_mask)
{
   struct of_msg_descriptor *msg_desc = &msg->desc;

   cntlr_assert(msg != NULL);
   msg_desc = &msg->desc;

   if(is_mask_set)
   {
     if( (msg_desc->ptr1+msg_desc->length1+OFU_IPV4_SRC_MASK_LEN) >
         (msg->desc.start_of_data + msg_desc->buffer_len)   )
        {
          OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"%s %d :Length of buffer is not sufficient to add data\r\n",__FUNCTION__,__LINE__);
          return OFU_NO_ROOM_IN_BUFFER;
        } 

        *(uint32_t*)(msg_desc->ptr1+msg_desc->length1)  = htonl(NXM_NX_TUN_IPV4_SRC_W);
        *(uint32_t*)(msg_desc->ptr1+msg_desc->length1 + OF_NXM_HDR_LEN) = htonl(*ipv4_addr);
        *(uint32_t*)(msg_desc->ptr1+msg_desc->length1 + 8) = htonl(*ipv4_addr_mask);
         msg_desc->length1 += OFU_IPV4_SRC_MASK_LEN;
   }     
   
   else
   {
     if( (msg_desc->ptr1+msg_desc->length1+OFU_IPV4_SRC_FIELD_LEN) >
         (msg->desc.start_of_data + msg_desc->buffer_len)   )
        {
          OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"%s %d :Length of buffer is not sufficient to add data\r\n",__FUNCTION__,__LINE__);
          return OFU_NO_ROOM_IN_BUFFER;
        }
       *(uint32_t*)(msg_desc->ptr1+msg_desc->length1) = htonl(NXM_NX_TUN_IPV4_SRC);
       *(uint32_t*)(msg_desc->ptr1+msg_desc->length1 + OF_NXM_HDR_LEN) = htonl(*ipv4_addr);
       msg_desc->length1 += OFU_IPV4_SRC_FIELD_LEN;
   }
   return OFU_SET_FIELD_SUCCESS;
}

int32_t
of_set_ipv4_tun_dst(struct of_msg *msg,
                    ipv4_addr_t   *ipv4_addr,
                    uint8_t       is_mask_set,
                    ipv4_addr_t   *ipv4_addr_mask)
{
  struct of_msg_descriptor *msg_desc = &msg->desc;

  cntlr_assert(msg != NULL);
  msg_desc = &msg->desc;

  if(is_mask_set)
  {
    if( (msg_desc->ptr1+msg_desc->length1+OFU_IPV4_DST_MASK_LEN) >
        (msg->desc.start_of_data + msg_desc->buffer_len)   )
       {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"%s %d :Length of buffer is not sufficient to add data\r\n",__FUNCTION__,__LINE__);
         return OFU_NO_ROOM_IN_BUFFER;
       }

       *(uint32_t*)(msg_desc->ptr1+msg_desc->length1)  = htonl(NXM_NX_TUN_IPV4_DST_W);
       *(uint32_t*)(msg_desc->ptr1+msg_desc->length1 + OF_NXM_HDR_LEN) = htonl(*ipv4_addr);
       *(uint32_t*)(msg_desc->ptr1+msg_desc->length1 + 8) = htonl(*ipv4_addr_mask);
       msg_desc->length1 += OFU_IPV4_SRC_MASK_LEN;
  }

  else
  {
   if( (msg_desc->ptr1+msg_desc->length1+OFU_IPV4_DST_FIELD_LEN) >
       (msg->desc.start_of_data + msg_desc->buffer_len)   )
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"%s %d :Length of buffer is not sufficient to add data\r\n",__FUNCTION__,__LINE__);
        return OFU_NO_ROOM_IN_BUFFER;
      }
      *(uint32_t*)(msg_desc->ptr1+msg_desc->length1) = htonl(NXM_NX_TUN_IPV4_DST);
      *(uint32_t*)(msg_desc->ptr1+msg_desc->length1 + OF_NXM_HDR_LEN) = htonl(*ipv4_addr);
      msg_desc->length1 += OFU_IPV4_DST_FIELD_LEN;
  }
  return OFU_SET_FIELD_SUCCESS;
}
/*########################### Action routines #################################*/
int32_t
ofu_push_set_ipv4_tun_src_addr_in_set_field_action(struct of_msg *msg,
                                                   ipv4_addr_t   *ipv4_addr)
{
  struct of_msg_descriptor *msg_desc;
  struct ofp_action_set_field *set_field;
  uint16_t padding_bytes = OF_SET_FIELD_ACTION_PADDING_LEN(4);

  cntlr_assert(msg != NULL);
  msg_desc = &msg->desc;


  if( (msg_desc->ptr1+msg_desc->length1+OFU_SET_FIELD_WITH_IPV4_SRC_FIELD_ACTION_LEN) >
      (msg->desc.start_of_data + msg_desc->buffer_len)   )
     {
       OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"%s %d :Length of buffer is not sufficient to add data\r\n",__FUNCTION__,__LINE__);
       return OFU_NO_ROOM_IN_BUFFER;
     }
     set_field = (struct ofp_action_set_field *)(msg_desc->ptr3+msg_desc->length3);
     set_field->type = htons(OFPAT_SET_FIELD);
     set_field->len  = htons(OFU_SET_FIELD_WITH_IPV4_SRC_FIELD_ACTION_LEN + padding_bytes);
     *(uint32_t*)set_field->field   = htonl(NXM_NX_TUN_IPV4_SRC);
     *(uint32_t*)(set_field->field + OF_NXM_HDR_LEN) = htonl(*ipv4_addr);
     memset( (set_field->field + OFU_SET_FIELD_WITH_IPV4_SRC_FIELD_ACTION_LEN),0,padding_bytes);
     msg_desc->length3 += OFU_SET_FIELD_WITH_IPV4_SRC_FIELD_ACTION_LEN + padding_bytes;
     return OFU_ACTION_PUSH_SUCCESS;
}

int32_t
ofu_push_set_ipv4_tun_dst_addr_in_set_field_action(struct of_msg *msg,
                                                   ipv4_addr_t   *ipv4_addr)
{
  struct of_msg_descriptor *msg_desc;
  struct ofp_action_set_field *set_field;
  uint16_t padding_bytes = OF_SET_FIELD_ACTION_PADDING_LEN(4);

  cntlr_assert(msg != NULL);
  msg_desc = &msg->desc;


  if( (msg_desc->ptr1+msg_desc->length1+OFU_SET_FIELD_WITH_IPV4_DST_FIELD_ACTION_LEN) >
      (msg->desc.start_of_data + msg_desc->buffer_len)   )
     {
       OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"%s %d :Length of buffer is not sufficient to add data\r\n",__FUNCTION__,__LINE__);
       return OFU_NO_ROOM_IN_BUFFER;
     }
     set_field = (struct ofp_action_set_field *)(msg_desc->ptr3+msg_desc->length3);
     set_field->type = htons(OFPAT_SET_FIELD);
     set_field->len  = htons(OFU_SET_FIELD_WITH_IPV4_DST_FIELD_ACTION_LEN + padding_bytes);
     *(uint32_t*)set_field->field   = htonl(NXM_NX_TUN_IPV4_DST);
     *(uint32_t*)(set_field->field + OF_NXM_HDR_LEN) = htonl(*ipv4_addr);
     memset( (set_field->field + OFU_SET_FIELD_WITH_IPV4_DST_FIELD_ACTION_LEN),0,padding_bytes);
     msg_desc->length3 += OFU_SET_FIELD_WITH_IPV4_DST_FIELD_ACTION_LEN + padding_bytes;
     return OFU_ACTION_PUSH_SUCCESS;
}
/*########################### Match field Parseing routines #################################*/
int32_t
ofu_get_tun_id_field(struct   of_msg *msg,
                     void     *match_fields,
                     uint16_t  match_fields_len,
                     uint64_t  *tun_id)
{
  uint32_t  oxm_header;
  uint8_t   *ptr;
  uint16_t  match_field_len;
  uint16_t  parsed_data_len;

  cntlr_assert(match_fields != NULL);

  ptr =  (uint8_t*)match_fields;

  while(match_fields_len)
  {
    oxm_header = ntohl(*(uint32_t*)ptr);

    if((NXM_VENDOR(oxm_header) == 0x0001) && (OXM_FIELD(oxm_header) == NXM01_TUN_ID))
    {
      *tun_id = ntohl(*(uint64_t*)(ptr+OF_OXM_HDR_LEN));
      return OFU_TUN_ID_FIELD_PRESENT;
    }
    else
    {
      parsed_data_len = (OF_OXM_HDR_LEN + OXM_LENGTH(oxm_header));
      match_field_len -= parsed_data_len;
      ptr             += parsed_data_len;
    }
  }
  return OFU_TUN_ID_FIELD_NOT_PRESENT;
}

int32_t
ofu_get_tun_src_field(struct   of_msg *msg,
                      void     *match_fields,
                      uint16_t match_fields_len,
                      uint32_t *tun_src)
{
  uint32_t  oxm_header;
  uint8_t   *ptr;
  uint16_t  match_field_len;
  uint16_t  parsed_data_len;

  cntlr_assert(match_fields != NULL);

  ptr = (uint8_t*)match_fields;

  while(match_fields_len)
  {
    oxm_header = ntohl(*(uint32_t*)ptr);

    if((NXM_VENDOR(oxm_header) == 0x0001) && (OXM_FIELD(oxm_header) == NXM01_TUN_IPV4_SRC))
    {
      *tun_src = ntohl(*(uint32_t*)(ptr+OF_OXM_HDR_LEN));
      return OFU_TUN_SRC_FIELD_PRESENT;
    }
    else
    {
      parsed_data_len = (OF_OXM_HDR_LEN + OXM_LENGTH(oxm_header));
      match_field_len -= parsed_data_len;
      ptr             += parsed_data_len;
    }
  }
  return OFU_TUN_SRC_FIELD_NOT_PRESENT;
}

int32_t
ofu_get_tun_dst_field(struct   of_msg *msg,
                      void      *match_fields,
                      uint16_t  match_fields_len,
                      uint32_t  *tun_dst)
{
  uint32_t  oxm_header;
  uint8_t   *ptr;
  uint16_t  match_field_len;
  uint16_t  parsed_data_len;

  cntlr_assert(match_fields != NULL);

  ptr =  (uint8_t*)match_fields;

  while(match_fields_len)
  {
    oxm_header = ntohl(*(uint32_t*)ptr);

    if((NXM_VENDOR(oxm_header) == 0x0001) && (OXM_FIELD(oxm_header) == NXM01_TUN_IPV4_DST))
    {
      *tun_dst = ntohl(*(uint32_t*)(ptr+OF_OXM_HDR_LEN));
      return OFU_TUN_DST_FIELD_PRESENT;
    }
    else
    {
      parsed_data_len = (OF_OXM_HDR_LEN + OXM_LENGTH(oxm_header));
      match_field_len -= parsed_data_len;
      ptr             += parsed_data_len;
    }
  }
  return OFU_TUN_DST_FIELD_NOT_PRESENT;
}
