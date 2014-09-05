#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>

#include "utils.h"
#include "dictionary.h"

using namespace std;

// Constructor
Dictionary::Dictionary() {
    entries = new vector<DictionaryEntry *>;
    allotted = new vector<DictionaryEntry *>;
    maxWordLength = 0;
    minWordLength = 0;
    maxUniqChars = 0;
    debug = false;
}

Dictionary::~Dictionary() {
    deleteEntries();
    entries->resize(0);
    allotted->resize(0);
    delete entries;
    delete allotted;
}

void
Dictionary::initialize(string filename) {
    ifstream dict(filename);
    if (dict.is_open()) {
	string word;
	while (getline(dict, word)) {
	    addWord(word);
	}
    }
    dict.close();
}

void
Dictionary::deleteEntries() {
    while (!allotted->empty()) {
	DictionaryEntry *de = allotted->back();
	allotted->pop_back();
	delete de;
    }
    entries->resize(0);
}

bool 
Dictionary::isValid() const {
    for (int i=0; i<getWordCount(); ++i) {
	if (!validateWord(getWord(i))) {
	    return false;
	}
    }
    return true;
}

int 
Dictionary::getMaxWordLength() const {
    return  maxWordLength;
}

int 
Dictionary::getMinWordLength() const {
    return  minWordLength;
}

void
Dictionary::addWord(string word) {
    DictionaryEntry *e = new DictionaryEntry(word);
    addEntry(e);
    allotted->push_back(e);
}

void
Dictionary::addEntry(DictionaryEntry *e) {
    entries->push_back(e);
    if (maxWordLength < e->length()) {
	maxWordLength = e->length();
    }
    if ((minWordLength > e->length()) || (minWordLength == 0)) {
	minWordLength = e->length();
    }
    if (maxUniqChars < e->uniqueCharacterCount()) {
	maxUniqChars = e->uniqueCharacterCount();
	guessWord = e->getWord();
    }
    e->collateStats(freq);
}

bool
Dictionary::validateWord(string word) {
    for (string::iterator it=word.begin(); it!= word.end(); ++it) {
	if (isalpha(*it)) {
	} else {
	    cout << "1:@" <<  word << "@ " << word.length() << consts::eol;
	    return false;
	}
    }
    return true;
}

int 
Dictionary::getWordCount() const {
    return entries->size();
}

const string &
Dictionary::getWord(int i) const {
    return (*entries)[i]->getWord();
}

string 
Dictionary::getGuessWord(int strategy) const {
    switch(strategy) {
	case 0:
	    return createTestWord(0);
	case 1:
	    return guessWord;
	default:
	    int index = getWordCount()/2;
	    return getWord(index);
    }
}

// Create a synthetic word for the first guess.  
// Strategy 0: For each character position identify the most frequent character
// Strategy 1: For each character position identify the least frequent character
// and use that to build a word!  string
// Strategy 1 sucks!
string
Dictionary::createTestWord(int strategy) const {
    // Initialize data structures
    int charFrequencyByPosition[maxWordLength][charCounts::charArraySize];
    for (int i=0; i<maxWordLength; ++i) {
	for (int j=0; j<charCounts::charArraySize; ++j) {
	    charFrequencyByPosition[i][j] = 0;
	}
    }

    // Iterate over all entries, and all positions, and count!
    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it) {
	const string word = (*it)->getWord();
	for (int i=0; i<word.length(); ++i) {
	    int j = utils::ctoi(word[i]);
	    charFrequencyByPosition[i][j]++;
	}
    }
    string rc;

    // For each position, get the max count, and associated character.
    // Use that to build the test word.
    int dictsize = getWordCount();
    for (int i=0; i<maxWordLength; ++i) {
	int maxCount=0; 
	int minCount=dictsize; 
	char maxChar;
	char minChar;
	for (int j=0; j<charCounts::charArraySize; ++j) {
	    if (charFrequencyByPosition[i][j] > maxCount) {
		maxCount = charFrequencyByPosition[i][j];
		maxChar = utils::itoc(j);
	    }
	    if (charFrequencyByPosition[i][j] < minCount) {
		minCount = charFrequencyByPosition[i][j];
		minChar = utils::itoc(j);
	    }
	}
	switch(strategy) {
	    case 0:
		rc.append(1, maxChar);
		break;
	    case 1:
		rc.append(1, minChar);
		break;
	}
    }
    return rc;
}

string 
Dictionary::getCharsByFrequency() const {
    string rc = freq.getCharsByFrequency();
    if (debug) {
	cout << "Dict Counts" << consts::eol;
	for (int j=0; j<rc.length(); ++j) {
	    char c = rc[j];
	    int i = utils::ctoi(c);
	    int count = freq[i];
	    cout << c << ": " << count << consts::eol;
	}
    }
    return rc;
}

void
Dictionary::debugprint() const {
    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it)
	(*it)->debugprint();
}

Dictionary *
Dictionary::getSubDictionary(int size) {
    Dictionary *sub = new Dictionary();
    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it) {
	if ((*it)->length() == size) {
	    DictionaryEntry *e = (*it);
	    sub->addEntry(e);
	}
    }
    return sub;
}

Dictionary *
Dictionary::getSubDictionary(string word, int positionMatches) const {
    Dictionary *sub = new Dictionary();
    DictionaryEntry *self = NULL;
    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it) {
	DictionaryEntry *de = (*it);
	string entry_word = de->getWord();
	int m = 0;
	for (int i=0; i<word.length() && i<entry_word.length(); ++i) {
	    if (word[i] == entry_word[i]) {
		m++;
	    }
	}
	if (m == positionMatches) {
	    if (word == entry_word) {
		self = de;
	    } else {
		sub->addEntry(de);
	    }
	}
    }
    if (sub->getWordCount() == 0) {
	sub->addEntry(self);
    }
    return sub;
}


Dictionary *
Dictionary::getSubDictionary(string word, int pos, int chars) const {
    Dictionary *sub = new Dictionary();
    DictionaryEntry *self = NULL;
    int wlen = word.length();
    charCounts cc; cc.addToCount(word);

    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it) {
	DictionaryEntry *de = (*it);
	string entry_word = de->getWord();
	if ((de->characterMatch(cc) == chars) &&
	    (wlen == entry_word.length())) {
	    int m = 0;
	    for (int i=0; i<wlen; ++i) {
		if (word[i] == entry_word[i]) {
		    m++;
		}
	    }
	    if (m == pos) {
		if (word == entry_word) {
		    self = de;
		} else {
		    sub->addEntry(de);
		}
	    }
	}
    }
    if (sub->getWordCount() == 0) {
	sub->addEntry(self);
    }
    return sub;
}

// Apply a collection of constraints to a dictionary to get a subdict.
Dictionary *
Dictionary::getSubDictionary(const DictConstraints &constr, 
                             bool debugThisCall) const {
    Dictionary *sub = new Dictionary();

    // For each dictionary  entry
    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it) {
	DictionaryEntry *de = (*it);

	// for each constraint specified
	bool flag = true;
	for (DictConstraints::const_iterator dc_it=constr.cbegin(); 
	     (dc_it!= constr.cend() && (flag == true)); ++dc_it) {
	    const DictionaryConstraint *dc = (*dc_it);

	    // check the constraint
	    if (false == dc->match(de)) {
		flag = false;
		if (debugThisCall) {
		    dc->explain(de);
		}
	    }
	}

	// if any constraint fails, do not add the entry to the subdirectory
	if (flag) {
	    sub->addEntry(de);
	}
    }
    return sub;
}

DictionaryEntry::DictionaryEntry(string word) {
    this->word = word;
    this->len = word.length();
    freq.addToCount(word);
    uniqChars = freq.uniqCounts();
}

DictionaryEntry::~DictionaryEntry() {
}

int 
DictionaryEntry::characterMatch(const DictionaryEntry* de) const {
    return freq.match(de->freq);
}

int 
DictionaryEntry::characterMatch(const charCounts& cc) const {
    return freq.match(cc);
}

void
DictionaryEntry::collateStats(charCounts& frequency) const {
    for (int i=0; i<charCounts::charArraySize; ++i) {
	if (freq[i] > 0) {
	    frequency[i]++;
	}
    }
}

const string &
DictionaryEntry::getWord() const {
    return this->word;
}

int
DictionaryEntry::length() const {
    return this->len;
}

int
DictionaryEntry::uniqueCharacterCount() const {
    return this->uniqChars;
}

void
DictionaryEntry::debugprint() const {
    cout << word << consts::eol;
}

void
DictUtils::clearDictConstraints(DictConstraints& constraints) {
    while (!constraints.empty()) {
	DictionaryConstraint *dc = constraints.back();
	constraints.pop_back();
	delete dc;
    }
    constraints.resize(0);
}

DictionaryConstraint::~DictionaryConstraint() {
}

CharMatchConstraint::
CharMatchConstraint(charCounts candidateCounts, int matchCount) {
    counts = candidateCounts;
    charMatchCount = matchCount;
}

CharMatchConstraint::~CharMatchConstraint() {
}

bool 
CharMatchConstraint::match(DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    return (matchCount == charMatchCount);
}

void 
CharMatchConstraint::explain(DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    debugprint();
    cout << "matched : " << matchCount << " with " << de->getWord() << consts::eol;
}

void 
CharMatchConstraint::debugprint() const {
    cout << "CharMatchConstraint : " << charMatchCount << " of ";
    counts.debugprint();
}

CharMatchWordConstraint::
CharMatchWordConstraint(charCounts candidateCounts, int matchCount) {
    counts = candidateCounts;
    charMatchCount = matchCount;
}

CharMatchWordConstraint::~CharMatchWordConstraint() {
}

bool 
CharMatchWordConstraint::match(DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    return (matchCount <= charMatchCount);
}

void 
CharMatchWordConstraint::explain(DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    debugprint();
    cout << "matched : " << matchCount << " with " << de->getWord() << consts::eol;
}

void 
CharMatchWordConstraint::debugprint() const {
    cout << "CharMatchWordConstraint : " << charMatchCount << " of ";
    counts.debugprint();
}
