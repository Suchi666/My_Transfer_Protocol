gcc -c test1.c -o test1.o
gcc -c test2.c -o test2.o
gcc test2.o test1.o -o my_program -pthread


# -c: This flag tells GCC to compile the source file(s) into object file(s) without linking. It means that it will only perform the compilation step and generate the object file(s) (.o files), but it won't attempt to create the final executable. This is useful when you have multiple source files and want to compile them separately before linking them together.
# -o : This flag specifies the output file name. In your command, -o test1.o indicates that the output of the compilation should be saved in a file named test1.o. 