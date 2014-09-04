#ifndef __PASSPHRASE_H__
#define __PASSPHRASE_H__

using namespace std;

/*
 * Passphrase represents a user passphrase.  This consists of 3 dictionary
 * words, separated by a space (hence 2 spaces).  It provides a match
 * functionality which 'leaks' certain information to the requester.
 * The goal of the requester is to use different techniques to match
 * the password as reliably, and as quickly as possible.
 */

class PassPhrase {
    public:
	// Constructor, create the phrase, and initialize data structures
	PassPhrase(const string phrase);

	// match the phrase with a candidate.
	// Return true if it is a complete match
	// Return false otherwise
	// positions : The number of characters that match at the same position
	//             in the candidate and passphrase
	// characters : The number of characters that match at any position.
	// if the passphrase contains 2 x's, the character matches will be
	// a. candidate has 1 x: characters = 1
	// b. candidate has 2 x's: characters = 2
	// c. candidate has 3 x's: characters = 2
	bool match(string candidate, int &positions, int &characters) const;

	// Shorter version, returns only if the candidate is a perfect match!
	bool match(string candidate) const;

	// There is no get method for the passphrase, nor is there any method
	// other than match to gather information about the phrase.  This is
	// AS PER DESIGN.  The problem is to guess this phrase!
	// However, for debugging purposes, a print method is useful!
	void debugprint() const;

    private:
	string phrase;
	charCounts counts;
	bool debug;
};
#endif
