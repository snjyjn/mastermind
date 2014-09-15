
all: mastermind

clean:
	rm -f mastermind 

mastermind: main.cpp utils.cpp dictionary.cpp passphrase.cpp mastermind.cpp spacefinder.cpp testpattern.cpp
	g++ -O3 -o mastermind main.cpp utils.cpp dictionary.cpp passphrase.cpp spacefinder.cpp mastermind.cpp testpattern.cpp

run: mastermind
	./mastermind

test: test.cpp utils.cpp dictionary.cpp passphrase.cpp mastermind.cpp spacefinder.cpp testpattern.cpp
	g++ -O3 -o test test.cpp utils.cpp dictionary.cpp passphrase.cpp spacefinder.cpp mastermind.cpp testpattern.cpp

