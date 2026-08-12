#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

PROG="./codexion"

if [ ! -f "$PROG" ]; then
    echo -e "${RED}Error: Executable $PROG not found. Please compile your program first.${NC}"
    exit 1
fi

# We use valgrind with memcheck and error-exitcode so we catch leaks and thread/runtime issues
VALGRIND="valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --quiet"

run_test() {
    local test_name="$1"
    local args="$2"

    # Run program with valgrind, discarding standard output/error noise unless it fails
    $VALGRIND $PROG $args > /dev/null 2>&1
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        echo -e "$test_name : ${GREEN}PASS${NC}"
    else
        echo -e "$test_name : ${RED}FAIL${NC}"
    fi
}

echo "=== Running All Codexion Tests (with Valgrind leak/thread check) ==="

# Running all cases defined in your script + the tight edge cases added earlier
run_test "1. basic_fifo" "4 800 200 200 200 5 10 fifo"
run_test "2. basic_edf" "4 800 200 200 200 5 10 edf"
run_test "3. success_fifo" "10 10000 100 100 100 5 50 fifo"
run_test "4. large_edf" "20 5000 500 500 500 10 100 edf"
run_test "5. low_cooldown" "5 2000 100 100 100 20 1 fifo"
run_test "6. long_actions" "3 10000 2000 2000 2000 2 100 fifo"
run_test "7. big_test" "100 10000 66 24 87 10 10 fifo"
run_test "8. starvation_fifo" "3 1000 600 10 10 5 100 fifo"
run_test "9. starvation_edf" "3 1000 600 10 10 5 100 edf"
run_test "10. one_compiler_fifo" "1 1000 200 200 200 5 50 fifo"
run_test "11. one_compiler_edf" "1 1000 200 200 200 5 50 edf"
run_test "12. zero_compiles" "5 1000 200 200 200 0 10 fifo"
run_test "13. immediate_burnout" "2 1 200 200 200 5 10 fifo"
run_test "14. cooldown_hell" "2 1000 100 100 100 5 2000 fifo"
run_test "15. max_coders" "300 10000 100 100 100 5 10 edf"
run_test "16. even_squeeze_tight" "4 430 200 200 10 10 10 fifo"
run_test "17. odd_nightmare_edf" "5 640 200 200 100 10 10 edf"
run_test "18. minimalist_stress" "4 120 50 5 5 50 5 fifo"
run_test "19. extreme_loner_edf" "3 345 100 100 10 10 15 edf"
run_test "20. error_arg_non_numeric" "banana 200 300 400 500 5 10 fifo"
run_test "21. error_arg_negative" "10 200 300 -400 500 5 10 edf"