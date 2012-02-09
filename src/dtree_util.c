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


static long dtree_print_node(const dtree_dt_index*,const char*(*f)(void*),const dtree_dt_node*,char*,int);
inline int dtree_node_depth(const dtree_dt_index*,const dtree_dt_node*);
char *dtree_node_path(const dtree_dt_index*,const dtree_dt_node*,char*);
void dtree_timersubn(struct timespec*,struct timespec*,struct timespec*);


//finds a certain flag among dup nodes
inline const dtree_dt_node *dtree_get_flag(const dtree_dt_index *h,const dtree_dt_node *n,flag_f flag)
{
    packed_ptr pp=n->curr;
    
    while(pp)
    {
        if(n->flags & flag)
            return n;
        
        pp=n->dup;
        n=DTREE_DT_GETPP(h,pp);
    }
    
    return NULL;
}

//finds a certain flag among dup nodes
flag_f dtree_get_flags(const dtree_dt_index *h,const dtree_dt_node *n)
{
    packed_ptr pp;
    
    if(n)
    {
        pp=n->dup;
        
        if(pp)
            return n->flags | dtree_get_flags(h,DTREE_DT_GETPP(h,pp));
        else
            return n->flags;
    }
    else
        return 0;
}

//gets the depth of the current node
inline int dtree_node_depth(const dtree_dt_index *h,const dtree_dt_node *n)
{
    int d=0;
    packed_ptr pp;
    
    pp=n->curr;
    
    while(pp && n)
    {
        d++;
        pp=n->prev;
        n=DTREE_DT_GETPP(h,pp);
    }
    
    return d-1;
}

//gets the pattern path of a node
char *dtree_node_path(const dtree_dt_index *h,const dtree_dt_node *n,char *buf)
{
    int d;
    packed_ptr pp;
    
    d=dtree_node_depth(h,n);
    
    buf[d]='\0';
    pp=n->curr;
    
    while(pp && n && d>0)
    {
        d--;
        buf[d]=n->data;
        pp=n->prev;
        n=DTREE_DT_GETPP(h,pp);
    }
    
    return buf;
}

//walks the dtree
long dtree_print(const dtree_dt_index *h,const char* (*f)(void*))
{
    unsigned int i;
    char buf[DTREE_DATA_BUFLEN];
    long tot=0;
    
    memset(buf,0,sizeof(buf));
    
    for(i=0;h->head && i<DTREE_HASH_NCOUNT;i++)
        tot+=dtree_print_node(h,f,DTREE_DT_GETPP(h,h->head->nodes[i]),buf,0);
    
    dtree_printd(DTREE_PRINT_INITDTREE,"WALK: Walked %ld tokens\n",tot);
    
    return tot;
}

//walks the dtree_node
static long dtree_print_node(const dtree_dt_index *h,const char* (*f)(void*),const dtree_dt_node *n,char *path,int depth)
{
    unsigned int i;
    char buf[DTREE_DATA_BUFLEN];
    const char *s;
    long tot=0;
    packed_ptr pp;
    const dtree_dt_node *dupn;
    
    if(!n || depth>(DTREE_DATA_BUFLEN-1))
        return 0;
    
    path[depth]=n->data;
    path[depth+1]='\0';
    
    if(n->flags & DTREE_DT_FLAG_TOKEN)
    {
        s="";
        
        if(f)
            s=f(n->payload);
        
        dtree_printd(DTREE_PRINT_INITDTREE,"PRINT: found token: '%s' flags: %d pdata: '%s' p(%p) cparam(%p)\n",
                 path,n->flags,s,n->payload,n->cparam);
        
        tot++;
        
        dtree_node_path(h,n,buf);
        
        if(strcmp(path,buf))
            dtree_printd(DTREE_PRINT_INITDTREE,"PRINT: ERROR path mismatch: '%s' != '%s'\n",path,buf);
    }
    
    pp=n->dup;
    dupn=DTREE_DT_GETPP(h,pp);
    
    while(pp)
    {
        s="";
        
        if(f)
            s=f(dupn->payload);
        
        dtree_printd(DTREE_PRINT_INITDTREE,"PRINT: found token: '%s' flags: %d pdata: '%s' p(%p) cparam(%p)\n",
                 path,dupn->flags,s,dupn->payload,dupn->cparam);

        tot++;
        
        pp=dupn->dup;
        dupn=DTREE_DT_GETPP(h,pp);
    }
    
    for(i=0;i<DTREE_HASH_NCOUNT;i++)
    {
        if(n->nodes[i])
            tot+=dtree_print_node(h,f,DTREE_DT_GETPP(h,n->nodes[i]),path,depth+1);
    }
    
    path[depth]='\0';
    
    return tot;
}

//debug printf
void dtree_printd(int level,const char* fmt,...)
{
#if DTREE_DEBUG_LOGGING
    va_list ap;
    
    if(level & DTREE_PRINT_ENABLED)
    {
        va_start(ap,fmt);
    
        vprintf(fmt,ap);
    
        va_end(ap);
    }
#endif
}

//subtracts timers
void dtree_timersubn(struct timespec *end,struct timespec *start,struct timespec *r)
{
    r->tv_sec=end->tv_sec-start->tv_sec;
    r->tv_nsec=end->tv_nsec-start->tv_nsec;
    
    if(r->tv_nsec<0)
    {
      r->tv_sec--;
      r->tv_nsec+=(1000*1000*1000);
    }
}
