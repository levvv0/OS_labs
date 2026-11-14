#!/bin/sh

if [ "$#" -ne 1 ]; then
    echo "Использование: $0 <файл.cpp>" >&2
    exit 1
fi

source_file=$1

if [ ! -f "$source_file" ]; then
    echo "Ошибка: файл '$source_file' не найден" >&2
    exit 2
fi

case "$source_file" in
    *.cpp) ;;
    *)
        echo "Ошибка: требуется исходный файл с расширением .cpp" >&2
        exit 3
        ;;
esac

output_name=$(sed -n \
    's|^[[:space:]]*//[[:space:]]*Output:[[:space:]]*\([A-Za-z0-9][A-Za-z0-9._-]*\)[[:space:]]*$|\1|p' \
    "$source_file" | head -n 1)

if [ -z "$output_name" ]; then
    echo "Ошибка: не найден комментарий '// Output: имя_файла'" >&2
    exit 4
fi

source_name=$(basename "$source_file")
source_dir=$(dirname "$source_file")

if [ "$output_name" = "$source_name" ]; then
    echo "Ошибка: выходной файл не должен совпадать с исходным" >&2
    exit 4
fi

temp_dir=
pending_signal=
cleanup() {
    if [ -n "$temp_dir" ]; then
        rm -rf "$temp_dir"
    fi
}

handle_signal() {
    if [ -n "$temp_dir" ]; then
        exit "$1"
    fi
    pending_signal=$1
}

trap cleanup 0

trap 'handle_signal 129' HUP
trap 'handle_signal 130' INT
trap 'handle_signal 131' QUIT
trap 'handle_signal 134' ABRT
trap 'handle_signal 138' USR1
trap 'handle_signal 140' USR2
trap 'handle_signal 141' PIPE
trap 'handle_signal 142' ALRM
trap 'handle_signal 143' TERM
trap 'handle_signal 152' XCPU
trap 'handle_signal 153' XFSZ

temp_dir=$(mktemp -d) || {
    echo "Ошибка: не удалось создать временный каталог" >&2
    exit 5
}

if [ -n "$pending_signal" ]; then
    exit "$pending_signal"
fi

if ! cp "$source_file" "$temp_dir/$source_name"; then
    echo "Ошибка: не удалось скопировать исходный файл" >&2
    exit 6
fi

if ! (
    cd "$temp_dir" || exit 1
    g++ -std=c++17 -Wall -Wextra -Wpedantic \
        "./$source_name" -o "./$output_name"
); then
    echo "Ошибка: компиляция завершилась неудачно" >&2
    exit 7
fi

if ! mv "$temp_dir/$output_name" "$source_dir/$output_name"; then
    echo "Ошибка: не удалось сохранить выходной файл" >&2
    exit 8
fi

echo "Сборка завершена: $source_dir/$output_name"
exit 0
