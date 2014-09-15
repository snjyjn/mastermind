#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <assert.h>

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


const DictionaryEntry *
Dictionary::getEntry(int i) const {
    return (*entries)[i];
}

const DictionaryEntry&
Dictionary::operator[](int i) const {
    return *(*entries)[i];
}

string
Dictionary::getGuessWord(int strategy) const {
    switch(strategy) {
	case 0:
	    return createTestWord();
	case 1:
	    return guessWord;
	default:
	    int index = getWordCount()/2;
	    return getWord(index);
    }
}

// Create a synthetic word for the first guess.
// Strategy: For each character position identify the most frequent character
string
Dictionary::createTestWord() const {
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
	char maxChar;
	for (int j=0; j<charCounts::charArraySize; ++j) {
	    if (charFrequencyByPosition[i][j] > maxCount) {
		maxCount = charFrequencyByPosition[i][j];
		maxChar = utils::itoc(j);
	    }
	}
	rc.append(1, maxChar);
    }
    return rc;
}

typedef pair<pair<int,int> ,int> mytuple;

bool sortfn(mytuple left, mytuple right) {
    return left.second < right.second;
}

string 
Dictionary::createTestWord(const GuessHistory &gh) const {
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

    // Now we build the test word that is consistent with GuessHistory and 
    // contains highest expectation chars
    // tuple = (position, ctoi(character), count)
    vector<mytuple> highFrequencyCharPositions;
    for (int i=0; i<maxWordLength; ++i) {
	for (int j=0; j<charCounts::charArraySize; ++j) {
	    highFrequencyCharPositions.push_back(
		make_pair(make_pair(i,j), charFrequencyByPosition[i][j]));
	}
    }
    sort(highFrequencyCharPositions.begin(), highFrequencyCharPositions.end(), 
         sortfn);

    string rc;
    rc.append(maxWordLength, consts::zpc);
    int counter = 0;
    charCounts rcCounts;
    rcCounts.addToCount(rc);
    rcCounts[utils::ctoi(consts::zpc)] = 0;
    while (!highFrequencyCharPositions.empty()) {
	 mytuple t = highFrequencyCharPositions.back();
	 highFrequencyCharPositions.pop_back();
	 int pos = t.first.first;
	 char c = utils::itoc(t.first.second);
	 if (rc[pos] == consts::zpc) {
	     // Add the character to the guess
	     counter++;
	     rc[pos] = c;
	     rcCounts[utils::ctoi(c)]++;

	    // check for consistency with guessHistory
	    bool match = true;
	    for (int l=0; (match && l<gh.size()); ++l) {
		GuessHistoryElement *g = gh[l];
		if (rcCounts.match(g->counts) > g->chars) {
		    match = false;
		}
	    }
	    if (match == false) {
		// Not consistent, remove the character
		counter--;
		rc[pos] = consts::zpc;
		rcCounts[utils::ctoi(c)]--;
	    }
	}
	if (counter == maxWordLength) {
	    return rc;
	}
    }
    cout << rc;
    assert(false);
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
Dictionary::getSubDictionary(const DictionaryConstraint &dc,
                             bool debugThis) const {
    Dictionary *sub = new Dictionary();
    for (vector<DictionaryEntry *>::iterator it=entries->begin();
         it!= entries->end(); ++it) {
	DictionaryEntry *de = (*it);
	if (dc.match(de)) {
	    sub->addEntry(de);
	} else if (debugThis) {
	    dc.explain(de);
	}
    }
    return sub;
}

//------------------------------------------------------------------------------
//
//		DictionaryEntry
//
//------------------------------------------------------------------------------

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

int
DictionaryEntry::positionMatch(const string word) const {
    int m = 0;
    string entry_word = getWord();
    for (int i=0; i<word.length() && i<entry_word.length(); ++i) {
	if (word[i] == entry_word[i]) {
	    m++;
	}
    }
    return m;
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

//------------------------------------------------------------------------------
//
//		DictionaryConstraint
//
//------------------------------------------------------------------------------

DictionaryConstraint::~DictionaryConstraint() {
}

//------------------------------------------------------------------------------
//
//		DictConstraints
//
//------------------------------------------------------------------------------

DictConstraints::DictConstraints()
    :vector<DictionaryConstraint *>()
{
}

DictConstraints::~DictConstraints() {
    clear();
}

bool
DictConstraints::match(const DictionaryEntry *de) const {
    bool flag = true;
    for (DictConstraints::const_iterator dc_it=cbegin();
	 (dc_it!= cend() && (flag == true)); ++dc_it) {
	const DictionaryConstraint *dc = (*dc_it);

	// check the constraint
	if (false == dc->match(de)) {
	    return false;
	}
    }
    return true;
}

void
DictConstraints::explain(const DictionaryEntry *de) const {
    cout << "DictConstraints:: " << consts::eol;
    for (DictConstraints::const_iterator dc_it=cbegin();
	 dc_it!= cend(); ++dc_it) {
	const DictionaryConstraint *dc = (*dc_it);
	// check the constraint
	if (false == dc->match(de)) {
	    dc->explain(de);
	}
    }
}

void
DictConstraints::debugprint() const {
    cout << "DictConstraints:: "
         << this->size() << " entries"
         << consts::eol;
    for (DictConstraints::const_iterator dc_it=cbegin();
	 (dc_it!= cend()); ++dc_it) {
	const DictionaryConstraint *dc = (*dc_it);
	dc->debugprint();
    }
    cout << "DictConstraints:: done" << consts::eol;
}

void
DictConstraints::clear() {
    while (!empty()) {
	DictionaryConstraint *dc = back();
	pop_back();
	delete dc;
    }
    resize(0);
}


//------------------------------------------------------------------------------
//
//		DictSizeConstraint
//
//------------------------------------------------------------------------------

DictSizeConstraint::DictSizeConstraint(int size) {
    this->size = size;
}

DictSizeConstraint::~DictSizeConstraint() {
}

bool
DictSizeConstraint::match(const DictionaryEntry *de) const {
    int wordSize = de->getWord().length();
    return (wordSize == size);
}

void
DictSizeConstraint::explain(const DictionaryEntry *de) const {
    int wordSize = de->getWord().length();
    debugprint();
    cout << "size : " << size << " compared with dictionary word size " << wordSize << consts::eol;
}

void
DictSizeConstraint::debugprint() const {
    cout << "DictSizeConstraint : " << size << " wordsize " << consts::eol;
}

//------------------------------------------------------------------------------
//
//		PosMatchConstraint
//
//------------------------------------------------------------------------------

PosMatchConstraint::PosMatchConstraint(string word, int matchCount) {
    this->word = word;
    this->posMatchCount = matchCount;
    // if the entire word matches, do not filter it out
    // (i.e. number of characters that match is equal to the word length)
    //                       else, filter out the word
    allowSelfMatch = (word.length() == matchCount);
}

PosMatchConstraint::~PosMatchConstraint() {
}

bool PosMatchConstraint::match(const DictionaryEntry *de) const {
    if (allowSelfMatch) {
	// SJ TODO: This is a bogus case, we have a complete match, and
	//          we should not be creating a subdictionary of size 1
	if (de->getWord() == word)
	    return true;
	else
	    return false;
    }
    int m = de->positionMatch(word);
    return (m == posMatchCount);
}

void PosMatchConstraint::explain(const DictionaryEntry *de) const {
    int matchCount = de->positionMatch(word);
    debugprint();
    cout << "matched : " << matchCount << " with " << de->getWord() 
         << consts::eol;
}

void PosMatchConstraint::debugprint() const {
    cout << "PosMatchConstraint : " << posMatchCount << " of " << word 
         << consts::eol;
}

//------------------------------------------------------------------------------
//
//		CharMatchConstraint
//
//------------------------------------------------------------------------------

CharMatchConstraint::
CharMatchConstraint(charCounts candidateCounts, int matchCount) {
    counts = candidateCounts;
    charMatchCount = matchCount;
}

CharMatchConstraint::~CharMatchConstraint() {
}

bool
CharMatchConstraint::match(const DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    return (matchCount == charMatchCount);
}

void
CharMatchConstraint::explain(const DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    debugprint();
    cout << "matched : " << matchCount << " with " << de->getWord() 
         << consts::eol;
}

void
CharMatchConstraint::debugprint() const {
    cout << "CharMatchConstraint : " << charMatchCount << " of ";
    counts.debugprint();
}

//------------------------------------------------------------------------------
//
//		CharMatchWordConstraint
//
//------------------------------------------------------------------------------

CharMatchWordConstraint::
CharMatchWordConstraint(charCounts candidateCounts, int matchCount)
: CharMatchConstraint(candidateCounts, matchCount) {
}

bool
CharMatchWordConstraint::match(const DictionaryEntry *de) const {
    int matchCount = de->characterMatch(counts);
    return (matchCount <= charMatchCount);
}

void
CharMatchWordConstraint::debugprint() const {
    cout << "CharMatchWordConstraint : " << charMatchCount << " of ";
    counts.debugprint();
}

//------------------------------------------------------------------------------
//
//		MasterMindConstraint
//
//------------------------------------------------------------------------------

MasterMindConstraint::MasterMindConstraint(string word, int chars, int pos) {
    this->word = word;
    this->charMatchCount = chars;
    this->posMatchCount = pos;
    this->wlen = word.length();
    counts.addToCount(word);
    // SJ TODO: See comment about allowSelfMatch elsewhere in the code!
    if ((wlen == chars) && (wlen == pos)) {
	allowSelfMatch = true;
    } else {
	allowSelfMatch = false;
    }
}

MasterMindConstraint::~MasterMindConstraint() {
}


bool
MasterMindConstraint::match(const DictionaryEntry *de) const {
    bool rc = false;
    string entry_word = de->getWord();

    if (allowSelfMatch) {
	if (entry_word == word) {
	    return true;
	}
    }
    if ((wlen == entry_word.length()) &&
        (de->characterMatch(counts) == charMatchCount) &&
        (de->positionMatch(word) == posMatchCount)) {
	rc = true;
    }
    if ((rc == true) && (allowSelfMatch == false)) {
	if (entry_word == word) {
	    return false;
	}
    }
    return rc;
}

void
MasterMindConstraint::explain(const DictionaryEntry *de) const {
    debugprint();
    cout << "Matching : " << de->getWord() << consts::eol;
}

void
MasterMindConstraint::debugprint() const {
    cout << "MasterMindConstraint : " << word << " (" << charMatchCount
         << "character matches of " << wlen << ") and ("
	 << posMatchCount << " position matches)" << consts::eol;
}




//------------------------------------------------------------------------------
//
//		DictUtils
//
//------------------------------------------------------------------------------

void
DictUtils::getDictSizeDistribution(const Dictionary *d, pInt &counts) {
    for (int i=0; i<d->getWordCount(); ++i) {
	int len = (*d)[i].length();
	counts[len]++;
    }
}

charCounts
DictUtils::getDictCharDistribution(const Dictionary *d) {
    charCounts rc;
    for (int i=0; i<d->getWordCount(); ++i) {
	const DictionaryEntry *de = d->getEntry(i);
	de->collateStats(rc);
    }
    return rc;
}
