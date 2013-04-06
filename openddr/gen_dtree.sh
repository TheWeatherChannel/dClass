#!/bin/bash

OPENDDR_RESOURCE_PATH=/some/path/OpenDDR/latest/resources
DCLASS_BIN=../src/dclass_client

if [ ! -d "$OPENDDR_RESOURCE_PATH" ]
then
    echo "ERROR: invalid OpenDDR resource path: $OPENDDR_RESOURCE_PATH"
    exit 1
fi

if [ ! -s "$DCLASS_BIN" ]
then
    echo "ERROR: dclass_client binary not found"
    exit 1
fi

cp BuilderDataSourcePatch.xml DeviceDataSourcePatch.xml "$OPENDDR_RESOURCE_PATH"

$DCLASS_BIN -d "$OPENDDR_RESOURCE_PATH" -o openddr.dtree

