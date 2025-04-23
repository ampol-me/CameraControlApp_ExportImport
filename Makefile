CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0 json-c` -Wall
LIBS = `pkg-config --libs gtk+-3.0 json-c` -lcurl -ljson-c

OBJS = main.o camera.o http_client.o

camera_app: $(OBJS)
	$(CC) -o camera_app $(OBJS) $(LIBS)

clean:
	rm -f *.o camera_app