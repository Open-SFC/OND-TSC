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


int32_t tsc_selectors_init(void);


int32_t tsc_selectors_addrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p);

int32_t tsc_selectors_modrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p);

int32_t tsc_selectors_getfirstnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p);

int32_t tsc_selectors_getnextnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        struct cm_array_of_iv_pairs *prev_record_key_p,  uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** next_n_record_data_p);

int32_t tsc_selectors_getexactrec (struct cm_array_of_iv_pairs * keys_arr_p,
                                   struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p);


int32_t tsc_selectors_verifycfg (struct cm_array_of_iv_pairs *key_iv_pairs_p,
                        uint32_t command_id, struct cm_app_result ** result_p);


int32_t tsc_selectors_ucm_setmandparams (struct cm_array_of_iv_pairs *pMandParams,
                                         struct nsc_selector_node_key* key_info);


int32_t tsc_selectors_ucm_setoptparams (struct cm_array_of_iv_pairs *pOptParams,
                                        struct nsc_selector_node_key* key_info);


int32_t tsc_selectors_ucm_getparams (struct nsc_selector_node_key* key_info,
                                     struct tsc_cnbind_entry_info *cnbind_entry_info_p,
                                     struct cm_array_of_iv_pairs * result_iv_pairs_p);



struct cm_dm_app_callbacks tsc_selector_ucm_callbacks = {
  NULL,
  tsc_selectors_addrec,
  tsc_selectors_modrec,
  NULL,
  NULL,
  tsc_selectors_getfirstnrecs,
  tsc_selectors_getnextnrecs,
  tsc_selectors_getexactrec,
  tsc_selectors_verifycfg,
  NULL
};


uint64_t saferef = 0;
uint64_t relative_saferef = 0;

struct nsc_selector_node_key  *global_key_p = NULL;


int32_t tsc_selectors_init(void)
{
  cm_register_app_callbacks (CM_ON_DIRECTOR_TSC_SELECTORS_APPL_ID,&tsc_selector_ucm_callbacks);
  return OF_SUCCESS;
}


int32_t tsc_selectors_addrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;
  struct nsc_selector_node_key  *key_p = NULL;  

  int32_t retval = OF_FAILURE;
  CM_CBK_DEBUG_PRINT ("Entered addrec"); 
  key_p = (struct nsc_selector_node_key*)calloc(1,sizeof(struct nsc_selector_node_key));
  key_p->vn_name = (char*)calloc(1,128);
 

  if(tsc_selectors_ucm_setmandparams(pMandParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
  

  if(tsc_selectors_ucm_setoptparams(pOptParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_OPT_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
 
  global_key_p = key_p;
 
  retval = tsc_add_selector_key(key_p);
  if(retval != TSC_SUCCESS)
  { 
    CM_CBK_DEBUG_PRINT ("Add Failed");
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
  return OF_SUCCESS;
}


int32_t tsc_selectors_modrec(void * config_trans_p,
                             struct cm_array_of_iv_pairs * pMandParams,
                             struct cm_array_of_iv_pairs * pOptParams,
                             struct cm_app_result ** result_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;
  struct nsc_selector_node_key  *key_p = NULL;  

  int32_t retval = OF_FAILURE;
  CM_CBK_DEBUG_PRINT ("Entered addrec"); 
  key_p = (struct nsc_selector_node_key*)calloc(1,sizeof(struct nsc_selector_node_key));
  key_p->vn_name = (char*)calloc(1,128);


  if(tsc_selectors_ucm_setmandparams(pMandParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }


  if(tsc_selectors_ucm_setoptparams(pOptParams,key_p) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
    fill_app_result_struct (&app_result, NULL, CM_GLU_SET_OPT_PARAM_FAILED);
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
 
  global_key_p = key_p;
 
  retval = tsc_add_selector_key(key_p);
  if(retval != TSC_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Add Failed");
    free(key_p->vn_name);
    free(key_p);
    return OF_FAILURE;
  }
  return OF_SUCCESS;
}





int32_t tsc_selectors_getfirstnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;

  struct nsc_selector_node_key  *key_p = NULL;
  struct nsc_selector_node_key  *key_info_p = NULL;
  struct tsc_cnbind_entry_info  *cnbind_node_p = NULL;
  uint32_t uiRecCount = 0;
  int32_t  retval = OF_FAILURE;

  key_p = (struct nsc_selector_node_key*)calloc(1,sizeof(struct nsc_selector_node_key));
  key_p->vn_name = (char*)calloc(1,128);

  cnbind_node_p = (struct tsc_cnbind_entry_info*)calloc(1,sizeof(struct tsc_cnbind_entry_info));
  cnbind_node_p->nschainset_name_p = (char*)calloc(1,128);
  
  retval = tsc_get_first_matching_cnbind_entry(key_info_p,&saferef,cnbind_node_p);
  if(retval != TSC_SUCCESS)
  {
     CM_CBK_DEBUG_PRINT("Get first failed");
     free(key_p->vn_name);
     free(key_p);
     free(cnbind_node_p->nschainset_name_p);
     free(cnbind_node_p);
     return OF_FAILURE;
  }

  result_iv_pairs_p = (struct cm_array_of_iv_pairs *)calloc(1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    free(key_p->vn_name);
    free(key_p);
    free(cnbind_node_p->nschainset_name_p);
    free(cnbind_node_p);
    return OF_FAILURE;
  }


  tsc_selectors_ucm_getparams (global_key_p,cnbind_node_p, &result_iv_pairs_p[uiRecCount]);
  uiRecCount++;
 *array_of_iv_pair_arr_p = result_iv_pairs_p;
  return OF_SUCCESS;
}


int32_t tsc_selectors_getnextnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        struct cm_array_of_iv_pairs *prev_record_key_p,  uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** next_n_record_data_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;
  struct cm_array_of_iv_pairs   *pMandParams = NULL;
  struct cm_array_of_iv_pairs   *pOptParams  = NULL;
  struct nsc_selector_node_key  *key_p = NULL;
  struct nsc_selector_node_key  *key_info_p = NULL;
  struct tsc_cnbind_entry_info  *cnbind_node_p = NULL;
  uint32_t uiRecCount = 0;
  int32_t  retval = OF_FAILURE;

  key_p = (struct nsc_selector_node_key*)calloc(1,sizeof(struct nsc_selector_node_key));
  key_p->vn_name = (char*)calloc(1,128);

  cnbind_node_p = (struct tsc_cnbind_entry_info*)calloc(1,sizeof(struct tsc_cnbind_entry_info));
  cnbind_node_p->nschainset_name_p = (char*)calloc(1,128);
  
  retval = tsc_get_next_matching_cnbind_entry(key_info_p,saferef,&relative_saferef,cnbind_node_p);
  if(retval != TSC_SUCCESS)
  {
     CM_CBK_DEBUG_PRINT("Get first failed");
     free(key_p->vn_name);
     free(key_p);
     free(cnbind_node_p->nschainset_name_p);
     free(cnbind_node_p);
     return OF_FAILURE;
  }
  CM_CBK_DEBUG_PRINT("saferef : %llx",saferef);
  CM_CBK_DEBUG_PRINT("rel_saferef : %llx",relative_saferef);
  saferef = relative_saferef;
  relative_saferef = 0;

  CM_CBK_DEBUG_PRINT("saferef : %llx",saferef);
  CM_CBK_DEBUG_PRINT("rel_saferef : %llx",relative_saferef);
  result_iv_pairs_p = (struct cm_array_of_iv_pairs *)calloc(1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    free(key_p->vn_name);
    free(key_p);
    free(cnbind_node_p->nschainset_name_p);
    free(cnbind_node_p);
    return OF_FAILURE;
  }


  tsc_selectors_ucm_getparams (global_key_p,cnbind_node_p, &result_iv_pairs_p[uiRecCount]);
  uiRecCount++;
  *next_n_record_data_p = result_iv_pairs_p;
  *count_p = uiRecCount;
  return OF_SUCCESS;
}


int32_t tsc_selectors_getexactrec (struct cm_array_of_iv_pairs * keys_arr_p,
                                   struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p)
{
  struct cm_app_result          *app_result = NULL;
  struct cm_array_of_iv_pairs   *result_iv_pairs_p = NULL;
  struct cm_array_of_iv_pairs   *pMandParams = NULL;
  struct cm_array_of_iv_pairs   *pOptParams = NULL;

  struct nsc_selector_node_key  *key_p = NULL;
  struct nsc_selector_node_key  *key_info_p = NULL;
  struct tsc_cnbind_entry_info  *cnbind_node_p = NULL;
  uint32_t uiRecCount = 0;
  int32_t  retval = OF_FAILURE;
 

  key_p = (struct nsc_selector_node_key*)calloc(1,sizeof(struct nsc_selector_node_key));
  key_p->vn_name = (char*)calloc(1,128);

  cnbind_node_p = (struct tsc_cnbind_entry_info*)calloc(1,sizeof(struct tsc_cnbind_entry_info));
  cnbind_node_p->nschainset_name_p = (char*)calloc(1,128);
  
  retval = tsc_get_exact_matching_cnbind_entry(key_p,global_key_p,cnbind_node_p);
  if(retval != TSC_SUCCESS)
  {
     CM_CBK_DEBUG_PRINT("Get exact failed");
     free(key_p->vn_name);
     free(key_p);
     free(cnbind_node_p->nschainset_name_p);
     free(cnbind_node_p);
     return OF_FAILURE;
  }

  result_iv_pairs_p = (struct cm_array_of_iv_pairs *)calloc(1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    free(key_p->vn_name);
    free(key_p);
    free(cnbind_node_p->nschainset_name_p);
    free(cnbind_node_p);
    return OF_FAILURE;
  }

  #if 0
  tsc_selectors_ucm_getparams (global_key_p,cnbind_node_p, result_iv_pairs_p);
  #endif
 *array_of_iv_pair_arr_p = result_iv_pairs_p;
  free(key_p->vn_name);
  free(key_p);
  free(cnbind_node_p->nschainset_name_p);
  free(cnbind_node_p);
  return OF_SUCCESS;
}





int32_t tsc_selectors_ucm_setmandparams (struct cm_array_of_iv_pairs *pMandParams,
                                         struct nsc_selector_node_key* key_info) 
{
  uint32_t uiMandParamCnt;
  uint64_t idpId;

  CM_CBK_DEBUG_PRINT ("Entered");
   
  for (uiMandParamCnt = 0; uiMandParamCnt < pMandParams->count_ui;
       uiMandParamCnt++)
  {
    switch (pMandParams->iv_pairs[uiMandParamCnt].id_ui)
    {
      case CM_DM_TSC_SELECTOR_NWNAME_ID:
           CM_CBK_DEBUG_PRINT ("nw_name:%s",(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p);
           of_strncpy (key_info->vn_name,(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p,
                       pMandParams->iv_pairs[uiMandParamCnt].value_length);

        break;
      
    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (pMandParams);
  return OF_SUCCESS;
}

int32_t tsc_selectors_ucm_setoptparams (struct cm_array_of_iv_pairs *pOptParams,
                                        struct nsc_selector_node_key* key_info)
{
   uint32_t uiOptParamCnt;

   CM_CBK_DEBUG_PRINT ("Entered");
   for (uiOptParamCnt = 0; uiOptParamCnt < pOptParams->count_ui; uiOptParamCnt++)
   {
     switch (pOptParams->iv_pairs[uiOptParamCnt].id_ui)
     {
        case CM_DM_TSC_SELECTOR_SIP_ID:
             CM_CBK_DEBUG_PRINT ("debug");
             if(cm_val_and_conv_aton((char *) pOptParams->iv_pairs[uiOptParamCnt].value_p,&(key_info->src_ip))                 != OF_SUCCESS)
                return OF_FAILURE;
             break;
        case CM_DM_TSC_SELECTOR_DIP_ID:
             CM_CBK_DEBUG_PRINT ("debug");
             if(cm_val_and_conv_aton((char *) pOptParams->iv_pairs[uiOptParamCnt].value_p,&(key_info->dst_ip))                 != OF_SUCCESS)
                return OF_FAILURE;
             break;
        case CM_DM_TSC_SELECTOR_DSTPORT_ID:
             CM_CBK_DEBUG_PRINT ("debug");
             key_info->dst_port = of_atoi(pOptParams->iv_pairs[uiOptParamCnt].value_p);
             break;
        case CM_DM_TSC_SELECTOR_PROTOCOL_ID : 
             CM_CBK_DEBUG_PRINT ("debug");
             key_info->protocol = of_atoi(pOptParams->iv_pairs[uiOptParamCnt].value_p);
             break;
     }
   }
   CM_CBK_PRINT_IVPAIR_ARRAY (pOptParams);
   return OF_SUCCESS;
}

int32_t tsc_selectors_ucm_getparams (struct nsc_selector_node_key *key_info_p,
                                     struct tsc_cnbind_entry_info *cnbind_entry_info_p,
                                     struct cm_array_of_iv_pairs * result_iv_pairs_p)
{

   struct tsc_nwservices *nwservices_scan_p = NULL;
   uint32_t uindex_i = 0;
   char buf[128] = "";
   char tmp_string_1[2048] = "";
   char tmp_string_2[2048] = "";
   char tmp_string_3[2048] = "";
   CM_CBK_DEBUG_PRINT ("Entered");
   #define CM_TSC_SELECTOR_COUNT   100
   
   result_iv_pairs_p->iv_pairs =(struct cm_iv_pair *) of_calloc (CM_TSC_SELECTOR_COUNT, sizeof (struct cm_iv_pair));
   if (result_iv_pairs_p->iv_pairs == NULL)
   {
     CM_CBK_DEBUG_PRINT("Memory allocation failed for result_iv_pairs_p->iv_pairs");
     return OF_FAILURE;
   }
 
   #if 1 
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i], CM_DM_TSC_SELECTOR_NWNAME_ID,
                   CM_DATA_TYPE_STRING, key_info_p->vn_name);
   CM_CBK_DEBUG_PRINT("name : %s",result_iv_pairs_p->iv_pairs[uindex_i].value_p);
   uindex_i++;
   #endif

   of_memset(buf, 0, sizeof(buf));
   cm_inet_ntoal(cnbind_entry_info_p->src_ip, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_SIP_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

  
   of_memset(buf, 0, sizeof(buf));
   cm_inet_ntoal(cnbind_entry_info_p->dst_ip, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_DIP_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

             
   of_memset(buf, 0, sizeof(buf));
   of_itoa (cnbind_entry_info_p->dst_port, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_DSTPORT_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   
   of_memset(buf, 0, sizeof(buf));
   of_itoa (cnbind_entry_info_p->protocol, buf);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_PROTOCOL_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   #if 0
   of_memset(buf, 0, sizeof(buf));
   of_sprintf(buf,"chain : %s",cnbind_entry_info_p->nschain_name_p);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_CHAIN_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;
   #endif

   of_memset(buf, 0, sizeof(buf));
   of_sprintf(buf,"%s",cnbind_entry_info_p->nschainset_name_p);
   FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_CHAINSET_ID,
                    CM_DATA_TYPE_STRING,buf);
   uindex_i++;

   of_memset(tmp_string_1, 0, sizeof(tmp_string_1));
   of_memset(tmp_string_2, 0, sizeof(tmp_string_2));
   of_memset(tmp_string_3, 0, sizeof(tmp_string_3));
   OF_LIST_SCAN(cnbind_entry_info_p->nwservices_info,nwservices_scan_p,
                struct tsc_nwservices *,
                CNBIND_NWSERVICE_LIST_NODE_OFFSET) 
   {
     of_sprintf(tmp_string_3," %s",nwservices_scan_p->nschain_name_p);
     of_sprintf(tmp_string_1," %s",nwservices_scan_p->nwservice_object_name_p);
     of_sprintf(tmp_string_2," %s",nwservices_scan_p->nwservice_instance_name_p);
     
     FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_CHAIN_ID,
                     CM_DATA_TYPE_STRING,tmp_string_3);

     uindex_i++;

     FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_NWSERVICE_ID,
                     CM_DATA_TYPE_STRING,tmp_string_1); 
   
     uindex_i++;
     FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_TSC_SELECTOR_SERVICEINSTANCE_ID,
                     CM_DATA_TYPE_STRING,tmp_string_2);

     uindex_i++;
       
   } 

   result_iv_pairs_p->count_ui = uindex_i;
   return OF_SUCCESS;
}


int32_t tsc_selectors_verifycfg (struct cm_array_of_iv_pairs *key_iv_pairs_p,
                        uint32_t command_id, struct cm_app_result ** result_p)
{
  return OF_SUCCESS;
} 
