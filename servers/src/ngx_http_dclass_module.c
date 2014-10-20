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


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "dclass_client.h"


#define NGX_HTTP_DCLASS_INDEXES  4
#define NGX_HTTP_DCLASS_LOGLEVEL NGX_LOG_INFO

//settings struct
typedef struct
{
    ngx_flag_t        enable;
    ngx_str_t         dtree_loc[NGX_HTTP_DCLASS_INDEXES];
    ngx_str_t         hfield;
    dclass_index      *head[NGX_HTTP_DCLASS_INDEXES];
}
ngx_http_dclass_conf_t;


//pointer wrapper
typedef struct
{
    u_char                  data[4];
    const dclass_keyvalue   *kvd[NGX_HTTP_DCLASS_INDEXES];
}
ngx_http_dclass_kvdata_str;


static const char *ngx_dclass_key_array[] =                \
                              {                            \
                                "vendor",                  \
                                "model",                   \
                                "parentId",                \
                                "inputDevices",            \
                                "displayHeight",           \
                                "displayWidth",            \
                                "device_os",               \
                                "ajax_support_javascript", \
                                "is_tablet",               \
                                "is_wireless_device",      \
                                "is_desktop",              \
                                "is_crawler",              \
                                "browser",                 \
                                "version",                 \
                                "os"                       \
                              };


static void *ngx_http_dclass_create_conf(ngx_conf_t*);
static char *ngx_http_dclass_merge_conf(ngx_conf_t*, void*, void*);
static ngx_int_t ngx_http_dclass_add_variables(ngx_conf_t*);
static ngx_http_dclass_kvdata_str *ngx_http_dclass_index(ngx_http_request_t*);
static ngx_int_t ngx_http_dclass_pointer(ngx_http_request_t*, ngx_http_variable_value_t*, uintptr_t);
static ngx_int_t ngx_http_dclass_variable(ngx_http_request_t*, ngx_http_variable_value_t*, uintptr_t);
static void ngx_http_dclass_cleanup(void*);


//config file commands
static ngx_command_t  ngx_http_dclass_commands[] = {

    { ngx_string("dclass"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_dclass_conf_t, enable),
      NULL },
      
    { ngx_string("dclass_def"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_dclass_conf_t, dtree_loc[0]),
      NULL },
      
    { ngx_string("dclass_def1"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_dclass_conf_t, dtree_loc[1]),
      NULL },
      
    { ngx_string("dclass_def2"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_dclass_conf_t, dtree_loc[2]),
      NULL },
      
    { ngx_string("dclass_def3"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_dclass_conf_t, dtree_loc[3]),
      NULL },
      
    { ngx_string("dclass_hfield"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_dclass_conf_t, hfield),
      NULL },

      ngx_null_command
};


//config callback
static ngx_http_module_t  ngx_http_dclass_module_ctx = {
    ngx_http_dclass_add_variables,          /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    ngx_http_dclass_create_conf,            /* create location configuration */
    ngx_http_dclass_merge_conf              /* merge location configuration */
};


//module callbacks
ngx_module_t  ngx_http_dclass_module = {
    NGX_MODULE_V1,
    &ngx_http_dclass_module_ctx,            /* module context */
    ngx_http_dclass_commands,               /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


//creates an uninitialized config
static void *ngx_http_dclass_create_conf(ngx_conf_t *cf)
{
    ngx_http_dclass_conf_t *conf;
    ngx_pool_cleanup_t *cln;

    conf=ngx_pcalloc(cf->pool,sizeof(ngx_http_dclass_conf_t));
    
    if(conf==NULL)
        return NULL;

    conf->enable=NGX_CONF_UNSET;
    
    cln=ngx_pool_cleanup_add(cf->pool, 0);
    
    if(cln==NULL)
        return NULL;
    
    cln->handler=ngx_http_dclass_cleanup;
    cln->data=conf;
    
    return conf;
}


//merges config from parent
static char *ngx_http_dclass_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    int ret,i;
    ngx_http_dclass_conf_t *prev = parent;
    ngx_http_dclass_conf_t *conf = child;
    
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    
    ngx_conf_merge_str_value(conf->hfield, prev->hfield, "");
    
    for(i=0;i<NGX_HTTP_DCLASS_INDEXES;i++)
    {
        if(conf->dtree_loc[i].data)
        {        
            conf->head[i]=ngx_pcalloc(cf->pool,sizeof(dclass_index));

            dclass_init_index(conf->head[i]);

            ret=dclass_load_file(conf->head[i],(char*)conf->dtree_loc[i].data);

            if(ret<=0)
                ret=devicemap_load_resources(conf->head[i],(char*)conf->dtree_loc[i].data);

            if(ret<0)
                return NGX_CONF_ERROR;

            ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,cf->log,0,"dClass: init %d: loaded %zu nodes from '%s'",
                          i,conf->head[i]->dti.node_count,conf->dtree_loc[i].data);
        }
        else if(prev->head[i])
            conf->head[i]=prev->head[i];
    }

    return NGX_CONF_OK;
}


//adds the dclass variable to the config
static ngx_int_t ngx_http_dclass_add_variables(ngx_conf_t *cf)
{
    int i,len;
    u_char buf[128];
    ngx_str_t str;
    ngx_http_variable_t *var;
    
    ngx_str_set(&str,"dclass_ptr");
    
    var=ngx_http_add_variable(cf,&str,NGX_HTTP_VAR_INDEXED);
    
    if(var==NULL)
        return NGX_ERROR;

    var->get_handler=ngx_http_dclass_pointer;
    var->data=0;
    
    ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,cf->log,0,"dClass: variable: '%s'","dclass_ptr");
    
    ngx_str_set(&str,"dclass_id");
    
    var=ngx_http_add_variable(cf,&str,NGX_HTTP_VAR_INDEXED);
    
    if(var==NULL)
        return NGX_ERROR;

    var->get_handler=ngx_http_dclass_variable;
    var->data=0;
    
    ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,cf->log,0,"dClass: variable: '%s'","dclass_id");
    
    len=sizeof(ngx_dclass_key_array)/sizeof(*ngx_dclass_key_array);
    
    for(i=0;i<len;i++)
    {
        ngx_cpystrn(buf,(u_char*)"dclass_",8);
        ngx_cpystrn(buf+7,(u_char*)ngx_dclass_key_array[i],ngx_strlen(ngx_dclass_key_array[i])+1);
        
        str.data=buf;
        str.len=ngx_strlen(buf);
        
        var=ngx_http_add_variable(cf,&str,NGX_HTTP_VAR_INDEXED);
    
        if(var==NULL)
            return NGX_ERROR;

        var->get_handler=ngx_http_dclass_variable;
        var->data=i+1;
        
        ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,cf->log,0,"dClass: variable: '%s'",buf);
    }

    return NGX_OK;
}


//get a value from the kvd
static ngx_int_t ngx_http_dclass_variable(ngx_http_request_t *r,ngx_http_variable_value_t *v,uintptr_t data)
{
    int i;
    ngx_http_dclass_kvdata_str *kvs;
    
    v->valid=1;
    v->escape=0;
    v->no_cacheable=0;
    v->not_found=0;
    v->len=0;
    v->data=NULL;
    
    kvs=ngx_http_dclass_index(r);
    
    if(kvs==NULL || kvs->kvd[0]==NULL)
        return NGX_OK;
    
    if(!data)
    {
        v->data=(u_char*)kvs->kvd[0]->id;
        v->len=ngx_strlen(kvs->kvd[0]->id);
        
        ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,r->connection->log,0,"dClass: lookup: '%s' => '%s'","dclass_id",v->data);
    }
    else
    {
        v->len=0;
        
        for(i=0;i<NGX_HTTP_DCLASS_INDEXES && kvs->kvd[i];i++)
        {
            v->data=(u_char*)dclass_get_kvalue(kvs->kvd[i],ngx_dclass_key_array[data-1]);
            if(v->data)
            {
                v->len=ngx_strlen(v->data);
                break;
            }
        }
        
        ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,r->connection->log,0,"dClass: lookup: '%s' => '%s'",(u_char*)ngx_dclass_key_array[data-1],v->data?v->data:(u_char*)"null");
    }
    
    return NGX_OK;
}


//get the index
static ngx_http_dclass_kvdata_str *ngx_http_dclass_index(ngx_http_request_t *r)
{
    ngx_uint_t i;
    u_char *hfield=NULL;
    ngx_uint_t key;
    ngx_str_t str;
    ngx_http_variable_value_t *val;
    const dclass_keyvalue *kvd;
    ngx_list_part_t *part;
    ngx_table_elt_t *header;
    ngx_http_dclass_conf_t  *cf;
    ngx_http_dclass_kvdata_str *kvs;
    
    cf = ngx_http_get_module_loc_conf(r,ngx_http_dclass_module);
    
    if(!cf->enable || !cf->head[0])
        return NULL;
    
    ngx_str_set(&str,"dclass_ptr");
    
    key=ngx_hash_key(str.data,str.len);
    val=ngx_http_get_variable(r, &str, key);
    
    if(val && val->valid && val->data && !ngx_rstrncmp((u_char*)"ptr",val->data,3))
    {
        kvs=(ngx_http_dclass_kvdata_str*)val->data;
        
        ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,r->connection->log,0,"dClass: classify cache: '%s'",kvs->kvd[0]->id);
        
        return kvs;
    }
    
    if(!*cf->hfield.data)
    {
        if(r->headers_in.user_agent)
            hfield = r->headers_in.user_agent->value.data;
    }
    else
    {
        part=&r->headers_in.headers.part;
        header=part->elts;
        for (i=0;;i++)
        {
            if(i>=part->nelts)
            {
                if(part->next==NULL)
                    break;
                part=part->next;
                header=part->elts;
                i=0;
            }
            if(!ngx_strcasecmp(cf->hfield.data,header[i].key.data))
            {
                hfield=header[i].value.data;
                break;
            }
        }
    }
    
    if(hfield==NULL)
        return NULL;
    
    kvs=(ngx_http_dclass_kvdata_str*)ngx_pcalloc(r->pool,sizeof(ngx_http_dclass_kvdata_str));
    
    if(kvs==NULL)
        return NULL;
    
    for(i=0;i<NGX_HTTP_DCLASS_INDEXES && cf->head[i];i++)
    {
        kvd=dclass_classify(cf->head[i],(char*)hfield);

        kvs->kvd[i]=kvd;
        ngx_cpystrn(kvs->data,(u_char*)"ptr",4);

        ngx_log_error(NGX_HTTP_DCLASS_LOGLEVEL,r->connection->log,0,"dClass: classify %d: '%s' => '%s'",i,hfield,kvd->id);
    }
    
    return kvs;
}

//populate the dclass ptr
static ngx_int_t ngx_http_dclass_pointer(ngx_http_request_t *r,ngx_http_variable_value_t *v,uintptr_t data)
{
    ngx_http_dclass_kvdata_str *kvs;
   
    v->valid=1;
    v->escape=0;
    v->no_cacheable=0;
    v->not_found=0;
    v->len=0;
    v->data=NULL;
    
    kvs=ngx_http_dclass_index(r);
    
    if(kvs==NULL)
        return NGX_OK;
    
    v->data=(u_char*)kvs;
    
    return NGX_OK;
}

//cleanup
static void ngx_http_dclass_cleanup(void *data)
{
    int i;
    ngx_http_dclass_conf_t *conf;
    
    conf=(ngx_http_dclass_conf_t*)data;
    
    for(i=0;i<NGX_HTTP_DCLASS_INDEXES;i++)
    {
        if(conf->head[i])
           dclass_free(conf->head[i]);
    }
}
