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

#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "vcc_if.h"


#define VMOD_DTREE_SIZE    4


typedef struct
{
    unsigned xid;
    dclass_keyvalue *kvd[VMOD_DTREE_SIZE];
}
vmod_dclass_container;

typedef struct
{
    dclass_index heads[VMOD_DTREE_SIZE];
    
    vmod_dclass_container *vmod_dc_list;
    int vmod_dc_list_size;
    pthread_mutex_t vmod_dc_list_mutex;
}
vmod_dtree_container;


void vmod_free_dtc(void*);
static vmod_dclass_container *dcc_get(struct sess*,vmod_dtree_container*);


//inits a dtc, stores it in PRIV_VCL, inits dc_list
int init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
    int i,j;
    vmod_dtree_container *dtc;
    
    dtc=malloc(sizeof(vmod_dtree_container));
    
    AN(dtc);
    
    pthread_mutex_init(&dtc->vmod_dc_list_mutex,NULL);
    
    for(i=0;i<VMOD_DTREE_SIZE;i++)
        dclass_init_index(&dtc->heads[i]);

    dtc->vmod_dc_list_size=256;
    
    dtc->vmod_dc_list=malloc(dtc->vmod_dc_list_size*sizeof(vmod_dclass_container));
    
    AN(dtc->vmod_dc_list);
    
    for (i=0;i<dtc->vmod_dc_list_size;i++)
    {
        for(j=0;j<VMOD_DTREE_SIZE;j++)
            dtc->vmod_dc_list[i].kvd[j]=NULL;
        
        dtc->vmod_dc_list[i].xid=0;
    }
    
    priv->priv=dtc;
    priv->free=vmod_free_dtc;
    
    return 0;
}

void vmod_init_dclass(struct sess *sp,struct vmod_priv *priv,const char *dtree_loc)
{
    vmod_init_dclass_p(sp,priv,dtree_loc,0);
}

//inits a dclass dtree
void vmod_init_dclass_p(struct sess *sp,struct vmod_priv *priv,const char *dtree_loc,int p)
{
    int ret;
    vmod_dtree_container *dtc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return;
    
    dtc=(vmod_dtree_container*)priv->priv;
    
    printf("vmod_init_dtree: loading new dtree: %d:'%s'\n",p,dtree_loc);
    
    ret=dclass_load_file(&dtc->heads[p],(char*)dtree_loc);
    
    if(ret<=0)
        openddr_load_resources(&dtc->heads[p],(char*)dtree_loc);
}

const char *vmod_classify(struct sess *sp, struct vmod_priv *priv, const char *str)
{
    return vmod_classify_p(sp,priv,str,0);
}

//classifies a string
const char *vmod_classify_p(struct sess *sp,struct vmod_priv *priv,const char *str,int p)
{
    dclass_keyvalue *kvd;
    vmod_dtree_container *dtc;
    vmod_dclass_container *dcc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return NULL;
    
    dtc=(vmod_dtree_container*)priv->priv;
    
    dcc=dcc_get(sp,dtc);
    
    kvd=(dclass_keyvalue*)dclass_classify(&dtc->heads[p],str);
    
    dcc->kvd[p]=kvd;
    
    if(!kvd)
        return NULL;
    else
        return kvd->id;
}

const char *vmod_get_field(struct sess *sp, struct vmod_priv *priv, const char *key)
{
    return vmod_get_field_p(sp,priv,key,0);
}

const char *vmod_get_field_p(struct sess *sp,struct vmod_priv *priv,const char *key,int p)
{
    const char *ret=NULL;
    vmod_dtree_container *dtc;
    vmod_dclass_container *dcc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return "";
    
    dtc=(vmod_dtree_container*)priv->priv;
    
    dcc=dcc_get(sp,dtc);
    
    if(dcc->kvd[p])
        ret=dclass_get_kvalue(dcc->kvd[p],key);
    
    if(ret)
        return ret;
    else
        return "";
}

int vmod_get_ifield(struct sess *sp, struct vmod_priv *priv, const char *key)
{
    return vmod_get_ifield_p(sp,priv,key,0);
}

int vmod_get_ifield_p(struct sess *sp,struct vmod_priv *priv,const char *key,int p)
{
    const char *s;
    vmod_dtree_container *dtc;
    vmod_dclass_container *dcc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return 0;
    
    dtc=(vmod_dtree_container*)priv->priv;
    
    dcc=dcc_get(sp,dtc);
    
    if(dcc->kvd[p])
    {
        s=dclass_get_kvalue(dcc->kvd[p],key);

        if(s)
            return atoi(s);
        else
            return 0;
    }
    
    return 0;
}

const char *vmod_get_comment(struct sess *sp, struct vmod_priv *priv)
{
    return vmod_get_comment_p(sp,priv,0);
}

const char *vmod_get_comment_p(struct sess *sp,struct vmod_priv *priv,int p)
{
    const char *ret=NULL;
    vmod_dtree_container *dtc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return "";
    
    dtc=(vmod_dtree_container*)priv->priv;
    
    ret=dtc->heads[p].dti.comment;
    
    if(ret)
        return ret;
    else
        return "";
}

//gets a dc from the dc_list
static vmod_dclass_container *dcc_get(struct sess *sp,vmod_dtree_container *dtc)
{
    int ns,j;
    vmod_dclass_container *dcc;
    
    AZ(pthread_mutex_lock(&dtc->vmod_dc_list_mutex));

    while (dtc->vmod_dc_list_size<=sp->id)
    {
            ns=dtc->vmod_dc_list_size*2;

            dtc->vmod_dc_list=realloc(dtc->vmod_dc_list,ns*sizeof(vmod_dclass_container));
            
            AN(dtc->vmod_dc_list);
            
            for (;dtc->vmod_dc_list_size<ns;dtc->vmod_dc_list_size++)
            {
                for(j=0;j<VMOD_DTREE_SIZE;j++)
                    dtc->vmod_dc_list[dtc->vmod_dc_list_size].kvd[j]=NULL;
                
                dtc->vmod_dc_list[dtc->vmod_dc_list_size].xid=0;
            }
    }
    
    dcc=&dtc->vmod_dc_list[sp->id];
    
    if (dcc->xid!=sp->xid)
    {
        dcc->kvd[0]=NULL;
        dcc->xid=sp->xid;
    }
    
    AZ(pthread_mutex_unlock(&dtc->vmod_dc_list_mutex));
    
    return dcc;
}

void vmod_free_dtc(void *data)
{
    int i;
    vmod_dtree_container *dtc;
    
    dtc=(vmod_dtree_container*)data;
    
    for(i=0;i<VMOD_DTREE_SIZE;i++)
        dclass_free(&dtc->heads[i]);
    
    free(dtc->vmod_dc_list);
    
    free(data);
}

const char *vmod_get_version(struct sess *sp)
{
    return dclass_get_version();
}
