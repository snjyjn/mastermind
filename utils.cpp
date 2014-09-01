#include <iostream>
#include <assert.h>

#include "utils.h"

using namespace std;


// Contains classes
//    consts
//    utils
//    GuessAnalytics

const char consts::eol = '\n';
const char consts::tab = '\t';
const char consts::spc = ' ';
const char consts::zpc = '.';
const string consts::dictionaryFileName = "dictionary.txt";
const string consts::phraseFileName = "phrases.txt";

void 
utils::initCounts(int *count) {
    for (int i=0; i<27; ++i) {
	count[i] = 0;
    }
}

void 
utils::countCharacters(const string& word, int *count) {
    for (string::const_iterator it = word.cbegin(); it != word.cend(); ++it) {
	count[utils::ctoi(*it)] += 1;
    }
}

char 
utils::itoc(int i) {
    if (i == 26) {
	return consts::spc;
    } else {
	return (char)('a' + i);
    }
}

int 
utils::ctoi(char c) {
    if (c == consts::spc) {
	return 26;
    } else {
	return (int)(c - 'a');
    }
}


// For analysis purposes:

GuessAnalytics::GuessAnalytics() {
    attempts = new vector<int>(9, 0);
    state = 0;
    for (int i=0; i<3; ++i) {
	dictSizes[i] = 0;
    }
    count = 0;
}

void 
GuessAnalytics::setState(int s) {
    state = s;
    if (s == PHRASETEST) {
	count++;
    }
}

void 
GuessAnalytics::addAttempt() {
    (*attempts)[state]++;
}

void 
GuessAnalytics::addDictSize(int wordNumber, int size) {
    assert(wordNumber <3);
    dictSizes[wordNumber] += size;
}

void 
GuessAnalytics::printAnalysis() {
    string  names[] = {
	"Unknown", 
	"Blanks",
	"Word 1 chars",
	"Word 1 words",
	"Word 2 chars",
	"Word 2 words",
	"Word 3 chars",
	"Word 3 words",
	"Phrase Confirm"
    };
    int total = 0;
    for (int i=0; i<attempts->size(); ++i) {
       if ((*attempts)[i] > 0) {
	   cout << names[i] << " : " << (*attempts)[i] << consts::eol;
	   if (i != PHRASETEST)
	       total += (*attempts)[i];
	}
    }
    cout << "TOTAL " << " : " << total << consts::eol;
    cout << consts::eol;
    for (int i=0; i<3; ++i) {
	cout << "Dict for word " << (i+1) << " : " << (dictSizes[i]/count) << consts::eol;
    }
}

// Global!
GuessAnalytics *analytics = new GuessAnalytics();
