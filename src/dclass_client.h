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


#ifndef _WX_DCLASS_H_INCLUDED_
#define _WX_DCLASS_H_INCLUDED_


#include "dtree_client.h"


#define DCLASS_VERSION    "dClass 2.0.4"


//key value struct, dtree payload
typedef struct
{
    const char            *id;
    
    const char            **keys;
    const char            **values;
    
    unsigned short int    size;
}
dclass_keyvalue;


//dclass index
typedef struct
{
    dtree_dt_index        dti;
    
    dclass_keyvalue       error;
}
dclass_index;


#include "openddr_client.h"


void dclass_init_index(dclass_index*);

int dclass_load_file(dclass_index*,const char*);
int dclass_write_file(const dclass_index*,const char*);

const dclass_keyvalue *dclass_classify(const dclass_index*,const char*);
const char *dclass_get_kvalue(const dclass_keyvalue*,const char*);

void dclass_free(dclass_index*);

const char *dclass_get_id(void*);

const char *dclass_get_version();


#endif	/* _WX_DCLASS_H_INCLUDED_ */
