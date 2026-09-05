#!/bin/sh

count=$#
sum=0

for num in "$@"; do
    sum=$((sum + num))
done

average=$((sum / count))
remainder=$((sum % count))

echo "Количество: $count"
echo "Сумма: $sum"
echo "Среднее: $average"
echo "Остаток: $remainder"