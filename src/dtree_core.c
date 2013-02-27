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


#include "dtree_client.h"


static int dtree_add_node(dtree_dt_index*,dtree_dt_node*,char*,void*,flag_f,void*);
static int dtree_set_payload(dtree_dt_index*,dtree_dt_node*,void*,flag_f,void*);
static const dtree_dt_node *dtree_search_node(const dtree_dt_index*,const dtree_dt_node*,const char*);
unsigned int dtree_hash_char(char);

extern packed_ptr dtree_alloc_node(dtree_dt_index*);
extern inline int dtree_node_depth(const dtree_dt_index*,const dtree_dt_node*);


//adds an entry to the tree
int dtree_add_entry(dtree_dt_index *h,const char *key,void *data,flag_f flags,void *param)
{
    char namebuf[DTREE_DATA_BUFLEN+1];
    packed_ptr pp;
    
    if(!key)
        return 0;

    *namebuf='\0';
    namebuf[sizeof(namebuf)-1]='\0';
    
    strncpy(namebuf+1,key,sizeof(namebuf)-1);
    
    if(namebuf[sizeof(namebuf)-1])
        return 0;
    
    dtree_printd(DTREE_PRINT_INITDTREE,"ADD ENTRY: name '%s':%d\n",namebuf+1,flags);
    
    if(!h->head)
    {
        pp=dtree_alloc_node(h);
        
        if(!pp)
        {
            fprintf(stderr,"DTREE: allocation error detected, aborting: %zu\n",h->node_count);
            return -1;
        }
        
        h->head=DTREE_DT_GETPP(h,pp);
        h->head->curr=pp;
        h->head->data=0;
    }
    
    if(!flags)
        flags=DTREE_DT_FLAG_STRONG;
    
    return dtree_add_node(h,h->head,namebuf+1,data,flags,param);
}

//adds a node to the tree
static int dtree_add_node(dtree_dt_index *h,dtree_dt_node *n,char *t,void *data,flag_f flags,void *param)
{
    int hash;
    char *p;
    packed_ptr pp;
    dtree_dt_node *next;
    
    if(!n)
        return 0;
    
    dtree_printd(DTREE_PRINT_INITDTREE,"ADD: tree: '%c' level: %d *t: '%c' p(%p) pp(%p)\n",
             n->data?n->data:'^',dtree_node_depth(h,n),*t?*t:'#',n,n->curr);
    
    //escape
    if(h->sflags & DTREE_S_FLAG_REGEX && (*t==DTREE_PATTERN_ESCAPE) && *(t-1)!=DTREE_PATTERN_ESCAPE)
        t++;

    //(abc)
    if(h->sflags & DTREE_S_FLAG_REGEX && (*t==DTREE_PATTERN_GROUP_S || *t==DTREE_PATTERN_GROUP_E) &&
            *(t-1)!=DTREE_PATTERN_ESCAPE)
        t++;

    //?
    if(h->sflags & DTREE_S_FLAG_REGEX && *t==DTREE_PATTERN_OPTIONAL && *(t-1)!=DTREE_PATTERN_ESCAPE)
    {
        pp=n->prev;
        next=DTREE_DT_GETPP(h,pp);
        
        if(!pp)
            return 0;
        
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: optional: '%c' *t: '%c'\n",next->data?next->data:'^',*(t+1));
        
        if(dtree_add_node(h,next,t+1,data,flags,param)<0)
            return -1;
        
        t++;
    }

    //EOT trailing wildcard
    if(!*t && n->prev)
        return dtree_set_payload(h,n,data,flags,param);
    
    //[abc]
    if(h->sflags & DTREE_S_FLAG_REGEX && *t==DTREE_PATTERN_SET_S && *(t-1)!=DTREE_PATTERN_ESCAPE)
    {
        for(p=t;*p;p++)
        {
            if(*p==DTREE_PATTERN_SET_E && *(p-1)!=DTREE_PATTERN_ESCAPE)
                break;
        }
        
        if(!*p)
            return 0;
        
        t++;
        
        while(t<p)
        {
            *p=*t;
            
            dtree_printd(DTREE_PRINT_INITDTREE,"ADD: set: '%c' *t: '%c' t: '%s'\n",n->data?n->data:'^',*t,p);
            
            if(dtree_add_node(h,n,p,data,flags,param)<0)
                return -1;
            
            t++;
        }
        
        *p=DTREE_PATTERN_SET_E;
        
        return 1;
    }
    
    hash=dtree_hash_char(*t);
        
    if(!hash && *t!='0')
        return 0;
        
    if(*t>='A' && *t<='Z')
        *t|=0x20;
        
    //.
    if(hash==DTREE_HASH_ANY || (h->sflags & DTREE_S_FLAG_REGEX && *t==DTREE_PATTERN_ANY &&
            *(t-1)!=DTREE_PATTERN_ESCAPE))
    {
        *t=DTREE_PATTERN_ANY;
        hash=DTREE_HASH_ANY;
    }
        
    pp=(packed_ptr)n->nodes[hash];
    next=DTREE_DT_GETPP(h,pp);
        
    if(!pp)
    {
        pp=dtree_alloc_node(h);
        next=DTREE_DT_GETPP(h,pp);
        
        if(!pp)
        {
            fprintf(stderr,"DTREE: allocation error detected, aborting: %zu\n",h->node_count);
            return -1;
        }
        
        next->data=*t;
        next->curr=pp;
        next->prev=n->curr;
        
        n->nodes[hash]=pp;
        
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: new node '%c' created level: %d hash: %d p(%p) pp(%p)\n",
                 *t,dtree_node_depth(h,n),hash,next,pp);
    }
    
    //EOT
    if(!*(t+1))
        return dtree_set_payload(h,next,data,flags,param);

    return dtree_add_node(h,next,t+1,data,flags,param);
}

//sets the data, param, and type for a node
static int dtree_set_payload(dtree_dt_index *h,dtree_dt_node *n,void *data,flag_f type,void *param)
{
    packed_ptr pp;
    dtree_dt_node *base=n;
    
    //dup
    if((type & n->flags)==type && data==n->payload && param==n->cparam)
        return 0;
    
    dtree_printd(DTREE_PRINT_INITDTREE,"ADD: EOT: '%c' level: %d p(%p) pp(%p) param(%p)\n",
             n->data?n->data:'^',dtree_node_depth(h,n),n,n->curr,param);
    
    if(n->flags && (h->sflags & DTREE_S_FLAG_DUPS))
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: DUPLICATE detected, adding\n");
        
        pp=n->dup;
        
        //iterate thru the dups
        while(pp)
        {
            n=DTREE_DT_GETPP(h,pp);
            
            //dup
            if((type & n->flags)==type && data==n->payload && param==n->cparam)
                return 0;
            
            pp=n->dup;
        }
        
        pp=dtree_alloc_node(h);
        
        if(!pp)
        {
            fprintf(stderr,"DTREE: allocation error detected, aborting: %zu\n",h->node_count);
            return -1;
        }
        
        n->dup=pp;
        
        n=DTREE_DT_GETPP(h,pp);
        
        n->curr=base->curr;
        n->prev=base->prev;
        n->data=base->data;
        
        return dtree_set_payload(h,n,data,type,param);
    }
    else if(n->flags && !(h->sflags & DTREE_S_FLAG_DUPS))
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: DUPLICATE detected, overwriting original\n");
    
    n->flags=DTREE_DT_FLAG_TOKEN;
    n->flags|=type;
    n->payload=data;
    n->cparam=param;
    
    return 1;
}

//hashes a character, on error 0 is returned
unsigned int dtree_hash_char(char c)
{
    unsigned int i,m;
    
    //alphanumeric
    if(c<='z' && c>='a')
        return c-'a'+10;
    else if(c>='A' && c<='Z')
        return c-'A'+10;
    else if(c<='9' && c>='0')
        return c-'0';
    
    //configurable
    m=sizeof(DTREE_HASH_TCHARS)-1;
    for(i=0;i<m;i++)
    {
        if(c==DTREE_HASH_TCHARS[i])
            return 36+i;
    }
    
    //configurable
    m=sizeof(DTREE_HASH_SCHARS)-1;
    for(i=0;i<m;i++)
    {
        if(c==DTREE_HASH_SCHARS[i])
            return DTREE_HASH_SEP+i;
    }
    
    //wildcard
    if(c>' ' && c<'~')
        return DTREE_HASH_ANY;
    
    //unsupported
    return 0;
}

//dtree lookup
void *dtree_get(const dtree_dt_index *h,const char *str,flag_f filter)
{
    const dtree_dt_node *ret;
    const dtree_dt_node *fnode;
    
    if(!str || !h->head)
        return NULL;
    
    ret=dtree_get_node(h,str,DTREE_S_FLAG_NONE);
    
    if(ret)
    {
        if(filter)
        {
            fnode=dtree_get_flag(h,ret,filter);
            
            if(fnode)
                return fnode->payload;
        }
        else
            return ret->payload;
    }
    
    return NULL;
}

//gets the flag for a token
const dtree_dt_node *dtree_get_node(const dtree_dt_index *h,const char *t,flag_f sflags)
{
    unsigned int hash;
    char n;
    const char *p;
    packed_ptr pp;
    const dtree_dt_node *base;
    const dtree_dt_node *lflag=NULL;
    
    if(!sflags)
        sflags=h->sflags;
    
    if(sflags & DTREE_S_FLAG_REGEX)
    {
        pp=h->head->nodes[dtree_hash_char(*t)];
        base=DTREE_DT_GETPP(h,pp);
        
        lflag=dtree_search_node(h,base,t);
        
        //leading wildcard
        if(!lflag)
        {
            pp=h->head->nodes[DTREE_HASH_ANY];
            base=DTREE_DT_GETPP(h,pp);
            
            if(pp)
                lflag=dtree_search_node(h,base,t);
        }
        
        return lflag;
    }
            
    pp=h->head->nodes[dtree_hash_char(*t)];
    base=DTREE_DT_GETPP(h,pp);
    
    //iterative search
    for(p=t;*p && pp;p++)
    {
        if(base->data!=*p && base->data!=(*p|0x20) && base->data!=DTREE_PATTERN_ANY)
            break;
   
        n=*(p+1);
        hash=dtree_hash_char(n);
        
        //token match
        if((!hash && n!='0') || hash>=DTREE_HASH_SEP)
        {
            //not full string
            if(base->flags && (sflags & DTREE_S_FLAG_PARTIAL) && n)
                return base;
            //full token
            else if(base->flags && !n)
                return base;
            else if(!n)
                break;
        }
        
        //partial token match
        if((base->flags & DTREE_DT_FLAG_TOKEN) && (sflags & DTREE_S_FLAG_PARTIAL) && (p+1-t)>=DTREE_S_PART_TLEN)
            lflag=base;
        
        pp=base->nodes[hash];
        base=DTREE_DT_GETPP(h,pp);
    }

    return lflag;
}

//searches for a token with regex
static const dtree_dt_node *dtree_search_node(const dtree_dt_index *h,const dtree_dt_node *n,const char *t)
{
    unsigned int hash;
    flag_f rf,nf;
    packed_ptr pp;
    const dtree_dt_node *rflag;
    
    if(!n || !n->curr)
        return NULL;
    
    //bad match
    if(n->data!=*t && n->data!=(*t|0x20) && n->data!=DTREE_PATTERN_ANY)
        return NULL;
    
    t++;
    hash=dtree_hash_char(*t);
    
    //EOS
    if(!hash && *t!='0')
    {
        //only quit if strong
        if(dtree_get_flag(h,n,DTREE_DT_FLAG_STRONG))
            return n;
        else if(!n->flags)
            return NULL;
    }
    
    pp=n->nodes[hash];
    rflag=DTREE_DT_GETPP(h,pp);
    
    if(pp)
        rflag=dtree_search_node(h,rflag,t);
    else
        rflag=NULL;
    
    //wildcard
    if(!rflag)
    {
        hash=DTREE_HASH_ANY;
        
        pp=n->nodes[hash];
        rflag=DTREE_DT_GETPP(h,pp);
        
        if(pp)
            rflag=dtree_search_node(h,rflag,t);
        else
            rflag=NULL;
        
        hash=dtree_hash_char(*t);
    }
    
    //partial
    if(!rflag && (n->flags & DTREE_DT_FLAG_TOKEN) && (h->sflags & DTREE_S_FLAG_PARTIAL))
    {
        if(dtree_node_depth(h,n)>=DTREE_S_PART_TLEN)
            rflag=n;
    }
    
    //n is lazy EOT or stronger match
    if((n->flags & DTREE_DT_FLAG_TOKEN) && ((!hash && *t!='0') || hash>=DTREE_HASH_SEP))
    {
        nf=dtree_get_flags(h,n);
        rf=dtree_get_flags(h,rflag);
        
        if(!rflag)
            rflag=n;
        else if((nf & DTREE_DT_FLAG_STRONG) && !(rf & DTREE_DT_FLAG_STRONG))
            rflag=n;
        else if(rf & DTREE_DT_FLAG_STRONG);
        else if((nf & DTREE_DT_FLAG_CHAIN) && !(nf & DTREE_DT_FLAG_BCHAIN) &&
                !(rf & DTREE_DT_FLAG_CHAIN) && !(rf & DTREE_DT_FLAG_BCHAIN))
            rflag=n;
        else if(rf & DTREE_DT_FLAG_CHAIN);
        else if((nf & DTREE_DT_FLAG_WEAK) && !(rf & DTREE_DT_FLAG_WEAK))
            rflag=n;
    }
    
    return rflag;
}
