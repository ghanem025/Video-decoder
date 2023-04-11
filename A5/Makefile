SOURCE_FILES = ./sound.c
OUTPUT = example

CFLAGS=`pkg-config --cflags cairo gtk4`
LIBS=`pkg-config --libs libpulse cairo gtk4` -lavformat -lswscale -lavdevice -lavformat -lavcodec -lavutil -lpthread -lm -lpulse-simple -lpulse -lswresample
lAV = -lavformat -lswscale -lavdevice -lavformat -lavcodec -lavutil -lpthread -lm -lpulse-simple -lpulse -lswresample

main: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o example
