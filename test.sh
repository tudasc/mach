#!/bin/bash

# it will run all the tests, overwriting a.out

#Setup
VERBOSE=true
TEST_FILE=tests/test_cases.txt


# colorize
Red='\033[0;31m'
Green='\033[0;32m'
NC='\033[0m'

run_test () {
test_name=$1
expected_result=$2

# 0 means test failed!
# 1 menas one test succeded!
exitcode=0

if [ "$expected_result" == "no" ]; then
	if [ "$(./run.sh $test_name 2> >(grep "No conflicts detected"))" == "" ]; then
		exitcode=0
	else
		exitcode=1
	fi
elif [ "$expected_result" == "conflict" ]; then

if [ "$(./run.sh $test_name  2> >(grep "Message race conflicts detected"))" == "" ]; then
		exitcode=0
	else
		exitcode=1
	fi
fi

return $exitcode
}

num_tests=0
succesful=0
while read -u 6 line; do

run_test $line
status=$?
if [ "$status" == 0 ]; then
	echo -e "${Red}FAILED${NC}" $line
elif [ "$VERBOSE" == true ]; then
	echo -e "${Green}SUCCES${NC}" $line
fi

num_tests=$(( num_tests + 1))
succesful=$(( succesful + status))

done 6<$TEST_FILE
# not use stdin rather input channel 6

echo succeded at $succesful of $num_tests tests

if [ $succesful  -lt $num_tests ]; then
	echo -e "${Red}FAILED SOME TESTS${NC}"
	exit 1
else
	echo -e "${Green}ALL TESTS PASSED${NC}"
	exit 0
fi




