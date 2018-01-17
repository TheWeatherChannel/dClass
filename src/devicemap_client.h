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

#ifndef _DCLASS_DEVICEMAP_CLIENT_H
#define _DCLASS_DEVICEMAP_CLIENT_H

#include "dclass_client.h"
#include "dtree_client.h"

// resource files
#define DEVICEMAP_RSRC_DD "DeviceDataSource.xml"
#define DEVICEMAP_RSRC_DDP "DeviceDataSourcePatch.xml"
#define DEVICEMAP_RSRC_BD "BuilderDataSource.xml"
#define DEVICEMAP_RSRC_BDP "BuilderDataSourcePatch.xml"

#define DEVICEMAP_COMMENT                                                      \
  "DeviceMap %s http://devicemap.apache.org/ (device detection ddr)"

#define DEVICEMAP_KEYS                                                         \
  {                                                                            \
    "vendor", "model", "parentId", "inputDevices", "displayHeight",            \
        "displayWidth", "device_os", "ajax_support_javascript", "is_tablet",   \
        "is_wireless_device", "is_crawler", "is_desktop"                       \
  }

int devicemap_load_resources(dclass_index *, const char *);

// openddr blackberry fix
#define OPENDDR_BLKBRY_FIX 1

#endif /* _DCLASS_DEVICEMAP_CLIENT_H */
