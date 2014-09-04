#include <iostream>

#include "utils.h"
#include "passphrase.h"

using namespace std;

/* 
 * Passphrase represents a user passphrase.  This consists of 3 dictionary
 * words, separated by a space (hence 2 spaces).  It provides a match
 * functionality which 'leaks' certain information to the requester.  
 * The goal of the requester is to use different techniques to match 
 * the password as reliably as possible.
 */

// Constructor, create the phrase, and initialize data structures
PassPhrase::PassPhrase(const string phrase) {
   debug = false;
   this->phrase = phrase;
   counts.addToCount(phrase);
}

void
PassPhrase::debugprint() const {
   cout << phrase << consts::eol;
   counts.debugprint();
}

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
bool
PassPhrase::match(string candidate, int &positions, int &characters) const {
    analytics->addAttempt();
    positions = 0;
    string::iterator c=candidate.begin();
    string::const_iterator p=phrase.cbegin();
    while (c!= candidate.end() && p != phrase.cend()) {
	if (*c == *p) {
	    positions++;
	}
	++c; ++p;
    }

    charCounts candidateCounts;
    candidateCounts.addToCount(candidate);
    characters = candidateCounts.match(counts);

    bool rc = (positions == candidate.length());
    if (debug) {
	cout << "@" << candidate.substr(0,65) << "@" 
	     << ":p " << positions << ":c " << characters
	     << ": " << rc
	     << consts::eol;
    }
    return rc;
}

// Same as the match call
bool
PassPhrase::match(string candidate) const {
    int pos, chars;
    return match(candidate, pos, chars);
}
