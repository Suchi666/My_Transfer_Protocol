gcc -c test1.c -o test1.o
gcc -c test2.c -o test2.o
gcc test2.o test1.o -o my_program -pthread
