all: render

clean : 
	-rm main.o  render

render: main.o
	g++ -g -o render main.o -fopenmp -O3

main.o : main.cpp
	g++ -c -g main.cpp -fopenmp -O3