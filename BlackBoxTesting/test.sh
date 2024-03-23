#!/bin/bash
# This is a comment

./clean_os.sh
rm *.txt
rm *~
make clean
make
echo
echo "Six tests will be performed and each test will take around 4 seconds."

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
sed 's,replace,'"$DIR"',' commandFile1.dat > commandFile1.txt
sed 's,replace,'"$DIR"',' commandFile2.dat > commandFile2.txt
sed 's,replace,'"$DIR"',' commandFile3.dat > commandFile3.txt

./mapper commandFile1.txt 10&
sleep 1
./reducer student_out1.txt 1
sleep 3

if diff -w student_out1.txt correct_output/correct_out1.txt; then
    echo Test 1: Success-----------------------------------------------Success
else
    echo Test 1: Fail--------------------------------------------------Fail
fi

./mapper commandFile2.txt 10&
sleep 1
./reducer student_out2.txt 1
sleep 3
if diff -w student_out2.txt correct_output/correct_out2.txt; then
    echo Test 2: Success-----------------------------------------------Success
else
    echo Test 2: Fail--------------------------------------------------Fail
fi

./mapper commandFile3.txt 10&
sleep 1
./reducer student_out3.txt 1
sleep 3
if diff -w student_out3.txt correct_output/correct_out3.txt; then
    echo Test 3: Success-----------------------------------------------Success
else
    echo Test 3: Fail--------------------------------------------------Fail
fi

./mapper commandFile1.txt 10&
sleep 1
./reducer student_out4.txt 3
sleep 3

if diff -w student_out4.txt correct_output/correct_out4.txt; then
    echo Test 4: Success-----------------------------------------------Success
else
    echo Test 4: Fail--------------------------------------------------Fail
fi

./mapper commandFile2.txt 10&
sleep 1
./reducer student_out5.txt 3
sleep 3
if diff -w student_out5.txt correct_output/correct_out5.txt; then
    echo Test 5: Success-----------------------------------------------Success
else
    echo Test 5: Fail--------------------------------------------------Fail
fi

./mapper commandFile3.txt 10&
sleep 1
./reducer student_out6.txt 3
sleep 3
if diff -w student_out6.txt correct_output/correct_out6.txt; then
    echo Test 6: Success-----------------------------------------------Success
else
    echo Test 6: Fail--------------------------------------------------Fail
fi
