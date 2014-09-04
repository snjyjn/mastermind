#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>

using namespace std;

// Common typedefs
typedef int *pInt;

// Basic utility functions to be used throughout the program
class utils {
    public:
	static char itoc(int i);
	static int ctoi(char c);
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

	// Access operators - make this behave more like an Array
	int &operator[](int i);
	const int &operator[](int i) const;

	void reset();

	// Add character counts from the word
	void addToCount(const string &word);

	// Add from another frequency counts
	void addToCount(const charCounts& other);

	int match(const charCounts& other) const;

	int uniqCounts() const;

	string getCharsByFrequency() const;

	void debugprint() const;

    private:
	int counts[28];
};

// Keep track of all the guesses, and responses from the matcher.
class GuessHistoryElement {
    public:
	GuessHistoryElement(const string word, int pos, int chars);

	string word;		// The word sent to the matcher
	int pos;		// The number of positions that matched
	int chars;		// The number of characters that matched

	// Replicate the matcher in passphrase, so that candidates can 
	// be compared with historical attempts
	bool phraseMatch(const charCounts &candidateCounts, 
	                 const string &candidate) const;

	bool phraseMatch(const string &candidate) const;

	// Use the matcher with words (sub phrases) for early detection of
	// words that should not be used 
	// differs in that, it only uses the character counts, and looks for 
	// a max number of matches.
	bool subPhraseMatch(const charCounts &candidateCounts) const;

    private:
	charCounts counts;
};

typedef vector<GuessHistoryElement *>  GuessHistory;

// For analysis purposes:
class GuessAnalytics {
    public:
	GuessAnalytics();

	void setState(int s);
	void printAnalysis();
	void addAttempt();

	void addDictSize(int wordNumber, int attempt, int size);

	static const int PHRASELEN = 0;
	static const int WORDLEN = 1;
	static const int WORD1GUESS = 2;
	static const int WORD2GUESS = 3;
	static const int WORD3GUESS = 4;
	static const int PHRASEGUESS = 5;
	static const int PHRASETEST = 6;

    private:
	int count;
	int dictSizes[5][50];
	int dictSizeCount[5][50];
	int state;
	vector<int> *attempts;
};

// Global Variable!
extern GuessAnalytics *analytics;

#endif
