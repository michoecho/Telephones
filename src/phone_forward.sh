#/bin/bash
temp_in=$(mktemp)
revs=$(mktemp)
output=$(mktemp)
error_log=$(mktemp)

cat - "$2" <<<"NEW A" > "$temp_in"
if ! "$1" <"$temp_in" >/dev/null 2>"$error_log"; then
    echo -n "Error while parsing input file: " 1>&2
    cat "$error_log" 1>&2
    rm "$temp_in" "$error_log"
    exit 1
fi
if grep -o -m1 "NEW\|DEL\|?" "$2" >"$error_log"; then
    echo -n "Illegal token in input file: " 1>&2
    cat "$error_log" 1>&2
    rm "$temp_in" "$error_log"
    exit 1
fi
cat "$temp_in" - <<<"?$3" | $1 >"$revs" 2>"$error_log"
sed "s/$/?/" "$revs" | cat "$temp_in" - | "$1" 2>"$error_log" |
    paste "$revs" - | grep "\s$3$" | cut -f1 >"$output"
if [ -s "$error_log" ]; then
    echo -n "Runtime error in $3: " 1>&2
    cat "$error_log" 1>&2
    rm "$temp_in" "$revs" "$output" "$error_log"
    exit 1
fi
cat "$output"
rm "$temp_in" "$revs" "$output" "$error_log"
