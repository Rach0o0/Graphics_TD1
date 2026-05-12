all: color

clean : 
	-rm main.o  color

color: main.o
	g++ -g -o color main.o -fopenmp -O3

main.o : main.cpp
	g++ -c -g main.cpp -fopenmp -O3