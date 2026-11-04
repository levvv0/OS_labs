#!/bin/sh
set -eu

shared_dir=/shared
lock_file="$shared_dir/.lock"
container_id=$(cat /proc/sys/kernel/random/uuid)
counter=0
current_file=

exec 9>> "$lock_file"

cleanup() {
    if [ -n "$current_file" ]; then
        flock -x 9
        rm -f "$current_file"
        current_file=
        flock -u 9
    fi
}

trap cleanup 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

while :; do
    flock -x 9

    number=1
    while :; do
        name=$(printf '%03d' "$number")
        candidate="$shared_dir/$name"

        if [ ! -e "$candidate" ]; then
            current_file=$candidate
  	    counter=$((counter + 1))
   	    printf '%s %s\n' "$container_id" "$counter" > "$current_file"
            break
        fi

        number=$((number + 1))
    done
    flock -u 9

    sleep 1

    flock -x 9
    rm "$current_file"
    current_file=
    flock -u 9

    sleep 1
done
