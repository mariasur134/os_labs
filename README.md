1st lab
Shurupova Maria, М8О-213БВ-25
task:
1 parent process creates 2 child processes, parent sends odd lines to child1, even - child2 for deleting vowels in the string and writing result in a file
input:
<file name of child1>
<file name of child2>
<any number of lines, that end with '\n'>


build:
gcc -o child child.c
gcc -o parent parent.c
(must be in the same folder)

run:
./parent
