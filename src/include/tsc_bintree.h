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

/* File: tsc_nsc_iface.h
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This header file contains definitions of interface to service chaining core module.
 */

struct tsc_bintree_node
{
  unsigned int data;
  char         zone[CRM_MAX_ZONE_SIZE];
  unsigned int zone_direction;
  struct tsc_bintree_node *left;
  struct tsc_bintree_node *right;
};

struct tsc_bintree_node* tsc_bintree_insert(struct tsc_bintree_node *node,unsigned int data,char* zone);
struct tsc_bintree_node* tsc_bintree_find(struct tsc_bintree_node *node, unsigned int data);
void   tsc_bintree_list(struct tsc_bintree_node* tree);
