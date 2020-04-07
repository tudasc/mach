#!/bin/bash

# it will run all the tests, overwriting a.out

#Setup
VERBOSE=true
TEST_FILE=tests/test_cases.txt


# colorize
Red='\033[0;31m'
Green='\033[0;32m'
Yellow='\033[0;33m' 
NC='\033[0m'

run_test () {
test_name=$1
expected_result=$2

# 0 means test failed!
# 1 means one test succeded!
exitcode=0

output=$(./run.sh $test_name 2>&1)

if [ "$( echo $output | grep "Successfully executed the pass")" == "" ]; then
	exitcode=2
	# failed executing this code
elif [ "$expected_result" == "no" ]; then
	if [ "$( echo $output | grep "No conflicts detected")" == "" ]; then
		exitcode=0
	else
		exitcode=1
	fi
elif [ "$expected_result" == "conflict" ]; then
	if [ "$( echo $output | grep "Message race conflicts detected")" == "" ]; then
		exitcode=0
	else
		exitcode=1
	fi
fi

return $exitcode
}

num_tests=0
succesful=0
false_positive=0
while read -u 6 line; do

run_test $line
status=$?

if [ "$status" == 2 ]; then
	echo -e "${Red}CRASHED${NC}" $line
elif [ "$status" == 0 ] && [ "$expected_result" == "conflict" ]; then
	echo -e "${Red}FAILED${NC}" $line
elif [ "$status" == 0 ] && [ "$expected_result" == "no" ]; then
	echo -e "${Yellow}FALSE POSITIVE${NC}" $line
	false_positive=$(( false_positive + 1 ))
elif [ "$VERBOSE" == true ]; then
	echo -e "${Green}SUCCES${NC}" $line
	succesful=$(( succesful + 1 ))
fi

num_tests=$(( num_tests + 1 ))

done 6<$TEST_FILE
# not use stdin rather use input channel 6

echo "succeded at $succesful (+${false_positive}) of $num_tests tests"

if [ $succesful  -lt $((num_tests - false_positive)) ]; then
	echo -e "${Red}FAILED SOME TESTS${NC}"
	exit 1
elif [ $succesful  -lt $num_tests ]; then
	echo -e "${Yellow}SOME FALSE POSITIVES${NC}"
	exit 1
else
	echo -e "${Green}ALL TESTS PASSED${NC}"
	exit 0
fi




