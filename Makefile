proj2: gba.o loguru.o
	g++ -g -std=c++11 -o proj2.x gba.o loguru.o

    
gba.o:
	g++ -g -std=c++11 -c gba.cpp
    
loguru.o:
	g++ -std=c++11 -c loguru.cpp

clean:
	rm *.o proj2.x