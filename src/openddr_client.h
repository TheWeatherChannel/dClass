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


#ifndef _DCLASS_OPENDDR_CLIENT_H
#define _DCLASS_OPENDDR_CLIENT_H


#include "dtree_client.h"
#include "dclass_client.h"


//resource files
#define OPENDDR_RSRC_DD       "DeviceDataSource.xml"
#define OPENDDR_RSRC_DDP      "DeviceDataSourcePatch.xml"
#define OPENDDR_RSRC_BD       "BuilderDataSource.xml"
#define OPENDDR_RSRC_BDP      "BuilderDataSourcePatch.xml"


#define OPENDDR_COMMENT       "OpenDDR %s http://www.openddr.org (device detection ddr)"


#define OPENDDR_KEYS          {                            \
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
                                "is_crawler",              \
                                "is_desktop"               \
                              }


int openddr_load_resources(dclass_index*,const char*);


//openddr blackberry fix
#define OPENDDR_BLKBRY_FIX    1


#endif	/* _DCLASS_OPENDDR_CLIENT_H */

