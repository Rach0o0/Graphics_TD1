all: geometry

clean : 
	-rm main.o  geometry

geometry: main.o
	g++ -g -o geometry main.o -L/usr/local/lib -llbfgs -O3

main.o : main.cpp
	g++ -c -g main.cpp -L/usr/local/lib -llbfgs -O3