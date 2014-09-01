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
    maxWordLength = 0;
    minWordLength = 0;
    maxUniqChars = 0;
    for (int i=0; i<27; ++i) {
	counts[i] = 0;
    }
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
    e->collateStats(counts);
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

bool sortByCount(const int& lhs, const int& rhs) {
    return (lhs > rhs);
}

void
Dictionary::getStats(int frequency[]) const {
    for (int i=0; i<27; ++i) {
	frequency[i] = counts[i];
    }
}

const string &
Dictionary::getWord(int i) const {
    return (*entries)[i]->getWord();
}

const string &
Dictionary::getGuessWord() const {
    return guessWord;
}

// Create a synthetic word for the first guess.  For each characterposition
// identify the most frequent character, and use that to build a word!
string
Dictionary::createTestWord() const {
    // Initialize data structures
    int charFrequencyByPosition[maxWordLength][27];
    for (int i=0; i<maxWordLength; ++i) {
	for (int j=0; j<27; ++j) {
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

    // For each position, get the max count, and associated character.
    // Use that to build the test word.
    string rc;
    for (int i=0; i<maxWordLength; ++i) {
	int maxCount=0; 
	char maxChar;
	for (int j=0; j<27; ++j) {
	    if (charFrequencyByPosition[i][j] > maxCount) {
		maxCount= charFrequencyByPosition[i][j];
		maxChar = utils::itoc(j);
	    }
	}
	rc.append(1, maxChar);
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
         it!= entries->end(); ++it)
	if ((*it)->length() == size) {
	    DictionaryEntry *e = (*it);
	    sub->addEntry(e);
	}
    return sub;
}

Dictionary *
Dictionary::getSubDictionary(string word, int positionMatches) const {
    Dictionary *sub = new Dictionary();
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
	    sub->addEntry(de);
	}
    }
    return sub;
}

// Apply a collection of constraints to a dictionary to get a subdict.
Dictionary *
Dictionary::getSubDictionary(vector<DictionaryConstraint *> constr) const {
    Dictionary *sub = new Dictionary();

    // For each dictionary  entry
    for (vector<DictionaryEntry *>::iterator it=entries->begin(); 
         it!= entries->end(); ++it) {
	DictionaryEntry *de = (*it);

	// for each constraint specified
	bool flag = true;
	for (vector<DictionaryConstraint *>::iterator dc_it=constr.begin(); 
	     (dc_it!= constr.end() && (flag == true)); ++dc_it) {
	    DictionaryConstraint *dc = (*dc_it);

	    // check the constraint
	    if (false == dc->match(de)) {
		flag = false;
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
    for (int i=0; i<27; ++i) { counts[i] = 0; }
    for (string::const_iterator it = word.cbegin(); it != word.cend(); ++it) {
	counts[utils::ctoi(*it)] += 1;
    }
    uniqChars = 0;
    for (int i=0; i<27; ++i) { 
	if (counts[i] > 0) {
	    uniqChars++;
	}
    }
}

int 
DictionaryEntry::characterMatch(const DictionaryEntry* de) const {
    return characterMatch(de->counts);
}

int 
DictionaryEntry::characterMatch(const int counts[]) const {
    int rc = 0;
    for (int i=0; i<27; ++i) {
	rc += min(counts[i], this->counts[i]);
    }
    return rc;
}

void
DictionaryEntry::collateStats(int frequency[]) const {
    for (int i=0; i<27; ++i) {
	if (counts[i] > 0) {
	    frequency[i] += 1;
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

DictionaryEntry::~DictionaryEntry() {
    assert(false);
}

CharMatchConstraint::CharMatchConstraint(int charCounts[], int matchCount) {
    utils::initCounts(counts);
    for (int i=0; i<27; ++i) {
	this->counts[i] = charCounts[i];
    }
    this->charMatchCount = matchCount;
}

bool 
CharMatchConstraint::match(DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    return (matchCount == charMatchCount);
}
