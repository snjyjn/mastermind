
all: mastermind

clean:
	rm -f mastermind 

mastermind: main.cpp utils.cpp dictionary.cpp passphrase.cpp mastermind.cpp spacefinder.cpp
	g++ -O -o mastermind main.cpp utils.cpp dictionary.cpp passphrase.cpp spacefinder.cpp mastermind.cpp

run: mastermind
	./mastermind

test: test.cpp utils.cpp dictionary.cpp passphrase.cpp mastermind.cpp spacefinder.cpp
	g++ -O -o test test.cpp utils.cpp dictionary.cpp passphrase.cpp spacefinder.cpp mastermind.cpp

