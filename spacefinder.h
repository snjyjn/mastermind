#ifndef __SPACEFINDER_H__
#define __SPACEFINDER_H__

#include <iostream>
#include <algorithm>
#include <vector>

#include "utils.h"
#include "passphrase.h"
#include "dictionary.h"

using namespace std;

/*
 * Space Finder:
 * The class uses the passphrase matcher to identify spaces in there.
 * It is known that there are 2 spaces in there, which have to be identified!
 */

// Only dictionary needs to access dictionary entries!  Use friend!
class SpaceFinder {
    public:
	// Constructor
	SpaceFinder(PassPhrase *p, Dictionary *d, int minWordLen, 
	            int maxWordLen, int phraseLen);

	// The main entry point to this class
	DictConstraints& findSpaces(GuessHistory &hist, int &space1, int &space2);

	void debugprint() const;

    private:
	// create a string for display purposes only
	string buildDebugString() const;

	// Append a test phrase (additional) for low frequency characters
	string appendTestPhrase(int counter);

	// Build a string to test for spaces.  Test only unknown positions
	// Half of which should be in test group, and hald in ignore.
	// n contiguous positions (defined by contig) should be in the same
	// group.
	string buildTestString(int contig,
                               vector<int>& test_group,
			       vector<int>& ignore_group) const;

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

	void excludeSet(vector<int> &coll);
	void foundExclusiveSet(vector<int> &coll);

	int getPairCount() const;
	void getPair(int &space1, int &space2) const;

	// External State - maintained for ease of use
	PassPhrase *p;
	Dictionary *d;
	int minWordLen;
	int maxWordLen;
	int maxPhraseLength;
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

class TestPatternGenerator {
public:
    TestPatternGenerator(Dictionary *d);

    // Get a test pattern, to be used when there are no other alpha 
    // chars in the test string.  
    // id 0: Returns a string with all 26 alphabets, repeated max times
    //       with the first max characters being the most frequent char
    // This returns the size of the phrase.
    // id 1-4: return a string with low freqeuncy character combinations,
    //       which can be used to reduce the dictionary size.
    const string& getPattern(int id);

private:

};
#endif
