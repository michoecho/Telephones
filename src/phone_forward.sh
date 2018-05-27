#/bin/bash
temp_in=$(mktemp)
revs=$(mktemp)
output=$(mktemp)
error_log=$(mktemp)

cat - "$2" <<<"NEW A" > "$temp_in"
if ! "$1" <"$temp_in" &>/dev/null || grep -q "NEW\|DEL\|?" "$2"; then
    echo "Error while parsing input file."
    rm "$temp_in"
    exit 1
else
    cat "$temp_in" - <<<"?$3" | $1 >"$revs" 2>"$error_log"
    sed "s/$/?/" "$revs" | cat "$temp_in" - | "$1" 2>"$error_log" | paste "$revs" - | grep "\s$3$" | cut -f1 >$output
    if [ -s "$error_log" ]; then
        echo "Execution error in $3."
        rm "$temp_in" "$revs" "$output" "$error_log"
        exit 1
    fi
fi
cat "$output"
rm "$temp_in" "$revs" "$output" "$error_log"
