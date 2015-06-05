
/*
 * Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013 Nicira, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPENFLOW_NICIRA_EXT_H
#define OPENFLOW_NICIRA_EXT_H 1

#define OF_OXM_HDR_LEN   (4)
#define OF_NXM_HDR_LEN   (4)

#define OF_SET_FIELD_ACTION_PADDING_LEN(nxm_len) (((nxm_len) + 7)/8*8 - (nxm_len))

#define NXM_HEADER__(VENDOR, FIELD, HASMASK, LENGTH) \
      (((VENDOR) << 16) | ((FIELD) << 9) | ((HASMASK) << 8) | (LENGTH))
#define NXM_HEADER(VENDOR, FIELD, LENGTH) \
      NXM_HEADER__(VENDOR, FIELD, 0, LENGTH)
#define NXM_HEADER_W(VENDOR, FIELD, LENGTH) \
      NXM_HEADER__(VENDOR, FIELD, 1, (LENGTH) * 2)
#define NXM_VENDOR(HEADER) ((HEADER) >> 16)
#define NXM_FIELD(HEADER) (((HEADER) >> 9) & 0x7f)
#define NXM_TYPE(HEADER) (((HEADER) >> 9) & 0x7fffff)
#define NXM_HASMASK(HEADER) (((HEADER) >> 8) & 1)
#define NXM_LENGTH(HEADER) ((HEADER) & 0xff)

#define NXM_MAKE_WILD_HEADER(HEADER) \
          NXM_HEADER_W(NXM_VENDOR(HEADER), NXM_FIELD(HEADER), NXM_LENGTH(HEADER))

#define NXM01_TUN_ID        16
#define NXM01_TUN_IPV4_SRC  31
#define NXM01_TUN_IPV4_DST  32
#define NXM01_TUN_NW_TYPE  
#define NXM01_TUN_PKT_ORIG  

#define NXM_NX_TUN_ID     NXM_HEADER  (0x0001, /*16*/ NXM01_TUN_ID, 8)
#define NXM_NX_TUN_ID_W   NXM_HEADER_W(0x0001, /*16*/ NXM01_TUN_ID, 8)

#define NXM_NX_TUN_IPV4_SRC   NXM_HEADER  (0x0001, /*31*/ NXM01_TUN_IPV4_SRC, 4)
#define NXM_NX_TUN_IPV4_SRC_W NXM_HEADER_W(0x0001, /*31*/ NXM01_TUN_IPV4_SRC, 4)
#define NXM_NX_TUN_IPV4_DST   NXM_HEADER  (0x0001, /*32*/ NXM01_TUN_IPV4_DST, 4)
#define NXM_NX_TUN_IPV4_DST_W NXM_HEADER_W(0x0001, /*32*/ NXM01_TUN_IPV4_DST, 4)

#define NXM_NX_REG0     NXM_HEADER  (0x0001, NXM01_REG1,4)
#define NXM_NX_REG0_W   NXM_HEADER_W(0x0001, NXM01_REG1,4)


int32_t
ofu_get_tun_id_field(struct   of_msg *msg,
                     void     *match_fields,
                     uint16_t  match_fields_len,
                     uint64_t *tun_id);

int32_t
ofu_get_tun_src_field(struct   of_msg *msg,
                      void     *match_fields,
                      uint16_t match_fields_len,
                      uint32_t *tun_src);

int32_t
ofu_get_tun_dst_field(struct   of_msg *msg,
                      void     *match_fields,
                      uint16_t match_fields_len,
                      uint32_t *tun_dst);


int32_t
of_set_ipv4_tun_src(struct of_msg *msg,
                    ipv4_addr_t   *ipv4_addr,
                    uint8_t       is_mask_set,
                    ipv4_addr_t   *ipv4_addr_mask);
int32_t
of_set_ipv4_tun_dst(struct of_msg *msg,
                    ipv4_addr_t   *ipv4_addr,
                    uint8_t       is_mask_set,
                    ipv4_addr_t   *ipv4_addr_mask);

int32_t
ofu_push_set_ipv4_tun_src_addr_in_set_field_action(struct of_msg *msg,
                                                   ipv4_addr_t   *ipv4_addr);

int32_t
ofu_push_set_ipv4_tun_dst_addr_in_set_field_action(struct of_msg *msg,
                                                   ipv4_addr_t   *ipv4_addr);



#endif /* openflow/nicira-ext.h */
