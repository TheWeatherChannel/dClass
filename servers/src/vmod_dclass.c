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

#if _DCLASS_VARNISH4
#include "cache/cache.h"
#else
#include "bin/varnishd/cache.h"
#endif

#include "vcc_if.h"


#define VMOD_DTREE_SIZE    4


#if _DCLASS_VARNISH4
typedef const struct vrt_ctx vctx;
typedef int vid_t;
typedef uint32_t vxid_t;
#else
typedef struct sess vctx;
typedef const char* VCL_STRING;
typedef int VCL_INT;
typedef int vid_t;
typedef unsigned vxid_t;
#endif


typedef struct
{
    vxid_t xid;
    dclass_keyvalue *kvd[VMOD_DTREE_SIZE];
}
vmod_user_container;

typedef struct
{
    dclass_index heads[VMOD_DTREE_SIZE];
    vmod_user_container *vmod_u_list;
    vid_t vmod_u_list_size;
    pthread_mutex_t vmod_u_list_mutex;
}
vmod_dclass_container;


void vmod_free_dtc(void*);

static vmod_user_container *uc_get(vctx*,vmod_dclass_container*);


//inits a dtc, stores it in PRIV_VCL, inits dc_list
int init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
    int i;
    vmod_dclass_container *dtc;
    
    dtc=malloc(sizeof(vmod_dclass_container));
    
    AN(dtc);
    
    for(i=0;i<VMOD_DTREE_SIZE;i++)
        dclass_init_index(&dtc->heads[i]);

    pthread_mutex_init(&dtc->vmod_u_list_mutex,NULL);
    
    dtc->vmod_u_list_size=256;
    
    dtc->vmod_u_list=calloc(sizeof(vmod_user_container),dtc->vmod_u_list_size);
    
    AN(dtc->vmod_u_list);
    
    priv->priv=dtc;
    priv->free=vmod_free_dtc;
    
    return 0;
}

void vmod_init_dclass(vctx *sp,struct vmod_priv *priv,VCL_STRING dtree_loc)
{
    vmod_init_dclass_p(sp,priv,dtree_loc,0);
}

//inits a dclass dtree
void vmod_init_dclass_p(vctx *sp,struct vmod_priv *priv,VCL_STRING dtree_loc,VCL_INT p)
{
    int ret;
    vmod_dclass_container *dtc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return;
    
    dtc=(vmod_dclass_container*)priv->priv;
    
    printf("vmod_init_dtree: loading new dtree: %d:'%s'\n",(int)p,dtree_loc);
    
    ret=dclass_load_file(&dtc->heads[p],(char*)dtree_loc);
    
    if(ret<=0)
        devicemap_load_resources(&dtc->heads[p],(char*)dtree_loc);
}

VCL_STRING vmod_classify(vctx *sp, struct vmod_priv *priv, VCL_STRING str)
{
    return vmod_classify_p(sp,priv,str,0);
}

//classifies a string
VCL_STRING vmod_classify_p(vctx *sp,struct vmod_priv *priv,VCL_STRING str,VCL_INT p)
{
    dclass_keyvalue *kvd;
    vmod_dclass_container *dtc;
    vmod_user_container *uc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return NULL;
    
    dtc=(vmod_dclass_container*)priv->priv;
    
    uc=uc_get(sp,dtc);
    
    kvd=(dclass_keyvalue*)dclass_classify(&dtc->heads[p],str);
    
    uc->kvd[p]=kvd;
    
    if(!kvd)
        return NULL;
    else
        return kvd->id;
}

VCL_STRING vmod_get_field(vctx *sp, struct vmod_priv *priv, VCL_STRING key)
{
    return vmod_get_field_p(sp,priv,key,0);
}

VCL_STRING vmod_get_field_p(vctx *sp,struct vmod_priv *priv,VCL_STRING key,VCL_INT p)
{
    VCL_STRING ret=NULL;
    vmod_dclass_container *dtc;
    vmod_user_container *uc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return "";
    
    dtc=(vmod_dclass_container*)priv->priv;
    uc=uc_get(sp,dtc);
    
    if(uc->kvd[p])
        ret=dclass_get_kvalue(uc->kvd[p],key);
    
    if(ret)
        return ret;
    else
        return "";
}

VCL_INT vmod_get_ifield(vctx *sp, struct vmod_priv *priv, VCL_STRING key)
{
    return vmod_get_ifield_p(sp,priv,key,0);
}

VCL_INT vmod_get_ifield_p(vctx *sp,struct vmod_priv *priv,VCL_STRING key,VCL_INT p)
{
    VCL_STRING s;
    vmod_dclass_container *dtc;
    vmod_user_container *uc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return 0;
    
    dtc=(vmod_dclass_container*)priv->priv;
    uc=uc_get(sp,dtc);
    
    if(uc->kvd[p])
    {
        s=dclass_get_kvalue(uc->kvd[p],key);

        if(s)
            return atoi(s);
        else
            return 0;
    }
    
    return 0;
}

VCL_STRING vmod_get_comment(vctx *sp, struct vmod_priv *priv)
{
    return vmod_get_comment_p(sp,priv,0);
}

VCL_STRING vmod_get_comment_p(vctx *sp,struct vmod_priv *priv,VCL_INT p)
{
    VCL_STRING ret=NULL;
    vmod_dclass_container *dtc;
    
    if(p<0 || p>=VMOD_DTREE_SIZE)
        return "";
    
    dtc=(vmod_dclass_container*)priv->priv;
    
    ret=dtc->heads[p].dti.comment;
    
    if(ret)
        return ret;
    else
        return "";
}

//gets a dc from the dc_list
static vmod_user_container *uc_get(vctx *sp,vmod_dclass_container *dtc)
{
    int ns,j;
    vmod_user_container *uc;
    vid_t id;
    vxid_t xid;
    
    AZ(pthread_mutex_lock(&dtc->vmod_u_list_mutex));

#if _DCLASS_VARNISH4
    id=sp->req->sp->fd;
    xid=sp->req->sp->vxid;
#else
    id=sp->id;
    xid=sp->xid;
#endif

    while (dtc->vmod_u_list_size<=id)
    {
            ns=dtc->vmod_u_list_size*2;

            dtc->vmod_u_list=realloc(dtc->vmod_u_list,ns*sizeof(vmod_user_container));
            
            AN(dtc->vmod_u_list);
            
            for (;dtc->vmod_u_list_size<ns;dtc->vmod_u_list_size++)
            {
                for(j=0;j<VMOD_DTREE_SIZE;j++)
                    dtc->vmod_u_list[dtc->vmod_u_list_size].kvd[j]=NULL;
                
                dtc->vmod_u_list[dtc->vmod_u_list_size].xid=0;
            }
    }
    
    uc=&dtc->vmod_u_list[id];
    
    if (uc->xid!=xid)
    {
        for(j=0;j<VMOD_DTREE_SIZE;j++)
            uc->kvd[j]=NULL;

        uc->xid=xid;
    }
    
    AZ(pthread_mutex_unlock(&dtc->vmod_u_list_mutex));
    
    return uc;
}

void vmod_free_dtc(void *data)
{
    int i;
    vmod_dclass_container *dtc;

    dtc=(vmod_dclass_container*)data;

    free(dtc->vmod_u_list);
    
    for(i=0;i<VMOD_DTREE_SIZE;i++)
        dclass_free(&dtc->heads[i]);
    
    free(data);
}

VCL_STRING vmod_get_version(vctx *sp)
{
    return dclass_get_version();
}
