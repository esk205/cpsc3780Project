SimpleHeader.o:SimpleHeader.cc
	g++ SimpleHeader.cc -c -o SimpleHeader.o
bouner:SimpleHeader.o bouner.cc
	g++ bouner.cc SimpleHeader.o -lz -lpthread -g -o bouner
	rm SimpleHeader.o
clear:
	rm -f bouner
