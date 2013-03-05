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


#include "dclass_client.h"


char **dclass_get_value_pos(dclass_keyvalue*,char*);
static const dclass_keyvalue *dclass_get_kverror(const dclass_index*);
static char *dclass_error_string(const dtree_dt_index*);

extern unsigned int dtree_hash_char(char);
extern char *dtree_node_path(const dtree_dt_index*,const dtree_dt_node*,char*);


//classifies a string
const dclass_keyvalue *dclass_classify(const dclass_index *di,const char *str)
{
    int on=0;
    int valid;
    int bcvalid;
    int i;
    unsigned int pos=0;
    unsigned int hash;
    char buf[DTREE_DATA_BUFLEN];
    const char *p;
    const char *token="";
    packed_ptr pp;
    const dtree_dt_node *wnode=NULL;
    const dtree_dt_node *nnode=NULL;
    const dtree_dt_node *fbnode;
    const dtree_dt_node *fnode;
    const dtree_dt_index *h=&di->dti;
    dclass_cnode cnodes[DTREE_S_MAX_CHAIN];
    
    if(!str || !h->head)
        return dclass_get_kverror(di);
    
    memset(cnodes,0,sizeof(cnodes));
    
    dtree_printd(DTREE_PRINT_CLASSIFY,"dclass_classify() UA: '%s'\n",str);
    
    for(p=str;*p;p++)
    {
        valid=0;

        hash=dtree_hash_char(*p);
        
        if(hash<DTREE_HASH_SEP && (hash || *p=='0'))
        {
            //new token found
            if(!on)
            {
                token=p;
                on=1;

                if(pos<DTREE_S_MAX_POS)
                    pos++;
            }
            
            valid=1;
        }
        
        if((!valid || (!*(p+1))) && on)
        {
            //EOT found
            fbnode=dtree_get_node(h,token,0,0);
            
            dtree_printd(DTREE_PRINT_CLASSIFY,"dtree_classify() token %d: '%s' = '%s':%d\n",
                    pos,token,fbnode?dtree_node_path(h,fbnode,buf):"",fbnode?(int)fbnode->flags:0);
            
            if(fbnode && dtree_get_flag(h,fbnode,DTREE_DT_FLAG_TOKEN,pos))
            {
                if((fnode=dtree_get_flag(h,fbnode,DTREE_DT_FLAG_STRONG,pos)))
                    return fnode->payload;
                else if((fnode=dtree_get_flag(h,fbnode,DTREE_DT_FLAG_WEAK,pos)))
                {
                    i=DTREE_DC_DISTANCE(h,(char*)fnode->payload);
                    if(!wnode || fnode->rank>wnode->rank || (fnode->rank==wnode->rank &&
                            i>=0 && i<DTREE_DC_DISTANCE(h,(char*)wnode->payload)))
                        wnode=fnode;
                }
                else if((fnode=dtree_get_flag(h,fbnode,DTREE_DT_FLAG_NONE,pos)))
                {
                    i=DTREE_DC_DISTANCE(h,(char*)fnode->payload);
                    if(!nnode || (i>=0 && i<DTREE_DC_DISTANCE(h,(char*)nnode->payload)))
                        nnode=fnode;
                }
                
                if((fnode=dtree_get_flag(h,fbnode,DTREE_DT_FLAG_CHAIN,pos)))
                {
                    dtree_printd(DTREE_PRINT_CLASSIFY,"dtree_classify() pchain detected\n");
                    
                    pp=fnode->curr;

                    while(pp)
                    {
                        bcvalid=0;
                        
                        if(fnode->flags & DTREE_DT_FLAG_CHAIN && fnode->cparam)
                        {   
                            dtree_printd(DTREE_PRINT_CLASSIFY,"dtree_classify() looking for pchain %p\n",fnode->cparam);
                            for(i=0;i<DTREE_S_MAX_CHAIN && cnodes[i].cn;i++)
                            {
                                //chain hit
                                if(cnodes[i].cn==fnode->cparam)
                                {
                                    if(fnode->flags & DTREE_DT_FLAG_BCHAIN)
                                        bcvalid=1;
                                    else
                                        return fnode->payload;
                                }
                            }
                        }
                        
                        if(fnode->flags & DTREE_DT_FLAG_BCHAIN && (bcvalid || !fnode->cparam))
                        {
                            for(i=0;i<DTREE_S_MAX_CHAIN;i++)
                            {
                                if(!cnodes[i].cn)
                                {
                                    cnodes[i].cn=fnode->payload;
                                    cnodes[i].pos=pos;
                                    dtree_printd(DTREE_PRINT_CLASSIFY,"dtree_classify() pchain added: %p('%d'):%d\n",fnode->payload,i,pos);
                                    break;
                                }
                            }
                        }
                        
                        pp=fnode->dup;
                        fnode=DTREE_DT_GETPP(h,pp);
                    }
                }
            }

            if(!*(p+1) && pos<DTREE_S_MAX_POS)
            {
                p--;

                if(token==str)
                    pos=DTREE_S_BE_POS;
                else
                    pos=DTREE_S_MAX_POS;
    
                continue;
            }

            on=0;
        }
    }
    
    if(wnode)
        return wnode->payload;
    else if(nnode)
        return nnode->payload;
    
    return dclass_get_kverror(di);
}


//gets a string
const dclass_keyvalue *dclass_get(const dclass_index *di,const char *str)
{
    const dtree_dt_node *node;
    const dtree_dt_index *h=&di->dti;

    if(!str || !h->head)
        return dclass_get_kverror(di);

    node=dtree_get_node(h,str,DTREE_DT_FLAG_TOKEN,0);
    
    if(node && node->payload)
        return node->payload;

    return dclass_get_kverror(di);
}


//gets the value for a key
const char *dclass_get_kvalue(const dclass_keyvalue *kvd,const char *key)
{
    int i;
    
    for(i=0;i<kvd->size;i++)
    {
        if(!strcasecmp(kvd->keys[i],key))
            return kvd->values[i];
    }
    
    return NULL;
}

//gets the value pointer for a key
char **dclass_get_value_pos(dclass_keyvalue *kvd,char *key)
{
    int i;
    
    for(i=0;i<kvd->size;i++)
    {
        if(!strcasecmp(kvd->keys[i],key))
            return (char**)&kvd->values[i];
    }
    
    return NULL;
}

//returns an error kvdata
static const dclass_keyvalue *dclass_get_kverror(const dclass_index *di)
{
    if(!di->error.id)
        ((dclass_index*)di)->error.id=dclass_error_string(&di->dti);
    
    return &di->error;
}

//show first string on error
static char *dclass_error_string(const dtree_dt_index *h)
{
    if(DTREE_M_LOOKUP_CACHE && h->dc_cache[0])
        return h->dc_cache[0];
    
    return DTREE_M_SERROR;
}

//gets the id for a generic payload
const char *dclass_get_id(void *payload)
{
    dclass_keyvalue *kvd;
    
    kvd=(dclass_keyvalue*)payload;
    
    return kvd->id;
}

//version
const char *dclass_get_version()
{
    return DCLASS_VERSION;
}

//init
void dclass_init_index(dclass_index *di)
{
    memset(&di->error,0,sizeof(dclass_keyvalue));
    
    dtree_init_index(&di->dti);
}

//free
void dclass_free(dclass_index *di)
{
    dtree_free(&di->dti);
    
    memset(&di->error,0,sizeof(dclass_keyvalue));
}

