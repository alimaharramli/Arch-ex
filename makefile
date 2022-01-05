SHELL = /bin/bash

CC = g++
OBJS = main_ui.cpp compression.cpp decompression.cpp
CFLAG = -std=c++17
PKG = `pkg-config --cflags --libs gtkmm-3.0`
OUT = archex

build:
	${CC} ${CFLAG} ${OBJS} -o ${OUT} ${PKG}
clean:
	rm -f ${OUT}

