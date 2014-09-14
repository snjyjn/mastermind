#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>

using namespace std;

// Common typedefs
typedef int *pInt;
class GuessHistoryElement;
typedef vector<GuessHistoryElement *>  GuessHistory;

// Basic utility functions to be used throughout the program
class utils {
    public:
	static char itoc(int i);
	static int ctoi(char c);

	// SJ TODO : Remove this!
	static void clearGuessHistory(GuessHistory &gh);
};

// Basic constants to be used throughout the program
class consts {
    public:
	// Characters
	static const char spc;	// space character
	static const char zpc;	// Zero Count Placeholder
				// Guaranteed not to match

	// For pretty printing only
	static const char eol;
	static const char tab;

	// Etc.
	static const string dictionaryFileName;
	static const string phraseFileName;
};

// A helper class to keep character counts - we seem to be doing this all
// the time!
class charCounts {
    public:
	// Array size to be used for storing character counts, etc.
	static const int charArraySize;

	// Constructor
	charCounts();

	// Copy Constructor
	charCounts(const charCounts &c);

	~charCounts();

	// Access operators - make this behave more like an Array
	int &operator[](int i);
	const int &operator[](int i) const;

	void reset();

	// Add character counts from the word
	void addToCount(const string &word);

	// Add from another frequency counts
	void addToCount(const charCounts& other);

	int match(const charCounts& other) const;

	// Returns if "other" is a equal of this
	bool isEqual(const charCounts& other) const;

	// Returns if "other" is a subset of this
	bool isSubSet(const charCounts& other) const;

	// remove another word from this (repr as a counter)
	// if other word has chars that are not here (or have a higher count)
	// that is ignored.  This is the same as Set difference: 
	// (A - B) is same as (A-(A intersection B))
	void removeFromCount(const charCounts& other);

	int uniqCounts() const;

	string getCharsByFrequency() const;

	string getChars() const;

	void debugprint() const;

    private:
	int counts[28];
};

// Keep track of all the guesses, and responses from the matcher.
class GuessHistoryElement {
    public:
	GuessHistoryElement(const string word, int pos, int chars);

	~GuessHistoryElement();

	string word;		// The word sent to the matcher
	int pos;		// The number of positions that matched
	int chars;		// The number of characters that matched
	// Replicate the matcher in passphrase, so that candidates can
	// be compared with historical attempts
	bool phraseMatch(const charCounts &candidateCounts,
	                 const string &candidate) const;

	bool phraseMatch(const string &candidate) const;

    private:
	charCounts counts;
};


// For analysis purposes:
class GuessAnalytics {
    public:
	GuessAnalytics();

	void setState(int s);
	void printAnalysis();
	void addAttempt();

	void addDictSize(int wordNumber, int attempt, long long size);

	static const int PHRASELEN = 0;
	static const int WORDLEN = 1;
	static const int WORD1GUESS = 2;
	static const int WORD2GUESS = 3;
	static const int WORD3GUESS = 4;
	static const int PHRASEGUESS = 5;
	static const int PHRASETEST = 6;

    private:
	int count;
	long long dictSizes[6][50];
	int dictSizeCount[6][50];
	int state;
	vector<int> *attempts;
};

// Global Variable!
extern GuessAnalytics *analytics;

#endif
