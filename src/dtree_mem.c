/*
 * Copyright 2012 The Weather Channel
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


#include "dtree_client.h"


packed_ptr dtree_alloc_node(dtree_dt_index*);
static void *dtree_alloc_mem_align(dtree_dt_index*,size_t,size_t);


//allocates a new dtree node
packed_ptr dtree_alloc_node(dtree_dt_index *h)
{
    packed_ptr pp;
    dtree_dt_node *p;

    if(h->node_count>=(h->slab_count*DTREE_DT_SLAB_SIZE))
    {
        if(h->slab_count>=DTREE_DT_MAX_SLABS)
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"NALLOC: ERROR: all slabs allocated\n");
            return (packed_ptr)0;
        }
        
        dtree_printd(DTREE_PRINT_INITDTREE,"NALLOC: allocating new SLAB\n");
        
        p=calloc(DTREE_DT_SLAB_SIZE,sizeof(dtree_dt_node));
        
        if(!p)
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"NMALLOC: ERROR: calloc memory allocation failure: %d\n",DTREE_DT_SLAB_SIZE);
            return (packed_ptr)0;
        }
        
        h->size+=DTREE_DT_SLAB_SIZE*sizeof(dtree_dt_node);
        h->slabs[h->slab_count]=p;
        h->slab_count++;
        h->slab_pos=0;
    }
    
    //return next open node on slab
    p=h->slabs[h->slab_count-1];
    
    if(!p)
        return (packed_ptr)0;
    
    p=&p[h->slab_pos];
    pp=DTREE_DT_GENPP(p,h->slab_count-1,h->slab_pos);
    
    dtree_printd(DTREE_PRINT_INITDTREE,"NALLOC: node %zu off of slab %zu p(%p) pp(%p)\n",
             h->slab_pos,h->slab_count,p,pp);
    
    h->slab_pos++;
    h->node_count++;
    
    //bypass null packed_ptr
    if(!pp)
        return dtree_alloc_node(h);
    
    return pp;
}

//allocates memory
void *dtree_alloc_mem(dtree_dt_index *h,size_t len)
{
    return dtree_alloc_mem_align(h,len,sizeof(h));
}

//aligned memory allocation
static void *dtree_alloc_mem_align(dtree_dt_index *h,size_t len,size_t align)
{
    char *ret;
    size_t olen=0;
    size_t nlen;
    
    if(h->dc_slab_pos%align && !(align&(align-1)))
    {
        olen=((h->dc_slab_pos+align-1) & (~(align-1))) - h->dc_slab_pos;
        dtree_printd(DTREE_PRINT_INITDTREE,"DMALLOC: unaligned(%zu) offset detected %zu => %zu\n",
                 align,h->dc_slab_pos,h->dc_slab_pos+olen);
    }
    
    nlen=len+olen;
    
    if(h->dc_slab_count>=DTREE_M_MAX_SLABS)
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"DMALLOC: ERROR: out of slabs: %zu\n",h->dc_slab_count);
        return NULL;
    }
    
    if(!h->dc_slabs[h->dc_slab_count])
    {
        //alloc new slab
        h->dc_slabs[h->dc_slab_count]=calloc(DTREE_M_SLAB_SIZE,sizeof(char));
        
        if(!h->dc_slabs[h->dc_slab_count])
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"DMALLOC: ERROR: calloc memory allocation failure: %d\n",DTREE_M_SLAB_SIZE);
            return NULL;
        }
        
        h->dc_slab_pos=0;
        h->size+=DTREE_M_SLAB_SIZE*sizeof(char);
    }

    if(nlen>DTREE_M_SLAB_SIZE || !len)
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"DMALLOC: ERROR: size too large: %zu\n",len);
        return NULL;
    }
    
    if(h->dc_slab_pos+nlen>DTREE_M_SLAB_SIZE)
    {
        //move to next slab
        h->dc_slab_count++;
        h->dc_slab_pos=0;
        return dtree_alloc_mem_align(h,len,align);
    }
    
    //alloc memory
    ret=h->dc_slabs[h->dc_slab_count]+h->dc_slab_pos+olen;
    
    if(((size_t)ret%align))
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"DMALLOC: ERROR: misalignment\n");
        return NULL;
    }
    
    h->dc_count++;
    
    dtree_printd(DTREE_PRINT_INITDTREE,"DMALLOC: memory: %zu,%zu+%zu,%zu(%p) align: %zu\n",
             h->dc_slab_count,h->dc_slab_pos,olen,len,ret,align);
    
    h->dc_slab_pos=(ret-h->dc_slabs[h->dc_slab_count])+len;
    
    return (void*)ret;
}

//allocates a string, reuses if in cache
char *dtree_alloc_string(dtree_dt_index *h,const char *s,int len)
{
    char *ret;
    int i;
    
    if(!s)
        return NULL;
    
    for(i=0;h->dc_cache[i] && i<DTREE_M_LOOKUP_CACHE;i++)
    {
        if(!strncmp(s,h->dc_cache[i],len) && !h->dc_cache[i][len])
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"SALLOC: cache hit %d: '%s'\n",i,h->dc_cache[i]);
            return h->dc_cache[i];
        }
    }
    
    ret=dtree_alloc_mem_align(h,(len+1)*sizeof(char),sizeof(char));
    
    //legacy
    if(!ret)
        return NULL;
    
    strncpy(ret,s,len);
    
    if(!h->dc_cache[DTREE_M_LOOKUP_CACHE-1])
    {
        for(i=0;i<DTREE_M_LOOKUP_CACHE;i++)
        {
            if(!h->dc_cache[i])
            {
                dtree_printd(DTREE_PRINT_INITDTREE,"SALLOC: cache store %d: '%s'\n",i,ret);
                h->dc_cache[i]=ret;
                break;
            }
        }
    }
    
    dtree_printd(DTREE_PRINT_INITDTREE,"SALLOC: str allocated: '%s'(%p)\n",ret,ret);
    
    return ret;
}

//inits the head
void dtree_init_index(dtree_dt_index *h)
{
    memset(h,0,sizeof(dtree_dt_index));
}

//frees memory
void dtree_free(dtree_dt_index *h)
{
    size_t i;
    
    for(i=0;i<h->slab_count;i++)
        free(h->slabs[i]);
    
    for(i=0;i<DTREE_M_MAX_SLABS;i++)
    {
        if(h->dc_slabs[i])
            free(h->dc_slabs[i]);
    }
    
    dtree_init_index(h);
}
