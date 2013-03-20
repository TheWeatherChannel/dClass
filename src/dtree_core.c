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


static int dtree_add_node(dtree_dt_index*,dtree_dt_node*,char*,const dtree_dt_add_entry*);
static int dtree_set_payload(dtree_dt_index*,dtree_dt_node*,const dtree_dt_add_entry*);
static const dtree_dt_node *dtree_search_node(const dtree_dt_index*,const dtree_dt_node*,const char*,unsigned char);
unsigned int dtree_hash_char(char);

extern packed_ptr dtree_alloc_node(dtree_dt_index*);
extern int dtree_node_depth(const dtree_dt_index*,const dtree_dt_node*);


//adds an entry to the tree
int dtree_add_entry(dtree_dt_index *h,const char *key,dtree_dt_add_entry *entry)
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
    
    dtree_printd(DTREE_PRINT_INITDTREE,"ADD ENTRY: name '%s':%d\n",namebuf+1,entry->flags);
    
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
    
    if(!entry->flags)
        entry->flags=DTREE_DT_FLAG_STRONG;
    
    return dtree_add_node(h,h->head,namebuf+1,entry);
}

//adds a node to the tree
static int dtree_add_node(dtree_dt_index *h,dtree_dt_node *n,char *t,const dtree_dt_add_entry *entry)
{
    int hash;
    char *s;
    packed_ptr pp;
    dtree_dt_node *next;
    
    if(!n)
        return 0;
    
    dtree_printd(DTREE_PRINT_INITDTREE,"ADD: tree: '%c' level: %d t: '%s' p(%p) pp(%p)\n",
             n->data?n->data:'^',dtree_node_depth(h,n),t,n,n->curr);
    
    //escape
    if(h->sflags & DTREE_S_FLAG_REGEX && (*t==DTREE_PATTERN_ESCAPE) && *(t-1)!=DTREE_PATTERN_ESCAPE)
        t++;

    //(abc)
    while(h->sflags & DTREE_S_FLAG_REGEX && (*t==DTREE_PATTERN_GROUP_S || *t==DTREE_PATTERN_GROUP_E) &&
            *(t-1)!=DTREE_PATTERN_ESCAPE)
    {
        hash=(*t==DTREE_PATTERN_GROUP_S);
        s=t+1;

        while(*s && hash)
        {
            if(*s==DTREE_PATTERN_GROUP_S && *(s-1)!=DTREE_PATTERN_ESCAPE)
                hash++;
            else if(*s==DTREE_PATTERN_GROUP_E && *(s-1)!=DTREE_PATTERN_ESCAPE)
                hash--;

            s++;
        }

        if(*t==DTREE_PATTERN_GROUP_S)
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"ADD: group detected: '%c' - '%c'\n",*(t+1)?*(t+1):'#',*s?*s:'#');

            //optional group
            if(*s==DTREE_PATTERN_OPTIONAL && *(s-1)!=DTREE_PATTERN_ESCAPE)
            {
                if(dtree_add_node(h,n,s+1,entry)<0)
                    return -1;
            }
        }

        t++;
    }

    //?
    if(h->sflags & DTREE_S_FLAG_REGEX && *t==DTREE_PATTERN_OPTIONAL && *(t-1)!=DTREE_PATTERN_ESCAPE)
    {
        //no group
        if(*(t-1)!=DTREE_PATTERN_GROUP_E || *(t-2)==DTREE_PATTERN_ESCAPE)
        {
            pp=n->prev;
            next=DTREE_DT_GETPP(h,pp);
        
            if(!pp)
                return 0;
        
            dtree_printd(DTREE_PRINT_INITDTREE,"ADD: optional: '%c' t: '%s'\n",next->data?next->data:'^',(t+1));
        
            if(dtree_add_node(h,next,t+1,entry)<0)
                return -1;
        }

        t++;

        return dtree_add_node(h,n,t,entry);
    }

    //EOT
    if(!*t && n->prev)
        return dtree_set_payload(h,n,entry);
    
    //[abc]
    if(h->sflags & DTREE_S_FLAG_REGEX && *t==DTREE_PATTERN_SET_S && *(t-1)!=DTREE_PATTERN_ESCAPE)
    {
        for(s=t;*s;s++)
        {
            if(*s==DTREE_PATTERN_SET_E && *(s-1)!=DTREE_PATTERN_ESCAPE)
                break;
        }
        
        if(!*s)
            return 0;
        
        t++;
        
        while(t<s)
        {
            *s=*t;
            
            dtree_printd(DTREE_PRINT_INITDTREE,"ADD: set: '%c' *t: '%c' t: '%s'\n",n->data?n->data:'^',*t,s);
            
            if(dtree_add_node(h,n,s,entry)<0)
                return -1;
            
            t++;
        }
        
        *s=DTREE_PATTERN_SET_E;
        
        return 1;
    }
    
    hash=dtree_hash_char(*t);
        
    if(!hash && *t!='0')
        return 0;
        
    if(*t>='A' && *t<='Z')
        *t|=0x20;
        
    //.
    if(h->sflags & DTREE_S_FLAG_REGEX && *t==DTREE_PATTERN_ANY && *(t-1)!=DTREE_PATTERN_ESCAPE)
        hash=DTREE_HASH_ANY;
        
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
        
        if(hash==DTREE_HASH_ANY)
            next->data=DTREE_PATTERN_ANY;
        else
            next->data=*t;

        next->curr=pp;
        next->prev=n->curr;
        
        n->nodes[hash]=pp;
        
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: new node '%c' created level: %d hash: %d p(%p) pp(%p)\n",
                 *t,dtree_node_depth(h,n),hash,next,pp);
    }
    
    //EOT
    if(!*(t+1))
        return dtree_set_payload(h,next,entry);

    return dtree_add_node(h,next,t+1,entry);
}

//sets the data, param, and type for a node
static int dtree_set_payload(dtree_dt_index *h,dtree_dt_node *n,const dtree_dt_add_entry *entry)
{
    packed_ptr pp;
    dtree_dt_node *base=n;
    
    if((entry->flags & n->flags)==entry->flags && entry->data==n->payload && entry->param==n->cparam &&
            entry->pos==n->pos && entry->dir==n->dir)
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: PURE DUPLICATE detected, skipping\n");
    
        return 0;
    }
    
    dtree_printd(DTREE_PRINT_INITDTREE,"ADD: EOT: '%c' level: %d p(%p) pp(%p) param(%p) flags: %d\n",
             n->data?n->data:'^',dtree_node_depth(h,n),n,n->curr,entry->param,entry->flags);
    
    if(n->flags && (h->sflags & DTREE_S_FLAG_DUPS))
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: DUPLICATE detected, adding\n");
        
        pp=n->dup;
        
        //iterate thru the dups
        while(pp)
        {
            n=DTREE_DT_GETPP(h,pp);
            
            //dup
            if((entry->flags & n->flags)==entry->flags && entry->data==n->payload && entry->param==n->cparam &&
                    entry->pos==n->pos && entry->dir==n->dir)
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
        
        return dtree_set_payload(h,n,entry);
    }
    else if(n->flags && !(h->sflags & DTREE_S_FLAG_DUPS))
        dtree_printd(DTREE_PRINT_INITDTREE,"ADD: DUPLICATE detected, overwriting original\n");
    
    n->flags=DTREE_DT_FLAG_TOKEN;
    n->flags|=entry->flags;

    n->payload=entry->data;
    n->cparam=entry->param;

    n->pos=entry->pos;
    n->rank=entry->rank;
    n->dir=entry->dir;
    
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
    if(c>=' ' && c<='~')
        return DTREE_HASH_ANY;
    
    //unsupported
    return 0;
}

//dtree lookup
void *dtree_get(const dtree_dt_index *h,const char *str,dtree_flag_f filter)
{
    const dtree_dt_node *ret;
    const dtree_dt_node *fnode;
    
    if(!str || !h->head)
        return NULL;
    
    ret=dtree_get_node(h,str,DTREE_S_FLAG_NONE,0);
    
    if(ret)
    {
        if(filter)
        {
            fnode=dtree_get_flag(h,ret,filter,0);
            
            if(fnode)
                return fnode->payload;
        }
        else
            return ret->payload;
    }
    
    return NULL;
}

//gets the flag for a token
const dtree_dt_node *dtree_get_node(const dtree_dt_index *h,const char *t,dtree_flag_f sflags,dtree_pos_f pos)
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
        
        lflag=dtree_search_node(h,base,t,pos);
        
        //leading wildcard
        if(!lflag)
        {
            pp=h->head->nodes[DTREE_HASH_ANY];
            base=DTREE_DT_GETPP(h,pp);
            
            if(pp)
                lflag=dtree_search_node(h,base,t,pos);
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
            //match is valid with position
            if(base->flags && (sflags & DTREE_S_FLAG_PARTIAL || !n) &&
                    (!base->pos || !pos || base->pos==pos))
                return base;
            else if(!n)
                break;
        }
        
        pp=base->nodes[hash];
        base=DTREE_DT_GETPP(h,pp);
    }

    return lflag;
}

//searches for a token with regex
static const dtree_dt_node *dtree_search_node(const dtree_dt_index *h,const dtree_dt_node *n,const char *t,dtree_pos_f pos)
{
    unsigned int hash;
    dtree_flag_f rf,nf;
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
        //only quit if strong with position
        if(dtree_get_flag(h,n,DTREE_DT_FLAG_STRONG,pos))
            return n;
        else if(!n->flags)
            return NULL;
    }
    
    pp=n->nodes[hash];
    rflag=DTREE_DT_GETPP(h,pp);
    
    if(pp)
        rflag=dtree_search_node(h,rflag,t,pos);
    else
        rflag=NULL;
    
    //wildcard
    if(!rflag)
    {
        hash=DTREE_HASH_ANY;
        
        pp=n->nodes[hash];
        rflag=DTREE_DT_GETPP(h,pp);
        
        if(pp)
            rflag=dtree_search_node(h,rflag,t,pos);
        else
            rflag=NULL;
        
        hash=dtree_hash_char(*t);
    }
    
    //n is lazy EOT or stronger match
    if((n->flags & DTREE_DT_FLAG_TOKEN) && ((!hash && *t!='0') || hash>=DTREE_HASH_SEP))
    {
        nf=dtree_get_flags(h,n,pos);
        rf=dtree_get_flags(h,rflag,pos);
        
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
