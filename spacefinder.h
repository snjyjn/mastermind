#ifndef __SPACEFINDER_H__
#define __SPACEFINDER_H__

#include "passphrase.h"
#include "dictionary.h"
#include "testpattern.h"

using namespace std;

/*
 * Space Finder:
 * The class uses the passphrase matcher to identify spaces in there.
 * It is known that there are 2 spaces in there, which have to be identified!
 */

class SpaceFinder {
    public:
	// Constructor
	//
	SpaceFinder(Dictionary *d, PassPhrase *p, TestPatternGenerator *tpg,
		    GuessHistory *hist, int minWordLen, int maxWordLen);

	~SpaceFinder();

	// First test attempt against the matcher.  The return from the matcher
	// tells us the size of the phrase, use that to initialize various
	// structures in SpaceFinder, as well as in the TestPatternGenerator
	int findPhraseLength();

	// The main entry point to this class, used to find the spaces in the
	// phrase (and hence the word lengths).
	DictConstraints* findSpaces(int &space1, int &space2);

	void debugprint() const;

    private:
	// create a string for display purposes only
	string buildDebugString() const;

	// Build a test string based on the known possible combinations
	// of spaces, such that at least 1 combination is eliminated with
	// this.
	string buildTestString(vector<int>& test_group,
			       vector<int>& ignore_group) const;

	// Process the response from a match call.  Use the number of
	// positions that matched
	void processMatchResponse(int pos,
				 vector<int>& test_group,
				 vector<int>& ignore_group);

	// Manage internal state
	void initialize();
	void updateInternalState();

	void setMaxPhraseLength(int phraseLen);

	// Append a test phrase (additional) for low frequency characters
	string appendTestPhrase(string testChars);

	void excludeSet(vector<int> &coll);
	void foundExclusiveSet(vector<int> &coll);

	int getPairCount() const;
	void getPair(int &space1, int &space2) const;

	vector<pair<int, int> >* getPairs() const;

	void printPairs() const;

    private:
	// External State - maintained for ease of use
	PassPhrase *p;
	Dictionary *d;
	TestPatternGenerator *tpg;
	GuessHistory *hist;
	int minWordLen;
	int maxWordLen;
	int maxPhraseLength;
	int phraseLength;
	string dictFreq;
	bool debug;

// Internal State
    // For each position, we have 3 states:
    //     -1: Known not to be a space
    //      0: Unknown
    //     +1: Known to be a space
	int *state;
	int countUnknowns; // where state[i] == 0

// For each pair of positions, (i,j), where i < j, if i and j can be the 2
// positions for spaces, possible[i][j] = 1, else it is 0
	int **possible;
};
#endif
