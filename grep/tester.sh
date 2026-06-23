#!/bin/bash
PASS=0
FAIL=0

GREP=./test_bin

strip() {
	sed 's/\x1b\[[0-9;]*m//g'
}

check() {
	desc=$1
	actual=$2
	expect=$3
	echo -n "$desc"
	if [ "$actual" = "$expect" ]; then
		echo "PASS"
		((PASS++))
	else
		printf "\033[31mFAIL\033[0m\n"
		((FAIL++))
	fi
}


printf "\n------- test cases --------\n"
n=1
check "$((n++)). grep ..." "$(echo "hello" | "$GREP" hello | strip)" "$(echo "hello" | grep hello)"
check "$((n++)). grep -i ..." "$(echo "hello" | "$GREP" -i hello | strip)" "$(echo "hello" | grep -i hello)"
check "$((n++)). grep -v ..." "$(echo "hello" | "$GREP" -v hello | strip)" "$(echo "hello" | grep -v hello)"
check "$((n++)). grep -F ..." "$(echo "hello" | "$GREP" -F hello | strip)" "$(echo "hello" | grep -F hello)"

check "$((n++)). grep -ivF ..." "$(echo "hello" | "$GREP" -ivF hello | strip)" "$(echo "hello" | grep -ivF hello)"
check "$((n++)). grep -ivF -A 1 ..." "$(echo "hello" | "$GREP" -ivF -A 1 hello | strip)" "$(echo "hello" | grep -ivF -A 1 hello)"
check "$((n++)). grep -ivF -A -B 1 ..." "$(printf "nah\nxxx\nmeh" | "$GREP" -ivF -A -B 1 xxx 2>/dev/null | strip 2>/dev/null )" \
	"$(printf "nah\n...eh" | grep -ivF -A -B 1 xxx 2>/dev/null)"
check "$((n++)). grep -ivF -A 1 -B 1 ..." "$(printf "nah\nxxx\nmeh" | "$GREP" -ivF -A 1 -B 1 xxx 2>/dev/null | strip 2>/dev/null )" \
	"$(printf "nah\nxxx\nmeh" | grep -ivF -A 1 -B 1 xxx 2>/dev/null)"
check "$((n++)). grep -ivF -AB 1 ..." "$(printf "nah\nxxx\nmeh" | "$GREP" -ivF -AB 1 xxx 2>/dev/null | strip 2>/dev/null)" \
	"$(printf "nah\nxxx\nmeh" | grep -ivF -AB 1 xxx 2>/dev/null)"
check "$((n++)). grep -C 1 ..." "$(printf "nah\nxxx\nmeh" | "$GREP" -ivF -C 1 xxx 2>/dev/null | strip 2>/dev/null)" \
	"$(printf "nah\nxxx\nmeh" | grep -ivF -C 1 xxx 2>/dev/null)"

check "$((n++)). grep -A 1 -B 2 ..." "$(printf "a\nmatch\nb\nb\nc\nd\nmatch\nc" | "$GREP" -A 1 -B 2 match | strip)" \
    "$(printf "a\nmatch\nb\nb\nc\nd\nmatch\nc" | grep -A 1 -B 2 match)"
check "$((n++)). grep -A 1 -B 2 -A 2 (non-contig ranges)..." "$(printf "a\nmatch\nb\nb\nc\nd\nmatch\nc" | "$GREP" -A 1 -B 2 -A 2 match | strip)" \
    "$(printf "a\nmatch\nb\nb\nc\nd\nmatch\nc" | grep -A 1 -B 2 -A 2 match)"
check "$((n++)). grep -B 2 -A 2 (overlap ranges)..." "$(printf "a\nmatch\nb\nmatch\nc" | "$GREP" -A 1 -B 2 -A 2 match | strip)" \
    "$(printf "a\nmatch\nb\nmatch\nc" | grep -A 1 -B 2 -A 2 match)"
check "$((n++)). grep -B 2 -A 2 (contiguous ranges)..." "$(printf "a\nmatch\nb\nc\nmatch\nz" | "$GREP" -A 1 -B 2 -A 2 match | strip)" \
    "$(printf "a\nmatch\nb\nc\nmatch\nz"| grep -A 1 -B 2 -A 2 match)"

# arg overwrites
check "$((n++)). grep -C 2 -A 1 (arg overwrite)..." "$(printf "a\nmatch\nb\nc\nmatch\nz" | "$GREP" -C 2 -A 1 match | strip)" \
    "$(printf "a\nmatch\nb\nc\nmatch\nz" | grep -C 2 -A 1 match)"

printf "\n------- exit code --------\n"

echo "hello..." | "$GREP" foo >/dev/null 2>&1
check "$((n++)). no match, exits 1..." "$?" "1"

echo "hello..." | "$GREP" hello >/dev/null 2>&1
check "$((n++)). match exits 0..." "$?" "0"

printf "\n------- results --------\n"

echo "PASS:" "$PASS"
echo "FAIL:" "$FAIL"
