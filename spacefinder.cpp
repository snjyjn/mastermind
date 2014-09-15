#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <assert.h>

#include "utils.h"
#include "spacefinder.h"

using namespace std;

// Constructor - not very interesting
SpaceFinder::SpaceFinder(Dictionary *d, PassPhrase *p,
			 TestPatternGenerator *tpg, GuessHistory *hist,
			 int minWordLen, int maxWordLen) {
    this->p = p;
    this->d = d;
    this->tpg = tpg;
    this->hist = hist;
    this->minWordLen = minWordLen;
    this->maxWordLen = maxWordLen;
    this->maxPhraseLength = 3*maxWordLen + 2;
    phraseLength = -1;
    dictFreq = d->getCharsByFrequency();
    debug = false;
    state = NULL;
    possible = NULL;
}

SpaceFinder::~SpaceFinder() {
    if (possible != NULL) {
	for (int i=0; i<maxPhraseLength; ++i) {
	    delete possible[i];
	}
    }
    delete possible;
    delete state;
}

void
SpaceFinder::setMaxPhraseLength(int phraseLength) {
    this->phraseLength = phraseLength;
    maxPhraseLength = phraseLength;
    state = NULL;
    possible = NULL;

    // Go to a clean state
    initialize();
}

int
SpaceFinder::findPhraseLength() {
    // There are methods to create test strings, but since the data structures
    // are not yet initialized, we do the string creation in here, get the
    // phrase length, and then initialize the structures!

    vector<int> test_group;
    vector<int> ignore_group;

    // The candidate phrase
    string phrase;

    for (int i=0; i<maxPhraseLength; ++i) {
	if (i%2 == 0) {
	    phrase.append(1,consts::spc);
	    test_group.push_back(i);
	} else {
	    phrase.append(1,consts::zpc);
	    ignore_group.push_back(i);
	}
    }

    // Additional testing sneaked in here!
    int additionalTestCounter = 0;
    string tc = tpg->getTestCombo(additionalTestCounter);
    string testChars = appendTestPhrase(tc);

    phrase.append(testChars);

    // Test the candidate phrase againt the passphrase
    int pos, chars;
    p->match(phrase, pos, chars);

    // Set the maximum phrase length.  This also initializes all the
    // data structures
    setMaxPhraseLength(chars);

    hist->push_back(new GuessHistoryElement(phrase, pos, chars));
    processMatchResponse(pos, test_group, ignore_group);

    int additionalTestCharCount = chars - 2;
    additionalTestCounter = tpg->setCharCount(additionalTestCounter, tc, additionalTestCharCount);
    return chars;
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
	    possible[i][j] = 0;
	}
    }

    // There are 3 words, of lengths len1, len2, len3 separated by 1 space
    // hence space1 = len1, space2 = len1 + 1 + len2
    // Each of len1, len2, and len3 can be [minWordLen - maxWordLen] inclusive.
    // Mark all position pairs that satisfy these conditions as possible soln.
    for (int i=0; i<maxPhraseLength; ++i) {
	int len1 = i;
	if ((len1 >= minWordLen) && (len1 <= maxWordLen)) {
	    for (int j=0; j<maxPhraseLength; ++j) {
		int len2 = j - i - 1;  // second word length
		int len3 = phraseLength - j - 1; // third word length
		assert ((len1 + len2 + len3 + 2) == maxPhraseLength);
		if ((len2 >= minWordLen) && (len2 <= maxWordLen) &&
		    (len3 >= minWordLen) && (len3 <= maxWordLen)) {
		    assert(i < j);
		    possible[i][j] = 1;
		}
	    }
	}
    }

    updateInternalState();
}

void
SpaceFinder::updateInternalState() {
    // Where state is -1 (known not possible), remove all pairs which
    // include it.
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

    // For nodes that are not in any possible pair.
    // Mark them as known not possible.
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

    // update the internal state
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
    if (coll.size() == 1) {
	// Special handling - we have 1 entry, and it is to be marked as
	// 'known'.  Not confident that the code will handle this case yet
	// so marking it unknown, but fixing 'possible'
	int known = coll[0];
	for (int i=0; i<maxPhraseLength; ++i) {
	    for (int j=0; j<maxPhraseLength; ++j) {
		if ((i != known) && (j != known)) {
		    if (possible[i][j] == 1) {
			possible[i][j] = 0;
		    }
		}
	    }
	}
    } else {
	for (vector<int>::iterator it1 = coll.begin();
	     it1 != coll.end(); ++it1) {
	    int i = *it1;
	    if (i < maxPhraseLength) {
		for (vector<int>::iterator it2 = coll.begin();
		     it2 != coll.end(); ++it2) {
		    int j = *it2;
		    if (j < maxPhraseLength) {
			if (i < j) {
			    possible[i][j] = 0;
			} else if (i > j) {
			    possible[j][i] = 0;
			}
		    }
		}
	    }
	}
    }
}

// Mark all entries in the collection as known - non spaces
void
SpaceFinder::excludeSet(vector<int> &coll) {
    for (vector<int>::iterator it1 = coll.begin(); it1 != coll.end(); ++it1) {
	int i = *it1;
	if (i < maxPhraseLength) {
	    state[i] = -1;
	}
    }
}


DictConstraints *
SpaceFinder::findSpaces(int &space1, int &space2) {
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
    // Always have half as space, and half as invalid.

    // Additional information strategy:  Move to TestPatternGenerator
    int additionalTestCounter = 1;
    DictConstraints *rc = new DictConstraints();

    // The candidate phrase
    string phrase;

    while (countUnknowns > 2) {
	// Now we can divide the unknown positions into 2 groups, one of which
	// we are testing with a space, and the other that we are ignoring
	vector<int> test_group;
	vector<int> ignore_group;

	// The candidate phrase
	phrase = buildTestString(test_group, ignore_group);

	// Additional Testing sneaked in here!
	string tc = tpg->getTestCombo(additionalTestCounter);
	string testChars = appendTestPhrase(tc);
	charCounts testCounts;
	testCounts.addToCount(testChars);

	phrase.append(testChars);

	// Test the candidate phrase againt the passphrase
	int pos, chars;
	p->match(phrase, pos, chars);

	hist->push_back(new GuessHistoryElement(phrase, pos, chars));
	processMatchResponse(pos, test_group, ignore_group);

	int additionalTestCharCount = chars - 2;
	additionalTestCounter = tpg->setCharCount(additionalTestCounter, tc, additionalTestCharCount);

	DictionaryConstraint *dc = new CharMatchWordConstraint(testCounts, additionalTestCharCount);
	rc->push_back(dc);

	if (tc.length() == dictFreq.length()) {
	    setMaxPhraseLength(chars);
	    processMatchResponse(pos, test_group, ignore_group);
	}

	if (debug) {
	    debugPhrase = buildDebugString();
	    cout << debugPhrase << consts::eol;
	    debugprint();
	    cout << "Unknown : " << countUnknowns
		 << consts::eol;
	}
    }

    getPair(space1, space2);
    if (debug) {
	tpg->debugprint();
    }

    // SJ TODO: Why does the tpg not have all the constraints, what is the
    //          value addition of the constraints from the guess history?
    //          It is important to understand that to see if we are missing
    //          something.
    // SJ TODO: The tpg getWordConstraints can be optimized to remove duplicate
    //          constraints, and optimize the cpu usage.
    // SJ TODO: tpg should be able to create PhraseConstraints as well for the
    //          filtering the phrase dictionary (before it is added)
    //          These can be optimized as well.  Need to understand if the tpg
    //          phrases have more filtering power than guess history for that
    //          case.
    //          My guess: tpg should be able to do a better job at word
    //          constraints, but not at phrase constraints.

    DictConstraints *tgpc = tpg->getWordConstraints();
    while (!tgpc->empty()) {
	DictionaryConstraint *dc = tgpc->back();
	tgpc->pop_back();
	rc->push_back(dc);
    }
    delete tgpc;
    // There is probably a lot of duplicate constraints in here!
    return rc;
}

string
SpaceFinder::appendTestPhrase(string testChars) {
    string rc;
    for (int i=0; i<maxPhraseLength; ++i) {
	rc.append(testChars);
    }
    rc.append(2, consts::spc);
    return rc;
}

// Process the response from the matcher.
// The positions were divided into 2 groups, which were combined into a single
// 'word' for matching.  At most 2 positions can match.  Process the response
// and incorporate that information in the state.
// SJ TODO: Confirm that this is equivalent to walking all pairs, and
//          identifying the ones that would return a value other than returned
//          and marking possible(tuple) as false.
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


// SJ TODO: Partition this into 2 functions, 1 of which decides which function
//          to call for the test vector, and then builds the test string
//          The other function generates a 'good enough' test string
//          The caller will call that, or the 'getBestTestVector'
//
// Use the pairs to generate a test string, so that the pair count can be
// reduced
string
SpaceFinder::buildTestString(vector<int>& test_group,
			     vector<int>& ignore_group) const {
    vector<pair<int, int> >* pairs = getPairs();
    int paircount = pairs->size();
    int group[maxPhraseLength];

    int count0 = 0;   // The number of pairs impacted if 0 is returned
    int count1 = 0;   // The number of pairs impacted if 1 is returned
    int count2 = 0;   // The number of pairs impacted if 2 is returned
    long score = 0;

    if (paircount > 8) {
	int best_score = -1; 
	int best_counter = 0;
	int positions[countUnknowns];
	test_group.clear();
	for (int i=0, j=0; i<maxPhraseLength; ++i) {
	    group[i] = 0;  // Unknown
	    if (state[i] == 0) {
		positions[j] = i;
		++j;
	    }
	}
	for (int counter = 1; counter < 1+(countUnknowns/2); ++counter) {
	    test_group.clear();
	    for (int i=0; i<countUnknowns;) {
		for (int j=0; ((j<counter) && (i<countUnknowns)); ++j,++i) {
		    group[positions[i]] = 1;  // Test
		    test_group.push_back(positions[i]);
		}
		for (int j=0; ((j<counter) && (i<countUnknowns)); ++j,++i) {
		    group[positions[i]] = 2;  // Ignore
		}
	    }
	    SpaceTestBuilder::match(test_group, *pairs, count0, count1, count2);
	    score = SpaceTestBuilder::scorefn(count0, count1, count2);
	    if (debug) {
		cout << "counter: " << counter << " "
		     << "score: " << score
		     << consts::eol;
	    }
	    if (score > best_score) {
		best_score = score; 
		best_counter = counter;
	    }
	}

	for (int i=0; i<countUnknowns;) {
	    test_group.clear();
	    for (int j=0; ((j<best_counter) && (i<countUnknowns)); ++j,++i) {
		group[positions[i]] = 1;  // Test
		test_group.push_back(positions[i]);
	    }
	    for (int j=0; ((j<best_counter) && (i<countUnknowns)); ++j,++i) {
		group[positions[i]] = 2;  // Ignore
	    }
	}
	SpaceTestBuilder::match(test_group, *pairs, count0, count1, count2);
	score = SpaceTestBuilder::scorefn(count0, count1, count2);
	if (debug) {
	    cout << "(counter: " << best_counter << ")" << " "
		 << "(score: " << score << ")" << " "
		 << "(count0: " << count0 << ")" << " "
		 << "(count1: " << count1 << ")" << " "
		 << "(count2: " << count2 << ")" << " "
		 << consts::eol;
	}
    }
    if ((paircount > 8) && (score == 0)) {
	debugprint(true);
	for (int i=0; i<maxPhraseLength; ++i) {
	    if (group[i] == 1) {
		cout << i << " ";
	    }
	}
	cout << consts::eol;
	cout << count0 << " " << count1 << " " << count2  << " " << consts::eol;
	assert(false);
    }

    if ((paircount <= 8) || (score == 0)) {
	for (int i=0; i<maxPhraseLength; ++i) {
	    group[i] = 0;  // Test
	}
	vector<int> *best = SpaceTestBuilder::getBestTestVector(*pairs);
	for (vector<int>::iterator it=best->begin(); it!=best->end(); ++it) {
	    int pos = *it;
	    group[*it] = 1;
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
	    case 1:
		phrase.append(1, consts::spc);
		test_group.push_back(i);
		break;
	    case 0:
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

vector<pair<int, int> >*
SpaceFinder::getPairs() const {
    vector<pair<int, int> >* rc = new vector<pair<int, int> >();
    for (int i=0; i<maxPhraseLength; ++i) {
	for (int j=0; j<maxPhraseLength; ++j) {
	    if (possible[i][j] == 1) {
		rc->push_back(make_pair(i,j));
	    }
	}
    }
    return rc;
}

void
SpaceFinder::printPairs() const {
    for (int i=0; i<maxPhraseLength; ++i) {
	int count = 0;
	for (int j=0; j<maxPhraseLength; ++j) {
	    if (possible[i][j] == 1) {
		count++;
		cout << " X " ;
	    } else {
		if (i < j)
		    cout << " . " ;
		else
		    cout << "   " ;
	    }
	}
	cout << consts::tab << count << consts::eol;
    }
    for (int j=0; j<maxPhraseLength; ++j) {
	int count = 0;
	for (int i=0; i<maxPhraseLength; ++i) {
	    if (possible[i][j] == 1) {
		count++;
	    }
	}
	cout << " " << count << " ";
    }
    cout << consts::eol;
}

void
SpaceFinder::debugprint(bool flag) const {
    int pairs = getPairCount();

    cout << "Valid combinations: " << pairs << consts::eol;
    if (flag || (pairs < 10)) {
	for (int i=0; i<maxPhraseLength; ++i) {
	    for (int j=0; j<maxPhraseLength; ++j) {
		if (possible[i][j] == 1) {
		    cout << "(" << i << ", " << j << ")";
		}
	    }
	}
	cout << consts::eol;
    }
    printPairs();
}
