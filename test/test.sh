#!/bin/bash

DEVICEMAP_RESOURCE=/some/path/devicemap/latest/devicedata
DCLASS_BIN=../src/dclass_client
#DCLASS_BIN="../java/dclass.jar"

if [ ! -s "uas.txt" ]
then
        echo "ERROR: uas.txt not found"
        exit 1
fi

if [ ! -s "$DCLASS_BIN" ]
then
        echo "ERROR: dclass_client binary not found"
        exit 1
fi

#DCLASS_BIN="java -cp ../java/dclass.jar dclass.dClassMain"

CFLAG=`$DCLASS_BIN test.txt | grep "UA lookup 5: '" | wc -l`

if [ $CFLAG != "1" ]
then
	echo "ERROR: binary not compiled correctly"
	echo "Make sure DTREE_TEST_UALOOKUP is set to 1"
	exit 1
fi

if [ -d "$DEVICEMAP_RESOURCE" ]
then
	$DCLASS_BIN -d "$DEVICEMAP_RESOURCE" uas.txt | grep "^UA lookup " | sed "s/time.*$//" > uas.devicemap.testd

	TESTDDIFF=`diff uas.devicemap uas.devicemap.testd | grep "^<" | sed "s/^.* lookup //" | sed "s/: '.*$//" | xargs echo`

	if [ "$TESTDDIFF" != "" ]
	then
		echo "ERROR: Test DeviceMap failed"
		echo "Lines in uas.devicemap: $TESTDDIFF"
		echo "Move uas.devicemap.testd to uas.devicemap if test is correct"
		exit 1
	else
		echo "PASSED: Test DeviceMap"
	fi
else
	echo "SKIPPED: Test DeviceMap"
fi

$DCLASS_BIN -l ../dtrees/devicemap.dtree -o test2.dtree uas.txt | grep "^UA lookup " | sed "s/time.*$//" > uas.devicemap.test1

TEST1DIFF=`diff uas.devicemap uas.devicemap.test1 | grep "^<" | sed "s/^.* lookup //" | sed "s/: '.*$//" | xargs echo`

if [ "$TEST1DIFF" != "" ]
then
	echo "ERROR: Test 1 failed"
	echo "Lines in uas.devicemap: $TEST1DIFF"
	echo "Move uas.devicemap.test1 to uas.devicemap if test is correct"
	exit 1
else
	echo "PASSED: Test 1"
fi

$DCLASS_BIN -l test2.dtree uas.txt | grep "^UA lookup " | sed "s/time.*$//" > uas.devicemap.test2

TEST2DIFF=`diff uas.devicemap uas.devicemap.test2 | grep "^<" | sed "s/^.* lookup //" | sed "s/: '.*$//" | xargs echo`

if [ "$TEST2DIFF" != "" ]
then
	echo "ERROR: Test 2 failed"
	echo "Lines: $TEST2DIFF"
	exit 1
else
	echo "PASSED: Test 2"
fi

$DCLASS_BIN -l ../dtrees/browser.dtree uas.txt | grep "^UA lookup " | sed "s/time.*$//" > uas.browser.test3

TEST3DIFF=`diff uas.browser uas.browser.test3 | grep "^<" | sed "s/^.* lookup //" | sed "s/: '.*$//" | xargs echo`

if [ "$TEST3DIFF" != "" ]
then
	echo "ERROR: Test 3 failed"
	echo "Lines: $TEST3DIFF"
	echo "Move uas.browser.test3 to uas.browser if test is correct"
	exit 1
else
	echo "PASSED: Test 3"
fi

$DCLASS_BIN -l ../dtrees/test.dtree test.txt -o test5.dtree | grep "^UA lookup " | sed "s/time.*$//" > test.out.test4

TEST4DIFF=`diff test.out test.out.test4 | grep "^<" | sed "s/^.* lookup //" | sed "s/: '.*$//" | xargs echo`

if [ "$TEST4DIFF" != "" ]
then
	echo "ERROR: Test 4 failed"
	echo "Lines: $TEST4DIFF"
	echo "Move test.out.test4 to test.out if test is correct"
	exit 1
else
	echo "PASSED: Test 4"
fi

$DCLASS_BIN -l test5.dtree test.txt | grep "^UA lookup " | sed "s/time.*$//" > test.out.test5

TEST5DIFF=`diff test.out test.out.test5 | grep "^<" | sed "s/^.* lookup //" | sed "s/: '.*$//" | xargs echo`

if [ "$TEST5DIFF" != "" ]
then
	echo "ERROR: Test 5 failed"
	echo "Lines: $TEST5DIFF"
	exit 1
else
	echo "PASSED: Test 5"
fi

rm uas.devicemap.test* uas.browser.test* test2.dtree test.out.test* test5.dtree > /dev/null 2>&1

echo "All tests completed"

exit 0
