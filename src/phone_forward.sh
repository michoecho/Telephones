#/bin/bash
# Skrypt przyjmuje jako poprawny argument $2 dowolny tekst,
# który po dodaniu na początku linii "NEW A" jest poprawnym programem w języku
# phone_forward, który po wykonaniu wszystkich instrukcji posiada obecną bazę.
# Szukanie odwrotności numeru jest wykonywane na tej właśnie bazie.
temp_in=$(mktemp)
revs=$(mktemp)
output=$(mktemp)
temp_output=$(mktemp)
error_log=$(mktemp)

if ! [ -x $1 ]; then echo "Program '$1' not found."; exit 1; fi
if ! [ -r $2 ]; then echo "Input file '$2' unavailable."; exit 1; fi
if ! [[ "$3" =~ ^[0-9]+$ ]]; then echo "'$3' is not a number."; exit 1; fi

cat - "$2" <<<"NEW A" >"$temp_in"
if ! "$1" <"$temp_in" >"$temp_output" 2>"$error_log"; then
    echo -n "Error while parsing input file: " 1>&2
    cat "$error_log" 1>&2
    rm "$temp_in" "$error_log" "$temp_output"
    exit 1
fi
unneeded=$(wc -l <"$temp_output")
((++unneeded))
rm "$temp_output"
cat "$temp_in" - <<<"?$3" | $1 2>"$error_log" | tail -n -$unneeded >"$revs"
sed "s/$/?/" "$revs" | cat "$temp_in" - | "$1" 2>"$error_log" |
    tail -n -$unneeded | paste "$revs" - | grep "\s$3$" | cut -f1 >"$output"

if [ -s "$error_log" ]; then
    echo -n "Runtime error: " 1>&2
    cat "$error_log" 1>&2
    rm "$temp_in" "$revs" "$output" "$error_log"
    exit 1
fi

cat "$output"
rm "$temp_in" "$revs" "$output" "$error_log"
