# Prerequisites
Make sure you have the following installed:

## gcc
- [WSL on windows](https://learn.microsoft.com/en-us/windows/wsl/install)
- MacOS: `brew install gcc`
- Linux
	- Arch based: `pacman -S gcc`
	- Debian based: `apt install gcc`


Verify installation:
```bash
$ gcc -v
...
gcc version 12.2.0 (GCC)
```

Version doesn't matter, as long as it's somewhat recent like > `10.0.0`

## make
- Use WSL on Windows
- MacOS: `brew install make`
- Linux
	- Arch based: `pacman -S make`
	- Debian based: `apt install make`

Verify installation:
```bash
$ make -v
GNU Make 4.3
...
```
Again version doesn't matter, as long as it's somewhat recent


## GTK 4
- Use whatever distro you have on WSL
- MacOS: `brew install gtk4`
- Linux
	- Arch based: `pacman -S gtk4`
	- Debian based: `apt install libgtk-4-dev`


<!-- TODO: Cairo installation -->


# Compiling and running the code

We are using make for this C project. So naturally, we call `make` to compile the source
code into a binary:

```bash
# -- COMPILE --
$ ls
main.c  Makefile  README.md  videos
$ make       # Equivalent: this defaults to first target, which is main
# now you should have the 'example' file created. example takes two arguments
# <input file> (a video) <integer> (the number of frame you want to pick)
$ ./example test.m4v 20 # here i am running video 'test.m4v' and I want frame number 20
```
Now you should see a GTK window with 5 images, they should all be the same image but with different grayscale effects
# Clean build

If for some reason there's a problem with the compiled binary, call `make clean` before
building the code:

```bash
$ make clean  # Cleans bin/
$ make main
```



