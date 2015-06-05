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
 * File name: ns2nsidcbk.c
 * Author: G Atmaram <B38856@freescale.com>
 * Date:   08/23/2013
 * Description: 
 * 
 */
/* File  :      tsc_cli_init.c
 * Author:      Vinod Sarma P<b46178@freescale.com>
 * Date:        08/13/2014
 * Description: 
 */

#ifdef OF_CM_SUPPORT


#include"openflow.h"
#include"cbcmninc.h"
#include "cntrucminc.h"
#include "cntrlappcmn.h"
#include "cmgutil.h"
#include"of_multipart.h"
#include"cmreg.h"

int32_t of_tsc_cbk_init();

void cntlr_shared_lib_init()
{
   int32_t retval=OF_SUCCESS;

   CM_CBK_DEBUG_PRINT("Loading  Network Element Mapper");

   do
   {
      /** Initialize TSC CLI  */
      retval=of_tsc_cbk_init();
      if(retval != OF_SUCCESS)
      {
         CM_CBK_DEBUG_PRINT( "TSC CBK initialization failed");
         break;
      }

   }while(0);

   return;
}

int32_t of_tsc_cbk_init()
{

   int32_t status=OF_SUCCESS;
   int32_t retval = OF_FAILURE;


   CM_CBK_DEBUG_PRINT( "entered");

   do
   {

      retval = tsc_selectors_init();
      if(retval != OF_SUCCESS)
      {
         CM_CBK_DEBUG_PRINT( "Selectors cbk  initialization failed");
         status = OF_FAILURE;
         break;
      }
      retval = nsc_repository_init();
      if(retval != OF_SUCCESS)
      {
         CM_CBK_DEBUG_PRINT( "Repository cbk for table 1 and 2 initialization failed");
         status = OF_FAILURE;
         break;
      }
      retval = nsc_tbl_3_repository_init();
      if(retval != OF_SUCCESS)
      {
         CM_CBK_DEBUG_PRINT( "Repository cbk for tabel 3 initialization failed");
         status = OF_FAILURE;
         break;
      }


   }while(0);

   return status;

}

void cntlr_shared_lib_deinit()
{

   CM_CBK_DEBUG_PRINT("UnLoading  tsc  cbk");

   do
   {
      /** De-Initialize NEM Infrastructure */
#if 0
      retval=of_nem_cbk_init();
      if(retval != OF_SUCCESS)
      {
         CM_CBK_DEBUG_PRINT( "Network Element Mapper CBK initialization failed");
         status= OF_FAILURE;
         break;
      }
#endif
   }while(0);

   return;
}
#endif /* OF_CM_SUPPORT */
