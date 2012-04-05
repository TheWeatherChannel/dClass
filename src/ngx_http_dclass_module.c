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


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "dclass_client.h"


//settings struct
typedef struct {
    ngx_flag_t     enable;
    ngx_str_t      dtree_loc;
    ngx_str_t      hfield;
    dclass_index   *head;
} ngx_http_dclass_conf_t;


//prototypes
static void* ngx_http_dclass_create_conf(ngx_conf_t*);
static char* ngx_http_dclass_merge_conf(ngx_conf_t*, void*, void*);
static ngx_int_t ngx_http_dclass_add_class_variable(ngx_conf_t*);
static ngx_int_t ngx_http_dclass_class_variable(ngx_http_request_t*, ngx_http_variable_value_t*, uintptr_t);
static void ngx_http_dclass_cleanup(void*);

#if DCLASS_DEBUG
extern void dtree_timersubn(struct timespec*,struct timespec*,struct timespec*);
#endif


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
      offsetof(ngx_http_dclass_conf_t, dtree_loc),
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
    ngx_http_dclass_add_class_variable,     /* preconfiguration */
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
static void * ngx_http_dclass_create_conf(ngx_conf_t *cf)
{
    ngx_http_dclass_conf_t *conf;
    ngx_pool_cleanup_t *cln;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_dclass_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET;
    
    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NULL;
    }
    
    cln->handler = ngx_http_dclass_cleanup;
    cln->data = conf;
    
    return conf;
}


//merges config from parent
static char * ngx_http_dclass_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    int ret=0;
    ngx_http_dclass_conf_t *prev = parent;
    ngx_http_dclass_conf_t *conf = child;
    
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    
    ngx_conf_merge_str_value(conf->hfield, prev->hfield, "");
    
    if(conf->dtree_loc.data)
    {        
        conf->head=ngx_pcalloc(cf->pool,sizeof(dclass_index));
        
        dclass_init_index(conf->head);

        ret=dclass_load_file(conf->head,(char*)conf->dtree_loc.data);
        
        if(ret<=0)
            ret=openddr_load_resources(conf->head,(char*)conf->dtree_loc.data);
        
        if(ret<0)
            return NGX_CONF_ERROR;
    }
    else if(prev->head)
        conf->head=prev->head;

    return NGX_CONF_OK;
}


//adds the dclass variable to the config
static ngx_int_t ngx_http_dclass_add_class_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t *var;
    ngx_str_t wcv;
    
    ngx_str_set(&wcv, "dclass");
    
    var = ngx_http_add_variable(cf, &wcv, NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOHASH);
    
    if (var == NULL)
        return NGX_ERROR;

    var->get_handler = ngx_http_dclass_class_variable;

    return NGX_OK;
}

//populated the dclass variable
static ngx_int_t ngx_http_dclass_class_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_dclass_conf_t  *cf;
    u_char *hfield=NULL;
    ngx_list_part_t *part;
    ngx_table_elt_t *header;
    ngx_uint_t i;
    const dclass_keyvalue *kvd;
    
    cf = ngx_http_get_module_loc_conf(r, ngx_http_dclass_module);
    
    v->valid=1;
    v->escape=0;
    v->no_cacheable=0;
    v->not_found=0;
    
    if(!cf->enable || !cf->head)
    {
        v->data=(u_char*)"error";
        v->len=5;
        return NGX_OK;
    }
    
    if(!*cf->hfield.data)
    {
        if(r->headers_in.user_agent)
            hfield = r->headers_in.user_agent->value.data;
    }
    else
    {
        part = &r->headers_in.headers.part;
        header = part->elts;
        for (i=0;;i++)
        {
            if (i >= part->nelts)
            {
                if (part->next == NULL)
                    break;
                part = part->next;
                header = part->elts;
                i = 0;
            }
            if(!ngx_strcasecmp(cf->hfield.data,header[i].key.data))
            {
                hfield=header[i].value.data;
                break;
            }
        }
    }
    
    if(hfield == NULL)
    {
        v->data=(u_char*)"unknown";
        v->len=7;
        return NGX_OK;
    }
    
    kvd=dclass_classify(cf->head,(char*)hfield);
    
    v->data=(u_char*)kvd->id;
    v->len=ngx_strlen(kvd->id);
    
    return NGX_OK;
}

//cleanup
static void ngx_http_dclass_cleanup(void *data)
{
    ngx_http_dclass_conf_t *conf;
    
    conf=(ngx_http_dclass_conf_t*)data;
    
    if(conf->head)
        dclass_free(conf->head);
}
