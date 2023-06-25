#check readme to make sure you have everything installed.
# Compiling and running the code
I have also attached a test.m4v video for reference.
We are using make for this C project. So naturally, we call `make` to compile the source
code into a binary:

# -- COMPILE --
```bash
$ cd A5
$ ls
main.c  Makefile  README.md  videos

$ make       # Equivalent: this defaults to first target, which is main

# now you should have the 'example' file created. example takes two arguments
# <input file> (a video) <integer> (the number of frame you want to pick)

$ ./example joe.mp4 30 # here i am running video 'joe.mp' at 30 frames

Sample rate is 44100 for our laptop and the video joe.mp4 has a sample rate of 44100.
The volume might be high-pitched but the speech in the video should be clear. I also added a video to our assignment as a bonus.
