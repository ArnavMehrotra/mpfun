CC = /usr/bin/g++

OBJS = mp4Read.o

CPPFLAGS = -std=c++11

%.o : %.c
	$(CC) -c $(CPPFLAGS) $< -o $@

codec : $(OBJS)
	$(CC) -o codec $(OBJS)

clean:
	rm -rf $(OBJS)
