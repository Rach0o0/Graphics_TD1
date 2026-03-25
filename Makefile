all: render

clean : 
	-rm main.o render.exe render

render: main.o
	g++ -g -o render main.o

main.o : main.cpp
	g++ -c -g main.cpp