/*
 * Copyright 2012 The Weather Channel
 * Copyright 2013 Reza Naghibi
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */


#ifndef _DTREE_H_INCLUDED_
#define _DTREE_H_INCLUDED_


#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef _DTREE_NO_TIMESPEC
#define __timespec_defined 1
#else
#include <time.h>
#include <sys/time.h>
#endif

#include <stdlib.h>


//logging and testing
#define DTREE_TEST_UALOOKUP     1
#define DTREE_PERF_WALKING      0
#define DTREE_DEBUG_LOGGING     0

#define DTREE_PRINT_GENERIC     (1 << 0)
#define DTREE_PRINT_CLASSIFY    (1 << 1)
#define DTREE_PRINT_INITDTREE   (1 << 2)

//what messages to print, DEBUG_LOGGING needs to be enabled
#define DTREE_PRINT_ENABLED     (DTREE_PRINT_GENERIC|DTREE_PRINT_CLASSIFY|DTREE_PRINT_INITDTREE)


//if enabled, this packs system pointers into 16/32 bits
#ifndef DTREE_DT_PACKED
#define DTREE_DT_PACKED         1
#endif

//if PACKED is enabled, uses 16bit pointers, otherwise 32bit
#ifndef DTREE_DT_PACKED_16
#define DTREE_DT_PACKED_16      1
#endif

#if DTREE_DT_PACKED

#if DTREE_DT_PACKED_16
//16bit pointer, 6 bits max for SLABS (max=64)
typedef unsigned short int packed_ptr;
#define DTREE_DT_MAX_SLABS      64
#else
//32bit pointer, 22 bits max for SLABS (max=4194304)
typedef unsigned int packed_ptr;
#define DTREE_DT_MAX_SLABS      4096
#endif /*DTREE_DT_PACKED_16*/

#define DTREE_DT_SLAB_SIZE      1024
#define DTREE_DT_GETPPH(PP)     ((PP)>>10)
#define DTREE_DT_GETPPL(PP)     ((PP)&0x3FF)
#define DTREE_DT_GETPP(H,PP)    (&((dtree_dt_node*)(H)->slabs[DTREE_DT_GETPPH(PP)])[DTREE_DT_GETPPL(PP)])
#define DTREE_DT_GENPP(P,H,L)   (((H)<<10)|(L))

#define DTREE_DT_PTR_TYPE       "v"

#else /*NOT DTREE_DT_PACKED*/

//it is safe to change these values
#define DTREE_DT_MAX_SLABS      4096
#define DTREE_DT_SLAB_SIZE      4096

#define DTREE_DT_GETPP(H,PP)    ((dtree_dt_node*)(PP))
#define DTREE_DT_GENPP(P,H,L)   ((packed_ptr)(P))

#define DTREE_DT_PTR_TYPE       "n"

#endif /*DTREE_DT_PACKED*/


#define DTREE_DT_PTR_SIZE       (sizeof(packed_ptr)*8)


//flags, constants, etc
#define DTREE_DT_FLAG_TOKEN     (1 << 0)
#define DTREE_DT_FLAG_STRONG    (1 << 1)
#define DTREE_DT_FLAG_WEAK      (1 << 2)
#define DTREE_DT_FLAG_NONE      (1 << 3)
#define DTREE_DT_FLAG_CHAIN     (1 << 4)
#define DTREE_DT_FLAG_BCHAIN    (1 << 5)

#define DTREE_DT_FLAG_ACHAIN    (DTREE_DT_FLAG_CHAIN|DTREE_DT_FLAG_BCHAIN)

#define DTREE_S_FLAG_NONE       (1 << 0)
#define DTREE_S_FLAG_PARTIAL    (1 << 1)
#define DTREE_S_FLAG_REGEX      (1 << 2)
#define DTREE_S_FLAG_DUPS       (1 << 3)
#define DTREE_S_FLAG_NOTRIM     (1 << 4)
#define DTREE_S_MAX_CHAIN       10
#define DTREE_S_BE_POS          255
#define DTREE_S_MAX_POS         254
#define DTREE_S_MAX_RANK        255
#define DTREE_S_MIN_DIR         -127
#define DTREE_S_MAX_DIR         127

#define DTREE_M_MAX_SLABS       512
#define DTREE_M_SLAB_SIZE       (1024*512)
#define DTREE_M_SERROR          "serror"
#define DTREE_M_LOOKUP_CACHE    6
#define DTREE_DATA_BUFLEN       128
#define DTREE_DATA_MKEYS        30
#define DTREE_DC_DISTANCE(H,S)  ((int)((S)-((char*)(H)->dc_slabs[0])))

#define DTREE_HASH_TCHARS       ""
#define DTREE_HASH_SEP          (36+sizeof(DTREE_HASH_TCHARS)-1)
#define DTREE_HASH_SCHARS       " -_/\\()."
#define DTREE_HASH_NCOUNT       (DTREE_HASH_SEP+sizeof(DTREE_HASH_SCHARS))
#define DTREE_HASH_ANY          (DTREE_HASH_NCOUNT-1)
#define DTREE_PATTERN_ANY       '.'
#define DTREE_PATTERN_OPTIONAL  '?'
#define DTREE_PATTERN_BEGIN     '^'
#define DTREE_PATTERN_END       '$'
#define DTREE_PATTERN_SET_S     '['
#define DTREE_PATTERN_SET_E     ']'
#define DTREE_PATTERN_GROUP_S   '('
#define DTREE_PATTERN_GROUP_E   ')'
#define DTREE_PATTERN_ESCAPE    '\\'
#define DTREE_PATTERN_DQUOTE    '\"'


//type defs
typedef unsigned char     dtree_flag_f;
typedef unsigned char     dtree_pos_f;
typedef unsigned char     dtree_rank_f;
typedef char              dtree_dir_f;


//dtree dtnode
typedef struct
{
    char                  data;
    dtree_flag_f          flags;

    dtree_pos_f           pos;
    dtree_rank_f          rank;
    dtree_dir_f           dir;

#if DTREE_DT_PACKED
    packed_ptr            nodes[DTREE_HASH_NCOUNT];
    packed_ptr            curr;
    packed_ptr            prev;
    packed_ptr            dup;
#else
    struct dtree_dt_node  *nodes[DTREE_HASH_NCOUNT];
    struct dtree_dt_node  *curr;
    struct dtree_dt_node  *prev;
    struct dtree_dt_node  *dup;
#endif
    
    void*                 payload;
    void*                 cparam;
}
dtree_dt_node;

#if !DTREE_DT_PACKED
typedef struct dtree_dt_node* packed_ptr;
#endif


//master dtree node
typedef struct
{
    dtree_flag_f          sflags;
    
    size_t                node_count;
    size_t                size;
    
    size_t                slab_count;
    size_t                slab_pos;
    
    size_t                dc_slab_count;
    size_t                dc_slab_pos;
    size_t                dc_count;
    
    dtree_dt_node         *head;
    
    char                  *comment;
    
    dtree_dt_node         *slabs[DTREE_DT_MAX_SLABS];
    char                  *dc_slabs[DTREE_M_MAX_SLABS];
    char                  *dc_cache[DTREE_M_LOOKUP_CACHE];
}
dtree_dt_index;


//add_entry struct
typedef struct
{
    void                  *data;
    void                  *param;

    dtree_flag_f          flags;

    dtree_pos_f           pos;
    dtree_rank_f          rank;
    dtree_dir_f           dir;
}
dtree_dt_add_entry;


void dtree_init_index(dtree_dt_index*);

int dtree_add_entry(dtree_dt_index *,const char*,dtree_dt_add_entry*);

void *dtree_get(const dtree_dt_index*,const char*,dtree_flag_f);
const dtree_dt_node *dtree_get_node(const dtree_dt_index*,const char*,dtree_flag_f,dtree_pos_f);
const dtree_dt_node *dtree_get_flag(const dtree_dt_index*,const dtree_dt_node*,dtree_flag_f,dtree_pos_f);
dtree_flag_f dtree_get_flags(const dtree_dt_index*,const dtree_dt_node*,dtree_pos_f);

char *dtree_alloc_string(dtree_dt_index*,const char*,int);
void *dtree_alloc_mem(dtree_dt_index*,size_t);

void dtree_free(dtree_dt_index*);

long dtree_print(const dtree_dt_index*,const char*(*f)(void*));
void dtree_printd(int,const char*,...);


#ifdef _DTREE_NO_TIMESPEC
struct timespec
{
    long tv_sec;
    long tv_nsec;
}
timespec_t;
#endif


#endif /* _DTREE_H_INCLUDED_ */
