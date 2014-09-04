
all: mastermind

clean:
	rm -f mastermind 

mastermind: utils.cpp dictionary.cpp passphrase.cpp mastermind.cpp spacefinder.cpp
	g++ -O -o mastermind utils.cpp dictionary.cpp passphrase.cpp spacefinder.cpp mastermind.cpp

run: mastermind
	./mastermind
