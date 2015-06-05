/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*           Copyright 1998 - 2001,                                       */
/*             Intoto, Inc, SANTACLARA, CA, USA                           */
/*                      ALL RIGHTS RESERVED                               */
/*                                                                        */
/*   Permission is hereby granted to licensees of Intoto,                 */
/*   Inc. products to use or abstract this computer program for the       */
/*   sole purpose of implementing a product based on                      */
/*   Intoto, Inc. products.No other rights to reproduce,                  */
/*   use,or disseminate this computer program, whether in part or in      */
/*   whole, are granted.                                                  */
/*                                                                        */
/*   Intoto, Inc. makes no representation or warranties                   */
/*   with respect to the performance of this computer program, and        */
/*   specifically disclaims any responsibility for any damages,           */
/*   special or consequential, connected with the use of this program.    */
/*                                                                        */
/************************************************************************ */
/**************************************************************************/
/*  File         : bobhash.h                                              */
/*  Description  : Dob Hash algorithms file.                              */
/*                                                                        */
/*  Version      Date        Author         Change Description            */
/*  -------    --------      ------        ----------------------         */
/*  1.0        11-10-2005   premaca          Initial Development          */
/**************************************************************************/
#ifndef _DOBHASH_H_
#define _DOBHASH_H_
#ifdef __cplusplus
extern "C" {
#endif

#define CNTLR_DOBBS_MIX(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/* Hash computation for a string/array 
 */
#define CNTLR_DOBBS_HASH( pStr, length, initval, hashmask, hash) \
{ \
\
  register uint32_t temp_a, temp_b, temp_c,temp_len;  \
  T_UCHAR8 *k = pStr; \
\
  /* Set up the internal state */ \
  temp_len = length;\
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/\
\
  /*---------------------------------------- handle most of the key */\
  while (temp_len >= 12) \
  { \
    temp_a += (k[0] +((uint32_t)k[1]<<8) +((uint32_t)k[2]<<16) +\
              ((uint32_t)k[3]<<24));\
    temp_b += (k[4] +((uint32_t)k[5]<<8) +((uint32_t)k[6]<<16) +\
              ((uint32_t)k[7]<<24));\
    temp_c += (k[8] +((uint32_t)k[9]<<8) +((uint32_t)k[10]<<16) +\
              ((uint32_t)k[11]<<24));\
    CNTLR_DOBBS_MIX(temp_a,temp_b,temp_c);\
    k += 12; temp_len -= 12;\
  }\
\
  /*------------------------------------- handle the last 11 bytes */\
  temp_c += length;\
  switch(temp_len)              /* all the case statements fall through */\
  {\
    case 11: temp_c+=((uint32_t)k[10]<<24);\
    case 10: temp_c+=((uint32_t)k[9]<<16);\
    case 9 : temp_c+=((uint32_t)k[8]<<8);\
             /* the first byte of c is reserved for the length */\
    case 8 : temp_b+=((uint32_t)k[7]<<24);\
    case 7 : temp_b+=((uint32_t)k[6]<<16);\
    case 6 : temp_b+=((uint32_t)k[5]<<8);\
    case 5 : temp_b+=k[4];\
    case 4 : temp_a+=((uint32_t)k[3]<<24);\
    case 3 : temp_a+=((uint32_t)k[2]<<16);\
    case 2 : temp_a+=((uint32_t)k[1]<<8);\
    case 1 : temp_a+=k[0];\
             /* case 0: nothing left to add */\
  }\
  CNTLR_DOBBS_MIX(temp_a,temp_b,temp_c);\
  /*-------------------------------------------- report the result */\
  hash = temp_c & (hashmask-1);\
}

/* Hash computation for a string/array and 4 words 
 */
#define CNTLR_DOBBS_HASH_WORDS4(word_a, word_b, word_c, word_d,pStr, length, initval, hashmask, hash) \
{ \
\
  register uint32_t temp_a, temp_b, temp_c,temp_len;  \
  T_UCHAR8 *k = pStr; \
\
  /* Set up the internal state */ \
  temp_len = length;\
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/\
\
  /*---------------------------------------- handle most of the key */\
  while (temp_len >= 12) \
  { \
    temp_a += (k[0] +((uint32_t)k[1]<<8) +((uint32_t)k[2]<<16) +\
              ((uint32_t)k[3]<<24));\
    temp_b += (k[4] +((uint32_t)k[5]<<8) +((uint32_t)k[6]<<16) +\
              ((uint32_t)k[7]<<24));\
    temp_c += (k[8] +((uint32_t)k[9]<<8) +((uint32_t)k[10]<<16) +\
              ((uint32_t)k[11]<<24));\
    CNTLR_DOBBS_MIX(temp_a,temp_b,temp_c);\
    k += 12; temp_len -= 12;\
  }\
\
  /*------------------------------------- handle the last 11 bytes */\
  temp_c += length;\
  switch(temp_len)              /* all the case statements fall through */\
  {\
    case 11: temp_c+=((uint32_t)k[10]<<24);\
    case 10: temp_c+=((uint32_t)k[9]<<16);\
    case 9 : temp_c+=((uint32_t)k[8]<<8);\
             /* the first byte of c is reserved for the length */\
    case 8 : temp_b+=((uint32_t)k[7]<<24);\
    case 7 : temp_b+=((uint32_t)k[6]<<16);\
    case 6 : temp_b+=((uint32_t)k[5]<<8);\
    case 5 : temp_b+=k[4];\
    case 4 : temp_a+=((uint32_t)k[3]<<24);\
    case 3 : temp_a+=((uint32_t)k[2]<<16);\
    case 2 : temp_a+=((uint32_t)k[1]<<8);\
    case 1 : temp_a+=k[0];\
             /* case 0: nothing left to add */\
  }\
  CNTLR_DOBBS_MIX(temp_a,temp_b,temp_c);\
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  /*-------------------------------------------- report the result */\
  hash = temp_c & (hashmask-1);\
}

/* Hash computation for 2 words */
#define CNTLR_DOBBS_WORDS2(word_a, word_b, initval, \
                         hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS3(word_a, word_b, word_c, initval, \
                         hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS4(word_a, word_b, word_c, word_d, initval, \
                         hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS5(word_a, word_b, word_c, word_d, word_e, initval, \
                         hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  temp_b += word_e; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS6(word_a, word_b, word_c, word_d, word_e, word_f, \
                         initval, hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  temp_b += word_e; \
  temp_c += word_f; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS7(word_a, word_b, word_c, word_d, word_e, word_f, \
                         word_g, initval, hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  temp_b += word_e; \
  temp_c += word_f; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_g; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS8(word_a, word_b, word_c, word_d, word_e, word_f, word_g, word_h, \
                           initval, hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  temp_b += word_e; \
  temp_c += word_f; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_g; \
  temp_b += word_h; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#define CNTLR_DOBBS_WORDS12(word_a, word_b, word_c, word_d, word_e, word_f, word_g, word_h, \
                            word_i, word_j,word_k,word_l,initval, hashmask, hash) \
{ \
  register uint32_t temp_a, temp_b, temp_c; \
  temp_a = temp_b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */\
  temp_c = initval;         /* random value*/ \
\
  temp_a += word_a; \
  temp_b += word_b; \
  temp_c += word_c; \
\
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_d; \
  temp_b += word_e; \
  temp_c += word_f; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_g; \
  temp_b += word_h; \
  temp_c += word_i; \
  CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  temp_a += word_j; \
  temp_b += word_k; \
  temp_b += word_l; \
 CNTLR_DOBBS_MIX(temp_a, temp_b, temp_c); \
\
  hash = temp_c & (hashmask-1); \
}

#ifdef __cplusplus
}
#endif
#endif /* _DOBHASH_H_ */
