#/bin/bash
temp_in=$(mktemp)
revs=$(mktemp)
cat - "$2" <<<"NEW A" >> "$temp_in"
cat "$temp_in" - <<<"?$3" | $1 >"$revs"
sed "s/$/?/" "$revs" | cat "$temp_in" - | "$1" | paste -d' ' "$revs" - | grep " $3$" | cut -f1 -d' '
rm "$temp_in" "$revs"
