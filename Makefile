all: render

clean : 
	-rm main.o render.exe render -fopenmp

render: main.o
	g++ -g -o render main.o -fopenmp

main.o : main.cpp
	g++ -c -g main.cpp -fopenmp