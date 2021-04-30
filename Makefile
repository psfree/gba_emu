proj2: gba.o loguru.o alu.o
	g++ -g -std=c++11 -o proj2.x gba.o loguru.o alu.o

alu.o:
	g++ -g -std=c++11 -c alu.cpp
gba.o:
	g++ -g -std=c++11 -c gba.cpp
    
loguru.o:
	g++ -std=c++11 -c loguru.cpp

clean:
	rm *.o proj2.x
