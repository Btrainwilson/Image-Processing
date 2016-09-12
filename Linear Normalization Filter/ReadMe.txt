This folder contains source code for an image filter designed for linear normalization (High contrast filter)
This was an assignment I'm very proud of. Source code is found in pa05.c

Compile with proper main file and pa05.c in gcc

Pass unfiltered image name with extension as argument 1
Pass filtered image save name for argument 2 with proper extension

main.c handles bitmap files
main2.c handles the custom .ee264 extension.

//Error Handling
The code will handle any corrupt file errors such as improper headers and pixel array size. It is fully tested against Purdue's rigorous 
bash tester, some 50+ tests.


Other files, ie diff-plus and images, are compare files to test our code for accuracy. Some of diff-plus was rewritten by myself, however 
the structure was from my professor