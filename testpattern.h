#ifndef __TESTPATTERN_H__
#define __TESTPATTERN_H__

#include <unordered_set>

#include "passphrase.h"
#include "dictionary.h"

using namespace std;

class TestPatternGenerator {
    public:
	TestPatternGenerator(Dictionary *d);

	~TestPatternGenerator();

	// Get a test pattern, to be used when there are no other alpha
	// chars in the test string.
	// id 0: Returns a string with all 26 alphabets, repeated max times
	//       with the first max characters being the most frequent char
	// This returns the size of the phrase.
	// id 1-4: return a string with low freqeuncy character combinations,
	//       which can be used to reduce the dictionary size.
	const string& getPattern(int id);

        int setCharCount(int counter, string combo, int count);

	string getTestCombo(int counter);

	string getNextTestCombo();

	DictConstraints* getWordConstraints() const;

	void debugprint() const;
    private:
	// Initialize the data structures.  Call this before calling getPattern
	void initialize();

	Dictionary *d;
	string dictFreq;
	int phraseLength;		// phrase length, including spaces

	string testCases[8];             // Test Cases computed in advance

	// State
	// Keep a track of all testing done so far
	vector<pair<string, int> > alphaCounts;

	charCounts zeroCountChars;

	int lastCounter;   // Used for nextCombo!
	bool debug;
};


class SpaceTestBuilder {
    public:
	// Convert an unordered set of nodes into a vector of nodes
	// Useful for various methods used in test string creation, including
	// this class
	static vector<int>* getTestVector(const unordered_set<int> &node_set,
	                                  int seq);

	// Given a collection of pairs, and a collection of nodes, return
	// the number of pairs which would return 0, 1 or 2.
	// count0 is the number of pairs that would match the test with count 0
	// count1 is the number of pairs that would match the test with count 1
	// count2 is the number of pairs that would match the test with count 2
	static void match(const vector<int> &nodes,
			  const vector<pair<int, int> > &pairs,
			  int& count0, int& count1, int& count2);

	// Score computation for match counts
	// This method is used to score the quality of a test vector for its
	// ability to reduce the number of possible position pairs.
	// 0 is very poor, and larger is better!
	static long long scorefn(int count0, int count1, int count2);

	// Given a collection of pairs, identify the best test vector.
	// This does exhaustive creation of test vectors, scores them and
	// identifies the best - it is very expensive, performing 2^n
	// vector creations where n is the number of nodes (not pairs).
	static vector<int>* getBestTestVector(const vector<pair<int, int> > &pairs);
};
#endif
