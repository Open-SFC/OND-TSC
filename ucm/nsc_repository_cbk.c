/*
** Traffic Steering selectors source file 
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

/*
 *
 * File name: tsc_selector.c
 * Author:P.Vinod Sarma
 * Date: 
 * Description: 
*/
#include "cmincld.h"
#include "controller.h"
#include "cbcmninc.h"
#include "cmgutil.h"
#include "cntrucminc.h"
//#include "tsc_nsc_core.h"
#include "tsc_cli.h"
//#include "tsc_controller.h"


int32_t nsc_repository_init(void);

int32_t nsc_repository_addrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p);

int32_t nsc_repository_modrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p);

int32_t nsc_repository_getfirstnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p);

int32_t nsc_repository_getnextnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        struct cm_array_of_iv_pairs *prev_record_key_p,  uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** next_n_record_data_p);

int32_t nsc_repository_getexactrec (struct cm_array_of_iv_pairs * keys_arr_p,
                                   struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p);


int32_t nsc_repository_verifycfg (struct cm_array_of_iv_pairs *key_iv_pairs_p,
                        uint32_t command_id, struct cm_app_result ** result_p);


int32_t nsc_repository_ucm_setmandparams (struct cm_array_of_iv_pairs *pMandParams,
                                          struct nsc_repository_key* key_info);


int32_t nsc_repository_ucm_setoptparams (struct cm_array_of_iv_pairs *pOptParams,
                                         struct  nsc_repository_key* key_info);


int32_t nsc_repository_ucm_getparams (struct  nsc_repository_key   *key_info,
                                      struct  nsc_repository_entry *repository_entry_info_p,
                                      struct  cm_array_of_iv_pairs * result_iv_pairs_p);


struct cm_dm_app_callbacks nsc_repository_ucm_callbacks = {
  NULL,
  nsc_repository_addrec,
  nsc_repository_modrec,
  NULL,
  NULL,
  nsc_repository_getfirstnrecs,
  nsc_repository_getnextnrecs,
  nsc_repository_getexactrec,
  nsc_repository_verifycfg,
  NULL
};


uint64_t node_saferef = 0;
uint64_t relative_node_saferef = 0;

struct   nsc_repository_key  *global_key = NULL;


int32_t nsc_repository_init(void)
{
  cm_register_app_callbacks (CM_ON_DIRECTOR_TSC_REPOSITORY_APPL_ID,&nsc_repository_ucm_callbacks);
  return OF_SUCCESS;
}

int32_t nsc_repository_addrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p)
{
  struct   nsc_repository_key     *key_p = NULL;
  struct   nsc_repository_entry   *nsc_repository_entry_p = NULL;
  struct   cm_app_result            *app_result = NULL;
  struct   cm_array_of_iv_pairs     *result_iv_pairs_p = NULL;

  int32_t retval = OF_FAILURE;
  CM_CBK_DEBUG_PRINT ("Entered addrec");
  key_p = (struct nsc_repository_entry*)calloc(1,sizeof(struct nsc_repository_entry));
  key_p->vn_name = (char*)calloc(1,128);

  if(nsc_repository_ucm_setmandparams(pMandParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }


  if(nsc_repository_ucm_setoptparams(pOptParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_OPT_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }

  global_key = key_p;
 
  retval = nsc_add_repository_entry(key_p);
  if(retval != TSC_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Add Failed");
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
  return OF_SUCCESS;
}

int32_t nsc_repository_modrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p)
{
  struct   nsc_repository_key     *key_p = NULL;
  struct   nsc_repository_entry   *nsc_repository_entry_p = NULL;
  struct   cm_app_result            *app_result = NULL;
  struct   cm_array_of_iv_pairs     *result_iv_pairs_p = NULL;

  int32_t retval = OF_FAILURE;
  CM_CBK_DEBUG_PRINT ("Entered modrec");
  key_p = (struct nsc_repository_entry*)calloc(1,sizeof(struct nsc_repository_entry));
  key_p->vn_name = (char*)calloc(1,128);

  if(nsc_repository_ucm_setmandparams(pMandParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }


  if(nsc_repository_ucm_setoptparams(pOptParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_OPT_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }

  global_key = key_p;
 
  retval = nsc_add_repository_entry(key_p);
  if(retval != TSC_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Add Failed");
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
  return OF_SUCCESS;
}

int32_t nsc_repository_getfirstnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;

  struct nsc_repository_key    *key_info_p = NULL;
  struct nsc_repository_entry  *nsc_repository_entry_p;
  uint32_t uiRecCount = 0;
  int32_t  retval = OF_FAILURE;

  nsc_repository_entry_p = (struct nsc_repository_entry*)calloc(1,sizeof(struct nsc_repository_entry));

  retval = nsc_get_first_repository_entry(key_info_p,&node_saferef,nsc_repository_entry_p);
  if(retval != TSC_SUCCESS)
  {
     CM_CBK_DEBUG_PRINT("Get first failed");
     free(nsc_repository_entry_p);
     return OF_FAILURE;
  }

  result_iv_pairs_p = (struct cm_array_of_iv_pairs *)calloc(1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    free(nsc_repository_entry_p);
    return OF_FAILURE;
  }

  nsc_repository_ucm_getparams (global_key,nsc_repository_entry_p, &result_iv_pairs_p[uiRecCount]);
  CM_CBK_DEBUG_PRINT("global_key vnaname : %s",global_key->vn_name);

  uiRecCount++;
 *array_of_iv_pair_arr_p = result_iv_pairs_p;
  return OF_SUCCESS;
}

int32_t nsc_repository_getnextnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        struct cm_array_of_iv_pairs *prev_record_key_p,  uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** next_n_record_data_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;

  struct nsc_repository_key    *key_info_p = NULL;
  struct nsc_repository_entry  *nsc_repository_entry_p = NULL;
  uint32_t uiRecCount = 0;
  int32_t  retval = OF_FAILURE;

  nsc_repository_entry_p = (struct nsc_repository_entry*)calloc(1,sizeof(struct nsc_repository_entry));

  retval = nsc_get_next_repository_entry(key_info_p,node_saferef,&relative_node_saferef,nsc_repository_entry_p);
  if(retval != TSC_SUCCESS)
  {
     CM_CBK_DEBUG_PRINT("Get next failed");
     free(nsc_repository_entry_p);
     return OF_FAILURE;
  }

  CM_CBK_DEBUG_PRINT("switchname : %s",nsc_repository_entry_p->local_switch_name_p); 
  node_saferef = relative_node_saferef;
  relative_node_saferef = 0;

  result_iv_pairs_p = (struct cm_array_of_iv_pairs *)calloc(1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    free(nsc_repository_entry_p);
    return OF_FAILURE;
  }
  nsc_repository_ucm_getparams (global_key,nsc_repository_entry_p, &result_iv_pairs_p[uiRecCount]);

  uiRecCount++;
  *next_n_record_data_p = result_iv_pairs_p;
  *count_p = uiRecCount;

  return OF_SUCCESS;
}

int32_t nsc_repository_ucm_setmandparams (struct cm_array_of_iv_pairs *pMandParams,
                                          struct nsc_repository_key* key_info)
{
  uint32_t uiMandParamCnt;
  uint64_t idpId;

  CM_CBK_DEBUG_PRINT ("Entered");

  for (uiMandParamCnt = 0; uiMandParamCnt < pMandParams->count_ui;
       uiMandParamCnt++)
  {
    switch (pMandParams->iv_pairs[uiMandParamCnt].id_ui)
    {
      case CM_DM_NSC_REPOSITORY_NWNAME_ID:
           CM_CBK_DEBUG_PRINT ("nw_name:%s",(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p);
           of_strncpy (key_info->vn_name,(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p,
                       pMandParams->iv_pairs[uiMandParamCnt].value_length);

           break;
      case CM_DM_NSC_REPOSITORY_TABLE_ID_ID:
           key_info->table_id = of_atoi(pMandParams->iv_pairs[uiMandParamCnt].value_p);
           CM_CBK_DEBUG_PRINT ("table_id :%d",key_info->table_id);
           break;
    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (pMandParams);
  return OF_SUCCESS;
}

int32_t nsc_repository_ucm_setoptparams (struct cm_array_of_iv_pairs *pOptParams,
                                         struct nsc_repository_key* key_info)
{
   uint32_t uiOptParamCnt;

   CM_CBK_DEBUG_PRINT ("Entered");
   for (uiOptParamCnt = 0; uiOptParamCnt < pOptParams->count_ui; uiOptParamCnt++)
   {
     switch (pOptParams->iv_pairs[uiOptParamCnt].id_ui)
     {
     }
   }
  return TSC_SUCCESS;
}

int32_t nsc_repository_ucm_getparams (struct nsc_repository_key   *key_info_p,
                                      struct nsc_repository_entry *repository_entry_info_p,
                                      struct cm_array_of_iv_pairs * result_iv_pairs_p)
{
   uint32_t uindex_i = 0;
   char buf[128] = "";
   
   #define CM_NSC_REPOSITORY_COUNT   25
   result_iv_pairs_p->iv_pairs =(struct cm_iv_pair *) of_calloc (CM_NSC_REPOSITORY_COUNT, sizeof (struct cm_iv_pair));
   if (result_iv_pairs_p->iv_pairs == NULL)
   {
     CM_CBK_DEBUG_PRINT("Memory allocation failed for result_iv_pairs_p->iv_pairs");
     free(repository_entry_info_p);
     return OF_FAILURE;
   }

   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i], CM_DM_NSC_REPOSITORY_NWNAME_ID,
                   CM_DATA_TYPE_STRING, key_info_p->vn_name);
   CM_CBK_DEBUG_PRINT("name : %s",result_iv_pairs_p->iv_pairs[uindex_i].value_p);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   cm_inet_ntoal(repository_entry_info_p->src_ip, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_SIP_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;
   
   of_memset(buf, 0, sizeof(buf));
   of_sprintf(buf,"%s",repository_entry_info_p->local_switch_name_p);
   CM_CBK_DEBUG_PRINT("local switch name : %s",repository_entry_info_p->local_switch_name_p);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i], CM_DM_NSC_REPOSITORY_LOCAL_SWITCH_NAME_ID,
                   CM_DATA_TYPE_STRING, buf);
   CM_CBK_DEBUG_PRINT("name : %s",result_iv_pairs_p->iv_pairs[uindex_i].value_p);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   cm_inet_ntoal(repository_entry_info_p->dst_ip, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_DIP_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->dst_port, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_DSTPORT_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;


   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->src_port, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_SRCPORT_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;


   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->protocol, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_PROTOCOL_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->vlan_id_pkt, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_PKT_VLAN_ID_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->match_vlan_id, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_MATCH_VLAN_ID_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->next_vlan_id, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_NEXT_VLAN_ID_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->out_port_no, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_OUT_PORT_NO_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->nid, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_NID_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
  // of_ltoa (repository_entry_info_p->dp_handle, buf);
   sprintf(buf,"%llx",repository_entry_info_p->dp_handle);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_DP_HANDLE_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;


   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->src_mac, buf);
   sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",repository_entry_info_p->src_mac[0],repository_entry_info_p->src_mac[1],repository_entry_info_p->src_mac[2],repository_entry_info_p->src_mac[3],repository_entry_info_p->src_mac[4],repository_entry_info_p->src_mac[5]);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_SRCMAC_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->dst_mac, buf);
   sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",repository_entry_info_p->dst_mac[0],repository_entry_info_p->dst_mac[1],repository_entry_info_p->dst_mac[2],repository_entry_info_p->dst_mac[3],repository_entry_info_p->dst_mac[4],repository_entry_info_p->dst_mac[5]);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_DSTMAC_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;


   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->local_in_mac, buf);
   sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",repository_entry_info_p->local_in_mac[0],repository_entry_info_p->local_in_mac[1],repository_entry_info_p->local_in_mac[2],repository_entry_info_p->local_in_mac[3],repository_entry_info_p->local_in_mac[4],repository_entry_info_p->local_in_mac[5]);

   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_LOCAL_IN_MAC_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(buf, 0, sizeof(buf));
   of_itoa (repository_entry_info_p->local_out_mac, buf);
   sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",repository_entry_info_p->local_out_mac[0],repository_entry_info_p->local_out_mac[1],repository_entry_info_p->local_out_mac[2],repository_entry_info_p->local_out_mac[3],repository_entry_info_p->local_out_mac[4],repository_entry_info_p->local_out_mac[5]);

   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_NSC_REPOSITORY_LOCAL_OUT_MAC_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;


   result_iv_pairs_p->count_ui = uindex_i;
   return OF_SUCCESS;
}

int32_t nsc_repository_getexactrec (struct cm_array_of_iv_pairs * keys_arr_p,
                                   struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p)
{
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;
  int32_t retval;
  retval = nsc_get_exact_repository_entry();
  if(retval == OF_FAILURE)
    return OF_FAILURE;

  result_iv_pairs_p = (struct cm_array_of_iv_pairs *)calloc(1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    return OF_FAILURE;
  }

 *array_of_iv_pair_arr_p = result_iv_pairs_p;
  return OF_SUCCESS;
}



int32_t nsc_repository_verifycfg (struct cm_array_of_iv_pairs *key_iv_pairs_p,
                        uint32_t command_id, struct cm_app_result ** result_p)

{
  return TSC_SUCCESS;
}

