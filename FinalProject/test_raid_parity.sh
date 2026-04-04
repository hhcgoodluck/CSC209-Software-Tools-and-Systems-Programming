#!/bin/bash

set -u

RAID_EXEC=./raid_sim
PASS=0
FAIL=0

pass() {
    echo "[PASS] $1"
    PASS=$((PASS + 1))
}

fail() {
    echo "[FAIL] $1"
    FAIL=$((FAIL + 1))
}

cleanup() {
    rm -f disk_*.dat
    rm -f *.trans
    rm -f data1 data2 data3 data4
    rm -f out.txt err.txt
    rm -f parity_check.py
}

make_data() {
    printf "aaaaaaaaaaaaaaaa" > data1
    printf "bbbbbbbbbbbbbbbb" > data2
    printf "cccccccccccccccc" > data3
    printf "dddddddddddddddd" > data4
}

run_sim() {
    local ndisks="$1"
    local transfile="$2"
    "$RAID_EXEC" -n "$ndisks" -t "$transfile" > out.txt 2> err.txt
    return $?
}

make_parity_checker() {
cat > parity_check.py << 'PYEOF'
import sys
from pathlib import Path

def xor_bytes(buffers):
    if not buffers:
        return b""
    length = len(buffers[0])
    out = bytearray(length)
    for i in range(length):
        v = 0
        for b in buffers:
            v ^= b[i]
        out[i] = v
    return bytes(out)

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 parity_check.py <num_data_disks>")
        sys.exit(2)

    num_data = int(sys.argv[1])
    total_disks = num_data + 1
    parity_index = num_data

    files = [Path(f"disk_{i}.dat") for i in range(total_disks)]
    for f in files:
        if not f.exists():
            print(f"Missing file: {f}")
            sys.exit(1)

    contents = [f.read_bytes() for f in files]
    lengths = [len(c) for c in contents]

    if len(set(lengths)) != 1:
        print("Disk files do not all have same size:", lengths)
        sys.exit(1)

    data_contents = contents[:num_data]
    parity_content = contents[parity_index]

    expected_parity = xor_bytes(data_contents)

    if expected_parity != parity_content:
        print("Parity mismatch detected.")
        for i, (a, b) in enumerate(zip(expected_parity, parity_content)):
            if a != b:
                print(f"First mismatch at byte offset {i}: expected {a:#04x}, got {b:#04x}")
                break
        sys.exit(1)

    print("Parity check passed.")
    sys.exit(0)

if __name__ == "__main__":
    main()
PYEOF
}

echo "========================================"
echo "RAID 4 Parity Test"
echo "========================================"

if [ ! -x "$RAID_EXEC" ]; then
    echo "Error: $RAID_EXEC not found or not executable"
    exit 1
fi

pass "Executable found"

cleanup
make_data
make_parity_checker

########################################
# Test 1: parity check with n=2
########################################
echo
echo "========================================"
echo "Test 1: parity validation with 2 data disks"
echo "========================================"

cat > test_parity_2.trans << EOF
wb 0 data1
wb 1 data2
wb 2 data3
wb 3 data4
exit
EOF

if run_sim 2 test_parity_2.trans; then
    if python3 parity_check.py 2; then
        pass "Parity correct for n=2"
    else
        echo "--- stdout ---"
        cat out.txt
        echo "--- stderr ---"
        cat err.txt
        fail "Parity incorrect for n=2"
    fi
else
    echo "--- stdout ---"
    cat out.txt
    echo "--- stderr ---"
    cat err.txt
    fail "Program crashed on parity test n=2"
fi

########################################
# Test 2: parity check with overwrite
########################################
echo
echo "========================================"
echo "Test 2: parity after overwrite"
echo "========================================"

cleanup
make_data
make_parity_checker

cat > test_parity_overwrite.trans << EOF
wb 0 data1
wb 1 data2
wb 0 data3
wb 1 data4
exit
EOF

if run_sim 2 test_parity_overwrite.trans; then
    if python3 parity_check.py 2; then
        pass "Parity correct after overwrite"
    else
        echo "--- stdout ---"
        cat out.txt
        echo "--- stderr ---"
        cat err.txt
        fail "Parity incorrect after overwrite"
    fi
else
    echo "--- stdout ---"
    cat out.txt
    echo "--- stderr ---"
    cat err.txt
    fail "Program crashed on overwrite parity test"
fi

########################################
# Test 3: parity check with n=3
########################################
echo
echo "========================================"
echo "Test 3: parity validation with 3 data disks"
echo "========================================"

cleanup
make_data
make_parity_checker

cat > test_parity_3.trans << EOF
wb 0 data1
wb 1 data2
wb 2 data3
wb 3 data4
wb 4 data1
wb 5 data2
exit
EOF

if run_sim 3 test_parity_3.trans; then
    if python3 parity_check.py 3; then
        pass "Parity correct for n=3"
    else
        echo "--- stdout ---"
        cat out.txt
        echo "--- stderr ---"
        cat err.txt
        fail "Parity incorrect for n=3"
    fi
else
    echo "--- stdout ---"
    cat out.txt
    echo "--- stderr ---"
    cat err.txt
    fail "Program crashed on parity test n=3"
fi

########################################
# Summary
########################################
echo
echo "========================================"
echo "Summary"
echo "========================================"
echo "PASS: $PASS"
echo "FAIL: $FAIL"

if [ "$FAIL" -eq 0 ]; then
    echo "All parity tests passed."
    exit 0
else
    echo "Some parity tests failed."
    exit 1
fi
