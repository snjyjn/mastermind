#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>

#include "utils.h"
#include "passphrase.h"
#include "dictionary.h"
#include "spacefinder.h"
#include "mastermind.h"

using namespace std;


int main (int argc, char **argv) 
{
    // Get the phrases from the file
    Dictionary phrases;
    phrases.initialize(consts::phraseFileName);

    // Get the words for the dictionary from the file
    Dictionary dict;
    dict.initialize(consts::dictionaryFileName);
    if (!dict.isValid()) {
	cout << "Dictionary is not valid" << consts::eol;
    }

    Mastermind *m = new Mastermind(&dict);

    for (int i=0; i<phrases.getWordCount(); ++i) {
	const string phrase = phrases.getWord(i);
	PassPhrase *secret = new PassPhrase(phrase);
	string guess = m->guess(secret);
	cout << guess << consts::eol;
	delete secret;
    }
    analytics->printAnalysis();
    delete m;
}
