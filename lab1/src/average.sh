#!/bin/bash
sum=0
if [ $# -eq 0 ]; then
    echo "Ошибка: Не передано ни одного числа!"
    exit 1
fi
count=$#
for num in "$@"; do
    sum=$((sum + num))
done

average=$((sum / count))

echo "Количество: $count"
echo "Среднее: $average"

