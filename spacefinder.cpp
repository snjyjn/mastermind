#include <iostream>
#include <vector>
#include <string>
#include <assert.h>

#include "utils.h"
#include "spacefinder.h"
#include "passphrase.h"
#include "dictionary.h"

using namespace std;

// Constructor - not very interesting
SpaceFinder::SpaceFinder(PassPhrase *p, Dictionary *d, int minWordLen, int maxWordLen, int phraseLen) {
    this->p = p;
    this->d = d;
    this->minWordLen = minWordLen;
    this->maxWordLen = maxWordLen;
    this->maxPhraseLength = phraseLen;
    dictFreq = d->getCharsByFrequency();
    debug = false;
}

// Initialize the space finder - prior to searching for spaces.
// Allows the same instance to be used for multiple searches - not that we 
// are doing it!
void
SpaceFinder::initialize() {
    // Create default state - all unknown
    state = new int[maxPhraseLength];
    for (int i=0; i<maxPhraseLength; ++i) {
	state[i] = 0;
    }

    possible = new pInt[maxPhraseLength];
    for (int i=0; i<maxPhraseLength; ++i) {
	possible[i] = new int[maxPhraseLength];
	for (int j=0; j<maxPhraseLength; ++j) {
	    if (i < j) {
		possible[i][j] = 1;
	    } else {
		possible[i][j] = 0;
	    }
	}
    }

    // The first space will follow the first word, which can be of length
    // [minWordLen - maxWordLen] inclusive.
    // The second space will follow the next word, which is in the same 
    // range

    for (int i=0; i<maxPhraseLength; ++i) {
	if ((i>= minWordLen) && (i<= maxWordLen)) {
	    for (int j=0; j<maxPhraseLength; ++j) {
		int len = j - i;  // second word length
		if ((len>= minWordLen) && (len<= maxWordLen)) {
		    assert(i < j);
		    possible[i][j] = 1;
		} else {
		    possible[i][j] = 0;
		}
	    }
	} else {
	    for (int j=(i+1); j<maxPhraseLength; ++j) {
		possible[i][j] = 0;
	    }
	}
    }

    // From this discussion it is clear that we need to set some elements in
    // state accordingly.
    // 0 - minWordLen is out of bounds!
    for (int i=0; i<minWordLen; ++i) {
	state[i] = -1;
    }
    // (2 * maxWordLen) + 2  to maxPhraseLength is also out of bounds
    // TODO: Test for off by 1
    for (int i=(2*(maxWordLen + 1)); i<maxPhraseLength; ++i) {
	state[i] = -1;
    }

    updateInternalState();
}

void 
SpaceFinder::updateInternalState() {
    for (int i=0; i<maxPhraseLength; ++i) {
	if (state[i] == -1) {
	    for (int j=0; j<maxPhraseLength; ++j) {
		if (i<j) {
		    possible[i][j] = 0;
		} else if (i>j) {
		    possible[j][i] = 0;
		}
	    }
	}
    }

    for (int i=0; i<maxPhraseLength; ++i) {
	if (state[i] == 0) {
	    bool flag;
	    flag = true;
	    for (int j=0; j<maxPhraseLength; ++j) {
		if ((possible[i][j] != 0) || (possible[j][i] != 0)) {
		    flag = false;
		}
	    }
	    if (flag == true) {
		state[i] = -1;
	    }
	}
    }

    countUnknowns = 0;
    for (int i=0; i<maxPhraseLength; ++i) {
	if (state[i] == 0) {
	    countUnknowns++;
	}
    }
}


// Given a collection of positions, of which only 1 can be a space
// update the possible matrix!
void 
SpaceFinder::foundExclusiveSet(vector<int> &coll) {
    for (vector<int>::iterator it1 = coll.begin();
	 it1 != coll.end(); ++it1) {
	int i = *it1;
	for (vector<int>::iterator it2 = coll.begin();
	     it2 != coll.end(); ++it2) {
	    int j = *it2;
	    if (i < j) {
		possible[i][j] = 0;
	    } else if (i > j) {
		possible[j][i] = 0;
	    }
	}
    }
}

// Mark all entries in the collection as known - non spaces
void 
SpaceFinder::excludeSet(vector<int> &coll) {
    for (vector<int>::iterator it1 = coll.begin(); it1 != coll.end(); ++it1) {
	int i = *it1;
	state[i] = -1;
    }
}


DictConstraints&
SpaceFinder::findSpaces(GuessHistory &hist, int &space1, int &space2) {
    // Setup the internal state to be clean based on all possible information
    initialize();

    string debugPhrase;
    if (debug) {
	// Compute a string that displays the current internal state.
	debugPhrase = buildDebugString();
	cout << debugPhrase << consts::eol;
	debugprint();
    }

    // Solution Strategy:
    // First make a set of binary search like passes through the positions
    // Always have half as space, and half as invalid.  The number of contiguous
    // spaces halves in each iteration till we get to 1.

    // Additional information strategy.  Append the string with low frequency
    // characters - the character match count will be 2 + count(char)
    // When this is 0, we can reduce dictionary size for words 1 & 2
    // In any case, this should help word 3 significantly!
    int additionalTestCounter = 0;
    DictConstraints *rc = new DictConstraints();

    int targetContigLength = countUnknowns/2;

    // The candidate phrase
    string phrase;

    while (targetContigLength > 0) {
	// Now we can divide the unknown positions into 2 groups, one of which 
	// we are testing with a space, and the other that we are ignoring
	vector<int> test_group;
	vector<int> ignore_group;

	// The candidate phrase
	phrase = buildTestString(targetContigLength, test_group, ignore_group);

	// Additional Testing sneaked in here!
	string testChars = appendTestPhrase(additionalTestCounter++);
	charCounts testCounts;
	testCounts.addToCount(testChars);

	phrase.append(testChars);

	// Test the candidate phrase againt the passphrase
	int pos, chars;
	p->match(phrase, pos, chars);

	hist.push_back(new GuessHistoryElement(phrase, pos, chars));
	processMatchResponse(pos, test_group, ignore_group);

	int additionalTestCharCount = chars - 2;

	DictionaryConstraint *dc = new CharMatchWordConstraint(testCounts, additionalTestCharCount);
	rc->push_back(dc);

	if (debug) {
	    debugPhrase = buildDebugString();
	    cout << debugPhrase << consts::eol;
	    debugprint();
	    cout << "Unknown : " << countUnknowns 
		 << " Target : " << targetContigLength
		 << consts::eol;
	}
	targetContigLength /= 2;
    }

    // We have now exhausted our pre-built test strings.   Initiate tests
    // that depend on the state!  The loop structure is the same as above, 
    // except that the test phrase is built differently.
    while (getPairCount() > 1) {
	vector<int> test_group;
	vector<int> ignore_group;

	// The candidate phrase
	phrase = buildTestString(test_group, ignore_group);

	// Additional Testing sneaked in here!
	string testChars = appendTestPhrase(additionalTestCounter++);
	phrase.append(testChars);

	charCounts testCounts;
	testCounts.addToCount(testChars);

	// Test the candidate phrase againt the passphrase
	int pos, chars;
	p->match(phrase, pos, chars);

	hist.push_back(new GuessHistoryElement(phrase, pos, chars));
	processMatchResponse(pos, test_group, ignore_group);

	int additionalTestCharCount = chars - 2;
	DictionaryConstraint *dc = new CharMatchWordConstraint(testCounts, additionalTestCharCount);
	rc->push_back(dc);

	// Created a phrase, tested it, applied the knowledge to internal state

	if (debug) {
	    debugPhrase = buildDebugString();
	    cout << debugPhrase << consts::eol;
	    debugprint();
	}
    }
    getPair(space1, space2);
    return *rc;
}

string 
SpaceFinder::appendTestPhrase(int counter) {
    string rc = "";
    string testChars;
    switch(counter) {
	case 0:
	    testChars = dictFreq.substr(dictFreq.length()-4, 4);
	    break;
	case 1:
	    testChars = dictFreq.substr(dictFreq.length()-7, 3);
	    break;
	case 2:
	    testChars = dictFreq.substr(dictFreq.length()-9, 2);
	    break;
	case 3:
	    testChars = dictFreq.substr(dictFreq.length()-11, 2);
	    break;
	case 4:
	    testChars = dictFreq.substr(dictFreq.length()-12, 1);
	    break;
	default:
	    testChars = dictFreq.substr(dictFreq.length()-(counter+8), 1);
	    break;
    }
    for (int i=0; i<maxPhraseLength; ++i) {
	rc.append(testChars);
    }
    return rc;
}

// Process the response from the matcher.
// The positions were divided into 2 groups, which were combined into a single
// 'word' for matching.  At most 2 positions can match.  Process the response
// and incorporate that information in the state.
void 
SpaceFinder::processMatchResponse(int pos, 
                             vector<int>& test_group, 
			     vector<int>& ignore_group) {
    // There are 3 options:
    // 0 Matches:  All the entries in the test_group are not spaces.
    // 2 Matches:  All the entries in the ignore_group are not spaces.
    // 1 Match: 1 of the entries in the test_group is a space, and 1 
    //          entry in the ignore_group is a space.
    if (pos == 0) {
	excludeSet(test_group);
    } else if (pos == 2) {
	excludeSet(ignore_group);
    } else if (pos == 1) {
	foundExclusiveSet(test_group);
	foundExclusiveSet(ignore_group);
    }
    // Propogate the changes.
    updateInternalState();
}

// Partition the unknown spaces into 2 groups - test group and ignore group
// which are about equal.  Select positions such that they are bunched in 
// contiguous groups of a given size
// For instance, if contig == 2 :  "**  **  **  **  **  "
//               if contig == 4 :  "****    ****    ****"
// Set the vectors with the grouping selected, and return a string that 
// can be sent to the matcher
string
SpaceFinder::buildTestString(int contig, 
                             vector<int>& test_group, 
			     vector<int>& ignore_group) const {
    string phrase;
    test_group.clear();
    ignore_group.clear();

    char currentChar = consts::spc;
    int currentLength = 0;
    for (int i=0; i<maxPhraseLength; ++i) {
	switch(state[i]) {
	case +1:
	case -1:
	    // We already know this value, dont test
	    phrase.append(1, consts::zpc);	
	    break;
	case 0:
	    phrase.append(1, currentChar);
	    if (currentChar == consts::spc) {
		test_group.push_back(i);
	    } else {
		ignore_group.push_back(i);
	    }
	    currentLength++;
	    if (currentLength >= contig) {
		currentChar = (currentChar == consts::spc)
				? consts::zpc 
				: consts::spc;
		currentLength = 0;
	    }
	    break;
	}
    }
    return phrase;
}

// Use the pairs to generate a test string, so that the pair count can be 
// reduced
string
SpaceFinder::buildTestString(vector<int>& test_group, 
			     vector<int>& ignore_group) const {
    // Identify the first pair - we wil ensure that these 2 entries are in 
    // the test_group.  The rest of the entries will be distributed between 
    // the 2 groups.
    int pos1, pos2;
    getPair(pos1, pos2);

    bool flag = false;
    int group[maxPhraseLength];
    for (int i=0; i<maxPhraseLength; ++i) {
	if (state[i] == 0) {
	    if ((i == pos1) || (i == pos2)) {
		group [i] = 1;
	    } else {
		group [i] = flag?1:2;
		flag = !flag;
	    }
	} else {
	    group[i] = 0;
	}
    }

    string phrase;
    test_group.clear();
    ignore_group.clear();

    for (int i=0; i<maxPhraseLength; ++i) {
	switch(state[i]) {
	case +1:
	case -1:
	    // We already know this value, dont test
	    phrase.append(1, consts::zpc);
	    break;
	case 0:
	    switch(group[i]) {
	    case 0:
		assert(false);
		break;
	    case 1:
		phrase.append(1, consts::spc);
		test_group.push_back(i);
		break;
	    case 2:
		phrase.append(1, consts::zpc);
		ignore_group.push_back(i);
		break;
	    }
	    break;
	}
    }
    return phrase;
}

string
SpaceFinder::buildDebugString() const {
    string debugPhrase;;
    for (int i=0; i<maxPhraseLength; ++i) {
	switch(state[i]) {
	case -1:
	    // This is a known non space
	    debugPhrase.append(1, 'X');
	    break;
	case +1:
	    // This is a known space
	    debugPhrase.append(1, consts::spc);
	    break;
	case 0:
	    // This is unknown, and could go either way
	    debugPhrase.append(1, consts::zpc);
	    break;
	}
    }
    return debugPhrase;
}

// Return 1 pair of positions, which could contain spaces
void 
SpaceFinder::getPair(int &space1, int &space2) const {
    int pairs = 0;
    for (int i=0; i<maxPhraseLength; ++i) {
	for (int j=0; j<maxPhraseLength; ++j) {
	    if (possible[i][j] == 1) {
		if (i < j) {
		    space1 = i; space2 = j;
		} else {
		    assert(false);
		    space1 = j; space2 = i;
		}
		return;
	    }
	}
    }
    assert(false);
}


// Count number of known pairs of spaces
int 
SpaceFinder::getPairCount() const {
    int pairs = 0;
    for (int i=0; i<maxPhraseLength; ++i) {
	for (int j=0; j<maxPhraseLength; ++j) {
	    if (possible[i][j] == 1) {
		assert(i < j);
		pairs++;
	    }
	}
    }
    return pairs;
}


void 
SpaceFinder::debugprint() const {
    int pairs = getPairCount();

    cout << "Valid combinations: " << pairs << consts::eol;
    if (pairs < 10) {
	for (int i=0; i<maxPhraseLength; ++i) {
	    for (int j=0; j<maxPhraseLength; ++j) {
		if (possible[i][j] == 1) {
		    cout << "(" << i << ", " << j << ")";
		}
	    }
	}
	cout << consts::eol;
    }
}
