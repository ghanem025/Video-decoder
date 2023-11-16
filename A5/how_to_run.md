#check readme to make sure you have everything installed.
# Compiling and running the code
We are using make for this C project. So naturally, we call `make` to compile the source
code into a binary:

# -- COMPILE --
```bash
$ ls
main.c  Makefile  README.md  videos

$ make       # Equivalent: this defaults to first target, which is main

# now you should have the 'example' file created. example takes two arguments
# <input file> (a video) <integer> (the number of frame you want to pick)

$ ./example joe.mp4 30 # here i am running video 'joe.mp' at 30 frames

Sample rate is 44100 for our laptop and the video joe.mp4 has a sample rate of 44100.
