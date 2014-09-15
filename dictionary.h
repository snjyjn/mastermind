#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

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
 *
 * A subdictionary is a dictionary that contains a subset of the entris
 * of a dictionary.  It is normally created by applying constraints to a
 * dictionary, and filtering out the elements that dont match the constraint
 * specified.   DictionaryConstraint is an Abstract Base Class that specifies
 * these constraints.
 *
 * dictionary.h and dictionary.cpp contain the entire dictionary system
 * definition and implementation.
 */

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

	int characterMatch(const charCounts &cc) const;

	int positionMatch(const string word) const;

	// Combine the character frequencies from this entry into a global
	// frequency table
	void collateStats(charCounts &frequency) const;

	// Allow the dictionary class to access all state, etc.
	friend class Dictionary;

    private:
	~DictionaryEntry();

	string word;		// The actual word

	// Derived metrics
	int len;		// word length
	charCounts freq;	// character frequencies
	int uniqChars;		// number of unique characters in the word
};

// A DictionaryConstraint is an abstract base class, which contains a
// mechanism to constrain a dictionary and hence create a subdictionary.
// The class contains a method to match with a DictionaryEntry, and return
// true if the entry is a match, and false if not.
class DictionaryConstraint {
    public:
	virtual ~DictionaryConstraint();
	virtual bool match(const DictionaryEntry *de) const = 0;
	virtual void explain(const DictionaryEntry *de) const = 0;
	virtual void debugprint() const = 0;
};

// DictConstraints is a collection of constraints.  It is also a
// constraint.  When this is specified, it implies that the entire
// collection of constraints is used to filter the dictionary.
//
// TOOO SJ: Choose a better name for this class, it is a collection
//          Also, should it be a vector or some other collection?
class DictConstraints: public DictionaryConstraint,
                       public vector<DictionaryConstraint *> {
    public:
	DictConstraints();

	virtual ~DictConstraints();
	virtual bool match(const DictionaryEntry *de) const;
	virtual void explain(const DictionaryEntry *de) const;
	virtual void debugprint() const;

	void clear();
};

class DictSizeConstraint: public DictionaryConstraint {
    public:
	DictSizeConstraint(int size);

	virtual ~DictSizeConstraint();
	virtual bool match(const DictionaryEntry *de) const;
	virtual void explain(const DictionaryEntry *de) const;
	virtual void debugprint() const;
    private:
        int size;
};


// A DictionaryConstraint that performs a match on character positions
// This is designed for the entire phrase match, or a word match
// For instance, we are aware that "abcd" matches in 2 positions, we can
// reduce the dictionary to all entries that match "abcd" in 2 positions
//
// NOTE: PosMatchContraint excludes entries that match the entire word
//       since that is known to not be a complete match!
class PosMatchConstraint : public DictionaryConstraint {
    public:
	PosMatchConstraint(string word, int matchCount);
	virtual ~PosMatchConstraint();

	virtual bool match(const DictionaryEntry *de) const;
	virtual void explain(const DictionaryEntry *de) const;
	virtual void debugprint() const;

    protected:
	string word;
	int posMatchCount;
	bool allowSelfMatch;
};

// A DictionaryConstraint that performs a match on character counts
// This is designed for the entire phrase match, or the last word
// For instance, when we are aware that a residual string "abc" must
// have exactly 2 characters which match the expected result.
class CharMatchConstraint : public DictionaryConstraint {
    public:
	CharMatchConstraint(charCounts counts, int matchCount);
	virtual ~CharMatchConstraint();

	virtual bool match(const DictionaryEntry *de) const;
	virtual void explain(const DictionaryEntry *de) const;
	virtual void debugprint() const;

    protected:
	charCounts counts;
	int charMatchCount;
};

// A DictionaryConstraint that performs a match on character counts
// This is designed for a word match before we have the final
// constraint.
// For instance, when we are aware that a string "abc" has only 1 match
// in the phrase, the word should have at most 1 of these.
//
class CharMatchWordConstraint : public CharMatchConstraint {
    public:
	CharMatchWordConstraint(charCounts counts, int matchCount);
	virtual bool match(const DictionaryEntry *de) const;
	virtual void debugprint() const;
};

// A DictionaryConstraint that implements the classic mastermind constraint
// on both character count, and position matches.
//
// NOTE: Assuming that the constraint is specified based on an existing
//       entry in the dictionary, MastermindConstraint will always reduce
//       the dictionary size by 1 when the guess is not a complete match
class MasterMindConstraint : public DictionaryConstraint {
    public:
	MasterMindConstraint(string word, int chars, int pos);
	virtual ~MasterMindConstraint();

	virtual bool match(const DictionaryEntry *de) const;
	virtual void explain(const DictionaryEntry *de) const;
	virtual void debugprint() const;

    protected:
	string word;
	int charMatchCount;
	int posMatchCount;

	int wlen;
	charCounts counts;
	bool allowSelfMatch;
};

// The actual dictionary class
class Dictionary {
    public:
	Dictionary();

	~Dictionary();

	// Read the dictionary from the file, 1 word per line
	void initialize(string filename);

	// Add a word to a dictionary
	void addWord(string word);

	// Return the i'th word in the dictionary.
	// Would have been better to implement an iterator
	const string & getWord(int i) const;

	// Get a const reference to a dictionary entry - useful for stats
	// and other reasons.
	const DictionaryEntry *getEntry(int i) const;

	const DictionaryEntry& operator[](int i) const;

	// if strategy is 0 -> create a synthetic test word and return it
	// if strategy is 1 -> The dictionary stores the word with the max
	//                     number of uniq chars in a word.  Return that
	// else return the word in the middle of the dictionary
	string getGuessWord(int strategy) const;

	string createTestWord(const GuessHistory &gh) const;

	void debugprint() const;

	string getCharsByFrequency() const;

	// Check each entry for validity.  This is not done by default.
	bool isValid() const;

	// Return the longest word length
	int getMaxWordLength() const;

	int getMinWordLength() const;

	int getWordCount() const;

	// Get a sibdirectory given a constraint
	Dictionary *getSubDictionary(const DictionaryConstraint& dc,
	                             bool debugThisCall=false) const;

    private:
	// Create a synthetic word (not from the dictionary) based on the
	// character distribution, so that it can be used for hangman!
	string createTestWord() const;

	// A valid word contains only lower case alphabets [a-z]
	static bool validateWord(string word);

	// For memory manangement, dictionary needs to delete the
	// entries.  Since we are creataing subdictionaries which
	// result in copying of references, this is done carefully
	// This method will be called only from the destructor
	void deleteEntries();

	void addEntry(DictionaryEntry *e);

	bool debug;

	// The actual collection of entries
	vector<DictionaryEntry *> *entries;

	// Dictionary Entries alloted by this dictionary
	// These are not copied to sub dictionaries, etc., and will be deleted
	// when this dictionary is deleted.
	vector<DictionaryEntry *> *allotted;

	// Derived metrics / ...
	int maxWordLength;
	int minWordLength;
	int maxUniqChars;
	string guessWord;	// word with uniq characters == maxUniqChars

	charCounts freq; 	// character frequencies
				// Number of words that have a certain character
};

class DictUtils {
    public:
	// Get a count of the number of words that match a particular size in
	// the dictionary
	static void getDictSizeDistribution(const Dictionary *d, pInt &counts);

	// Get a count of the dictionary entries which have a particular char
	static charCounts getDictCharDistribution(const Dictionary *d);
};

#endif
