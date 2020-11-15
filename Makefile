progname=GL_points
builddir=build
srcdir=src
libs=-Llib -Iinclude -lGL -lGLEW -lglfw

.PHONY: build clean

default: build

build:
	if [ ! -e ${builddir} ];then mkdir ${builddir};fi
	g++ -std=c++11 -O2 -o ${builddir}/${progname} ${srcdir}/Main.cpp ${libs}
clean:
	rm build/*
