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

/*File: tsc_bintree.c
 * Author: ns murthy <b37840@freescale.com>
 * Description:
 * This file implements binary tree for zone storage and search for a VM IP Address.
 */

#include "tsc_controller.h"

struct tsc_bintree_node* tsc_bintree_root;

struct tsc_bintree_node* tsc_bintree_insert(struct tsc_bintree_node *node,unsigned int data,char* zone)
{
  if(node==NULL)
  {
    struct tsc_bintree_node *temp;
    temp = (struct tsc_bintree_node *)malloc(sizeof(struct tsc_bintree_node));
    temp->data = data;
    srtncpy(temp->zone,zone,CRM_MAX_ZONE_SIZE);
    temp->zone_direction = 0;
    temp->left = temp->right = NULL;
    return temp;
  }

  if(data >(node->data))
  {
    node->right = tsc_bintree_insert(node->right,data,zone);
  }
  else if(data < (node->data))
  {
    node->left = tsc_bintree_insert(node->left,data,zone);
  }
  return node;
}

struct tsc_bintree_node* tsc_bintree_find(struct tsc_bintree_node *node, int data)
{
  if(node==NULL)
  {
    return NULL;
  }
  if(data > node->data)
  {
    return tsc_bintree_find(node->right,data);
  }
  else if(data < node->data)
  {
    return tsc_bintree_find(node->left,data);
  }
  else
  {
    return node;
  }
}

void tsc_bintree_list(struct tsc_bintree_node* tree)
{
  if(tree->left) tsc_bintree_list(tree->left);
  printf("\r\n ip_addr = %x",tree->data);
  printf(" zone = %s",tree->zone);
  if(tree->right) tsc_bintree_list(tree->right);
}
