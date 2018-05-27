#/bin/bash
temp_in=$(mktemp)
revs=$(mktemp)
if cat - "$2" <<<"NEW A" | "$1" &>/dev/null || grep -q "NEW\|DEL\|?" "$2"; then
    echo "Bad redirection file."
else
    cat - "$2" <<<"NEW A" > "$temp_in"
    cat "$temp_in" - <<<"?$3" | $1 >"$revs"
    sed "s/$/?/" "$revs" | cat "$temp_in" - | "$1" | paste -d' ' "$revs" - | grep " $3$" | cut -f1 -d' '
fi
rm "$temp_in" "$revs"
