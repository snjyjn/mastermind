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

const int charCounts::charArraySize = 28;  // 26 alpha + space + zpc

charCounts::charCounts() {
    reset();
}

charCounts::charCounts(const charCounts &c) {
    for (int i=0; i<charArraySize; ++i) {
	counts[i] = c.counts[i];
    }
}

int &
charCounts::operator[](int i) {
    assert(i<charArraySize);
    return counts[i];
}

const int &
charCounts::operator[](int i) const {
    assert(i<charArraySize);
    return counts[i];
}

void
charCounts::reset() {
    for (int i=0; i<charArraySize; ++i) {
	counts[i] = 0;
    }
}

void 
charCounts::addToCount(const string &word) {
    for (string::const_iterator it = word.cbegin(); it != word.cend(); ++it) {
	counts[utils::ctoi(*it)] += 1;
    }
}

void 
charCounts::addToCount(const charCounts &other) {
    for (int i=0; i<charArraySize; ++i) {
	counts[i] += counts[i];
    }
}

int
charCounts::match(const charCounts& other) const {
    int match = 0;
    for (int i=0; i<charArraySize; ++i) {
	match += min(counts[i], other.counts[i]);
    }
    return match;
}

int
charCounts::uniqCounts() const {
    int uniq = 0;
    for (int i=0; i<charArraySize; ++i)
	if (counts[i] > 0) 
	    uniq++;
    return uniq;
}

string 
charCounts::getCharsByFrequency() const {
    vector<pair<int,char> > wordFreq;
    for(int i=0; i<charArraySize; ++i) {
	if (counts[i] > 0) {
	    wordFreq.push_back(make_pair(counts[i], utils::itoc(i)));
	}
    }
    sort(wordFreq.begin(), wordFreq.end(), greater<pair<int,char> >());

    string rc;
    for(vector<pair<int,char> >::iterator it = wordFreq.begin(); 
        it != wordFreq.end(); ++it) {
	rc.append(1, it->second);
    }
    return rc;
}

void 
charCounts::debugprint() const {
   for (int i=0; i<charArraySize; ++i) {
       if (counts[i] > 0) {
	   cout << "(" << utils::itoc(i) << " : " << counts[i] << ")";
	   cout << consts::spc;
       }
   }
   cout << consts::eol;
}

char 
utils::itoc(int i) {
    if (i == 26) {
	return consts::spc;
    } else if (i == 27) {
	return consts::zpc;
    } else {
	return (char)('a' + i);
    }
}

int 
utils::ctoi(char c) {
    if (c == consts::spc) {
	return 26;
    } else if (c == consts::zpc) {
	return 27;
    } else {
	return (int)(c - 'a');
    }
}

GuessHistoryElement::GuessHistoryElement(const string word, int pos, int chars) {
    this->word = word;
    this->pos = pos;
    this->chars = chars;
    counts.addToCount(word);
}

bool 
GuessHistoryElement::phraseMatch(const string &candidate) const {
    charCounts candidateCounts;
    candidateCounts.addToCount(candidate);
    return phraseMatch(candidateCounts, candidate);
}

bool 
GuessHistoryElement::phraseMatch(const charCounts &candidateCounts, 
			         const string &candidate) const {
    int characterMatches = counts.match(candidateCounts);
    if (characterMatches != chars) {
	return false;
    }
    int positions = 0;
    string::const_iterator c = candidate.begin();
    string::const_iterator w = word.begin();
    while (c != candidate.end() && w != word.end()) {
	if (*c == *w) {
	    positions++;
	}
	++c; ++w;
    }
    if (positions != pos) {
	return false;
    }
    return true;
}

bool 
GuessHistoryElement::subPhraseMatch(const charCounts &candidateCounts) const {
    int characterMatches = counts.match(candidateCounts);
    return (characterMatches <= chars);
}


// For analysis purposes:

GuessAnalytics::GuessAnalytics() {
    attempts = new vector<int>(9, 0);
    state = 0;
    for (int i=0; i<4; ++i) {
	for (int j=0; j<50; ++j) {
	    dictSizes[i][j] = 0;
	    dictSizeCount[i][j] = 0;
	}
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
GuessAnalytics::addDictSize(int wordNumber, int attempt, int size) {
    assert(wordNumber < 5);
    assert(attempt < 50);
    dictSizes[wordNumber][attempt] += size;
    dictSizeCount[wordNumber][attempt]++;
}

void 
GuessAnalytics::printAnalysis() {
    string  names[] = {
	"Length", 
	"Spaces",
	"Word 1",
	"Word 2",
	"Word 3",
	"Phrase",
	"Confirm"
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
    for (int i=0; i<5; ++i) {
	cout << "word " << i;
	for (int j=0; j<50; ++j) {
	    if (dictSizeCount[i][j] > 0) {
		cout << consts::tab    // << j << ": " 
		     << (dictSizes[i][j]/dictSizeCount[i][j]);
	    }
	}
	cout << consts::eol;
    }
}

// Global!
GuessAnalytics *analytics = new GuessAnalytics();
