#! /bin/sh

for N in `seq 1 $1`; do (
        echo "# N-queens problem for N=$N"
        echo "ANY;"

        echo -n "all_different("
        for i in `seq 1 $N`; do
            echo -n "X$i"
            if [ $i -ne $N ]; then
                echo -n ", "
            fi
        done
        echo ");"

        echo -n "all_different("
        for i in `seq 1 $N`; do
            echo -n "X$i+$i"
            if [ $i -ne $N ]; then
                echo -n ", "
            fi
        done
        echo ");"

        echo -n "all_different("
        for i in `seq 1 $N`; do
            echo -n "X$i-$i"
            if [ $i -ne $N ]; then
                echo -n ", "
            fi
        done
        echo ");"

        for i in `seq 1 $N`; do
            echo "1 <= X$i; X$i <= $N;"
        done
) > queens$N.txt
done
