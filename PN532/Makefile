CC = gcc
CFLAGS = -Wall
LIBS = -lwiringPi  # 필요한 라이브러리 링크 추가

SRCS = dictionary.c pn532_rpi.c pn532.c main.c utils.c
OBJS = $(SRCS:.c=.o)
HEADERS = dictionary.h rp532.h rp532_rpi.h pn532.h utils.h

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
