#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <vector>

#include "utils.h"

using namespace std;

/* 
 * A Dictionary contains all the words that can be used within the passphrase.
 * The dictionary class provides a mechanism to initialize from a file, 
 * perform precomputations, collect statistics, and support the guesser.
 * Operations include creating subdictionaries based on specified constraints.
 *
 * A Dictionary contains DictionaryEntry objects, each of which represents
 * a word, with associated metrics.
 */

// Only dictionary needs to access dictionary entries!  Use friend!
class DictionaryEntry {
    public:
	DictionaryEntry(string word);

	// Useful for debugging only
	void debugprint() const;

	int length() const;

	int uniqueCharacterCount() const;

	const string &getWord() const;

	// Compare 2 dictionary entries, and count the number of characters 
	// that match
	int characterMatch(const DictionaryEntry* de) const;

	// Compare a dictionary entry with a word, defined by char frequencies
	int characterMatch(const int counts[]) const;

	// Combine the character frequencies from this entry into a global
	// frequency table
	void collateStats(int frequency[]) const;

	// Allow the dictionary class to access all state, etc.
	friend class Dictionary;
    private:
	// Destructor - should not be called till the end of the process
	~DictionaryEntry();

	string word;		// The actual word

	// Derived metrics
	int len;		// word length
	int counts[27];		// character frequencies
	int uniqChars;		// number of unique characters in the word
};


// A DictionaryConstraint is an abstract base class, which contains a 
// mechanism to constrain a dictionary and hence create a subdictionary.
// The class contains a method to match with a DictionaryEntry, and return
// true if the entry is a match, and false if not.
class DictionaryConstraint {
    public:
	virtual bool match(DictionaryEntry *de) const = 0;
};

// A DictionaryConstraint that performs a match on character counts
class CharMatchConstraint : public DictionaryConstraint {
    public:
	CharMatchConstraint(int charCounts[], int matchCount);
	virtual bool match(DictionaryEntry *de) const;

    private:
	int counts[27];
	int charMatchCount;
};

// The actual dictionary class
class Dictionary {
    public:
	Dictionary();

	// Read the dictionary from the file, 1 word per line
	void initialize(string filename);

	// Return the i'th word in the dictionary.
	// Would have been better to implement an iterator
	const string & getWord(int i) const;

	// The dictionary stores the word with the max number of uniq chars
	// in a word.  Return that word.
	const string & getGuessWord() const;

	// Create a synthetic word (not from the dictionary) based on the 
	// character distribution, so that it can be used for hangman!
	string createTestWord() const;

	void debugprint() const;

	// Check each entry for validity.  This is not done by default.
	bool isValid() const;

	// Return the longest word length
	int getMaxWordLength() const;

	int getMinWordLength() const;

	int getWordCount() const;

	// TODO: Convert all subdictionary methods to use DictionaryConstraint
	// and to actually reduce the size of the current dictionary.
	// Also provide a shallow clone method.

	// Get a sibdirectory of all words with a given size
	Dictionary *getSubDictionary(int size);

	// Get a subdirectory of all words that match the guess in terms 
	// of number of characters (present in the guess)
	Dictionary *getSubDictionary(string word, int positionMatches) const;

	// Apply a collection of constraints and get a smaller subdictionary
	Dictionary *getSubDictionary(vector<DictionaryConstraint *> constr) const;

    private:
	void getStats(int frequency[]) const;

	// A valid word contains only lower case alphabets [a-z]
	static bool validateWord(string word);

	void addWord(string word);
	void addEntry(DictionaryEntry *e);

	// The actual collection of entries
	vector<DictionaryEntry *> *entries;

	// Derived metrics / ...
	int maxWordLength;
	int minWordLength;
	int maxUniqChars;
	string guessWord;	// word with uniq characters == maxUniqChars

	int counts[27];		// character frequencies
				// Number of words that have a certain character
};

#endif
