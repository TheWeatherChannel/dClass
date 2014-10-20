#!/bin/bash

DEVICEMAP_RESOURCE_PATH=/some/path/devicemap/latest/devicedata
DCLASS_BIN=../src/dclass_client

if [ ! -d "$DEVICEMAP_RESOURCE_PATH" ]
then
    echo "ERROR: invalid DeviceMap resource path: $DEVICEMAP_RESOURCE_PATH"
    exit 1
fi

if [ ! -s "$DCLASS_BIN" ]
then
    echo "ERROR: dclass_client binary not found"
    exit 1
fi

cp BuilderDataSourcePatch.xml DeviceDataSourcePatch.xml "$DEVICEMAP_RESOURCE_PATH"

$DCLASS_BIN -d "$DEVICEMAP_RESOURCE_PATH" -o devicemap.dtree

