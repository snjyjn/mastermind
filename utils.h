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
	static void initCounts(int *count);
	static void countCharacters(const string& word, int *count);
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

// For analysis purposes:
class GuessAnalytics {
    public:
	GuessAnalytics();

	void setState(int s);
	void printAnalysis();
	void addAttempt();

	void addDictSize(int wordNumber, int size);

	static const int START = 0;
	static const int WORDLEN = 1;
	static const int WORD1CHARS = 2;
	static const int WORD1GUESS = 3;
	static const int WORD2CHARS = 4;
	static const int WORD2GUESS = 5;
	static const int WORD3CHARS = 6;
	static const int WORD3GUESS = 7;
	static const int PHRASETEST = 8;

    private:
	int count;
	int dictSizes[3];
	int state;
	vector<int> *attempts;
};

// Global Variable!
extern GuessAnalytics *analytics;

#endif
