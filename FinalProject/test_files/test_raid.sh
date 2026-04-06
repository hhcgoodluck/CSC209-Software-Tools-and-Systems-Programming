#!/bin/bash

set -u

RAID_EXEC=./raid_sim

PASS=0
FAIL=0

pass() {
    echo "[PASS] $1"
    PASS=$((PASS+1))
}

fail() {
    echo "[FAIL] $1"
    FAIL=$((FAIL+1))
}

cleanup() {
    rm -f disk_*.dat *.trans data* out.txt err.txt
}

make_data() {
    printf "aaaaaaaaaaaaaaaa" > data1
    printf "bbbbbbbbbbbbbbbb" > data2
    printf "cccccccccccccccc" > data3
}

run() {
    "$RAID_EXEC" -n "$1" -t "$2" > out.txt 2> err.txt
}

check_contains() {
    grep -q "$1" out.txt
}

check_file_prefix() {
    local file="$1"
    local expected="$2"
    [ -f "$file" ] && [[ "$(head -c ${#expected} "$file")" == "$expected" ]]
}

##################################################
echo "===== INIT ====="
##################################################

if [ ! -x "$RAID_EXEC" ]; then
    echo "Compile your program first"
    exit 1
fi

pass "Executable exists"

##################################################
echo "===== TEST 1: basic wb ====="
##################################################

cleanup
make_data

cat > t1.trans << EOF
wb 0 data1
wb 1 data2
exit
EOF

run 1 t1.trans

if check_file_prefix disk_0.dat "aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb"; then
    pass "Basic write works"
else
    fail "Basic write failed"
fi

##################################################
echo "===== TEST 2: rb ====="
##################################################

cleanup
make_data

cat > t2.trans << EOF
wb 0 data1
rb 0
exit
EOF

run 1 t2.trans

if check_contains "aaaaaaaaaaaaaaaa"; then
    pass "Read works"
else
    echo "--- stdout ---"
    cat out.txt
    fail "Read failed"
fi

##################################################
echo "===== TEST 3: overwrite ====="
##################################################

cleanup
make_data

cat > t3.trans << EOF
wb 0 data1
wb 0 data3
exit
EOF

run 1 t3.trans

if check_file_prefix disk_0.dat "cccccccccccccccc"; then
    pass "Overwrite works"
else
    fail "Overwrite failed"
fi

##################################################
echo "===== TEST 4: multi block ====="
##################################################

cleanup
make_data

cat > t4.trans << EOF
wb 0 data1
wb 1 data2
wb 2 data1
wb 3 data2
exit
EOF

run 1 t4.trans

EXPECTED="aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb"

if check_file_prefix disk_0.dat "$EXPECTED"; then
    pass "Multi-block works"
else
    fail "Multi-block failed"
fi

##################################################
echo "===== TEST 5: multi disk ====="
##################################################

cleanup
make_data

cat > t5.trans << EOF
wb 0 data1
wb 1 data2
exit
EOF

run 2 t5.trans

if [ -f disk_0.dat ] && [ -f disk_1.dat ] && [ -f disk_2.dat ]; then
    pass "Multi-disk startup works"
else
    fail "Multi-disk failed"
fi

##################################################
echo "===== TEST 6: kill disk ====="
##################################################

cleanup
make_data

cat > t6.trans << EOF
wb 0 data1
kill 0
wb 1 data2
exit
EOF

run 1 t6.trans

if [ $? -eq 0 ]; then
    pass "Kill does not crash"
else
    fail "Kill crashed program"
fi

##################################################
echo "===== TEST 7: kill + read ====="
##################################################

cleanup
make_data

cat > t7.trans << EOF
wb 0 data1
kill 0
rb 0
exit
EOF

run 1 t7.trans

# 如果还能读出数据说明恢复逻辑OK（或容错OK）
if check_contains "aaaaaaaaaaaaaaaa"; then
    pass "Read after kill works"
else
    echo "--- stdout ---"
    cat out.txt
    fail "Read after kill failed"
fi

##################################################
echo "===== TEST 8: multiple kills ====="
##################################################

cleanup
make_data

cat > t8.trans << EOF
wb 0 data1
wb 1 data2
kill 0
kill 1
exit
EOF

run 1 t8.trans

if [ $? -eq 0 ]; then
    pass "Multiple kill handled"
else
    fail "Multiple kill crash"
fi

##################################################
echo "===== TEST 9: invalid block ====="
##################################################

cleanup
make_data

cat > t9.trans << EOF
wb 999 data1
exit
EOF

run 1 t9.trans

if grep -qi "error" out.txt err.txt; then
    pass "Invalid block handled"
else
    echo "Note: your implementation may ignore invalid input"
    pass "Invalid block test (non-strict)"
fi

##################################################
echo "===== SUMMARY ====="
##################################################

echo "PASS: $PASS"
echo "FAIL: $FAIL"

if [ "$FAIL" -eq 0 ]; then
    echo "ALL TESTS PASSED ✅"
else
    echo "SOME TESTS FAILED ❌"
fi
