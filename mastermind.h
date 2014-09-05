#ifndef __MASTERMIND_H__
#define __MASTERMIND_H__

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>

#include "utils.h"
#include "passphrase.h"
#include "dictionary.h"
#include "spacefinder.h"

using namespace std;

// The main solver class.
class Mastermind {
    public:
	// Constructor - initialize it with a reference to the dictionary, 
	// so that it can get some basic stats from there.
	Mastermind(Dictionary *d);

	~Mastermind();

	// The main solver method.  Given a passphrase matcher, solve
	// and return the solution.
	string guess(PassPhrase *p);

	string appendTestPhrase(string word, TestPatternGenerator *tpg);

	// Find the length of the phrase,and the count of the most frequent
	// character (frequency is based on the dictionary, and not the 
	// phrase).
	// The string contains (maxPossibleLength) instances of each character
	// in the alphabet, starting with the most frequent (the others could
	// be in any order).  It also contains at least 2 spaces.
	// The matcher will respond with position matches = count of most freq
	// and character matches = length of string
	// Thanks to Roy for this wonderful idea!
	void findPhraseLength(GuessHistory &guessHistory,
	                      TestPatternGenerator *tpg);

	// Use guess history, and known information so far from the first
	// 2 words to constrain the dictionary.
	// See details in the comments in the code!
	// Build a vector of constraints, to be applied.
	DictConstraints * createDictConstraints(const string &base_guess, 
			                        const GuessHistory& hist);

    private:
	Dictionary *dict;
	string dictFreq;	// Characters in dict, sorted by count of words
				// they show up in.

	int phraseLength;
	int maxLength;
	int minLength;
	PassPhrase *p;
	bool debug;
};
#endif
