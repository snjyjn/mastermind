#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>

#include "utils.h"
#include "passphrase.h"
#include "dictionary.h"
#include "spacefinder.h"

using namespace std;

// Keep track of all the guesses, and responses from the matcher.
class GuessHistoryElement {
    public:
	GuessHistoryElement(const string word, int pos, int chars) {
	    this->word = word;
	    this->pos = pos;
	    this->chars = chars;
	}

	string word;		// The word sent to the matcher
	int pos;		// The number of positions that matched
	int chars;		// The number of characters that matched
};

// The main solver class.
class Mastermind {
    public:
	// Constructor - initialize it with a reference to the dictionary, 
	// so that it can get some basic stats from there.
	Mastermind(Dictionary *d) {
	    dict = d;
	    maxLength = dict->getMaxWordLength();
	    minLength = dict->getMinWordLength();
	    p = NULL;
	    debug = false;
	}

	// The main solver method.  Given a passphrase matcher, solve
	// and return the solution.
	string guess(PassPhrase *p) {
	    this->p = p;

	    //
	    // Step 1. Find the spaces.  Use SpaceFinder for this purpose
	    //
	    int space1 = -1;
	    int space2 = -1;
	    analytics->setState(GuessAnalytics::WORDLEN);
	    SpaceFinder *bf = new SpaceFinder(p, minLength, maxLength);
	    string rc = bf->findSpaces(space1, space2);

	    // Initialize GuessHistory.  Note that the guesses used for
	    // finding the spaces are not stored in here.
	    // TODO: Explore the possibility of doing that, and of 
	    // introducing extra text in there to get more information

	    vector<GuessHistoryElement *> guessHistory;

	    // The guess as we keep building it up
	    string base_guess = "";

	    analytics->setState(GuessAnalytics::WORD1GUESS);
	    //
	    // Step 2. .Word 1:  
	    //         .We know the length, get a subdirectory with all words 
	    //          of that length
	    //         .Make a guess, use it with the matcher
	    //         .Reduce the dictionary to all words that match the guess
	    //          in the same way as the passphrase matcher (by position)
	    //         .Repeat till done
	    // Improve the process by using a synthetic word for 1st guess
	    //

	    // Find a sub dictionary with all words that have the right 
	    // length!
	    Dictionary *d1 = dict->getSubDictionary(space1);
	    analytics->addDictSize(0, d1->getWordCount());

	    bool first = true;
	    bool found = false;
	    string word1;
	    while ((found == false) && (d1->getWordCount() > 1)) {
		if (first == true) {
		    // First guess for the word - use a synthetic word!
		    word1 = d1->createTestWord();
		    first = false;
		} else {
		    word1 = d1->getGuessWord();
		}
		int pos, chars;
		p->match(word1, pos, chars);
		guessHistory.push_back(new GuessHistoryElement(word1, pos, chars));
		if (pos == word1.length()) {
		    found = true;
		} else {
		    // reduce the dictionary further.
		    // TODO: Fix memory leaks!
		    d1 = d1->getSubDictionary(word1, pos);
		}
	    }
	    if (found == false) {
		if (d1->getWordCount() == 1) {
		    // We have not even tested the word, but dict is size 1
		    word1 = d1->getWord(0);
		} else {
		    assert(false);
		}
	    }

	    // Add the word and space to the guess
	    base_guess.append(word1);
	    base_guess.append(" ");


	    //
	    // Step 3. .Word 2:  
	    //         .Similar to Word 1, except that the code has to include
	    //          a 'prefix' character string before the word.
	    //

	    string prefix;
	    prefix.append(1+space1,'0');  // Create a string of 0's that
	                                  // will replace word1 and the space

	    Dictionary *d2 = dict->getSubDictionary(space2 - space1 - 1);
	    analytics->addDictSize(1, d2->getWordCount());

	    analytics->setState(GuessAnalytics::WORD2GUESS);

	    first = true;
	    found = false;
	    string word2;
	    while ((found == false) && (d2->getWordCount() > 1)) {
		if (first == true) {
		    word2 = d2->createTestWord();
		    first = false;
		} else {
		    word2 = d2->getGuessWord();
		}
		string w = prefix;
		w.append(word2);
		int pos, chars;
		p->match(w, pos, chars);
		guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
		if (pos == word2.length()) {
		    found = true;
		} else {
		    d2 = d2->getSubDictionary(word2, pos);
		}
	    }
	    if (found == false) {
		if (d2->getWordCount() == 1) {
		    word2 = d2->getWord(0);
		} else {
		    d2->debugprint();
		    assert(false);
		}
	    }

	    // Add the second word, and space to the guess
	    base_guess.append(word2);
	    base_guess.append(" ");

	    //
	    // Step 4. .Word 3:  
	    //         .This differs from the 1st 2 words in 2 significant ways
	    //         .1. We dont know the length of the word (-ve)
	    //         .2. We have a lot of information from prev guesses (+ve)
	    //         Use the previous guesses, compare with the current known
	    //         word1 and word2, identify the residual constraint, and 
	    //         apply it to the dictionary, reducing it.
	    //         .Now, follow the word guessing code that we used for
	    //         words 1 and 2.
	    //

	    vector<DictionaryConstraint *> constr 
		= createDictConstraints(base_guess, guessHistory);

	    Dictionary *d3 = dict->getSubDictionary(constr);
	    analytics->addDictSize(2, d3->getWordCount());

	    prefix.append(space2-space1,'0');  
	    // Create a string of 0's that will replace word1, word2 and
	    // the spaces after each!
	    if (prefix.length() != (space2+1)) {
		assert(false);
	    }

	    analytics->setState(GuessAnalytics::WORD3GUESS);
	    first = true;
	    found = false;
	    string word3;
	    while ((found == false) && (d3->getWordCount() > 1)) {
		if (first == true) {
		    word3 = d3->createTestWord();
		    first = false;
		} else {
		    word3 = d3->getGuessWord();
		}
		string w = prefix;
		w.append(word3);
		int pos, chars;
		p->match(w, pos, chars);
		guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
		if (pos == word3.length()) {
		    found = true;
		} else {
		    d3 = d3->getSubDictionary(word3, pos);
		}
	    }
	    if (found == false) {
		if (d3->getWordCount() == 1) {
		    word3 = d3->getWord(0);
		} else {
		    assert(false);
		}
	    }

	    // We are now done!

	    guessHistory.clear();

	    // Add the third word to the guess
	    base_guess.append(word3);
	    return base_guess;
	}

	// Use guess history, and known information so far from the first
	// 2 words to constrain the dictionary.
	// See details in the comments in the code!
	// Build a vector of constraints, to be applied.
	vector<DictionaryConstraint *>& 
	createDictConstraints(const string &base_guess, 
			      const vector<GuessHistoryElement *>& hist) {
	    vector<DictionaryConstraint *> *rc 
		= new vector<DictionaryConstraint *>;

	    // Get char counts for the now known information (words 1 and 2)
	    int base_counts[27];
	    utils::initCounts(base_counts);
	    utils::countCharacters(base_guess, base_counts);

	    // For each previous guess
	    for (vector<GuessHistoryElement *>::const_iterator it = hist.cbegin();
		it != hist.cend(); ++it) {
		GuessHistoryElement *g = (*it);

		// Get char counts for the guess string
		int word_counts[27];
		utils::initCounts(word_counts);
		utils::countCharacters(g->word, word_counts);

		int pending_counts[27];
		utils::initCounts(pending_counts);

		//
		// Identify the characters that matched the known words
		// Look at the response from passphrase matcher, identify 
		// the pending match count, and unaccounted characters in the 
		// test word.
		//
		int char_matches_so_far = 0;
		string pending_str;
		pending_str.clear();
		int unAccountedCharacterCount = 0;
		for (int i=0; i<27; ++i) {
		    int match = min(word_counts[i], base_counts[i]);
		    char_matches_so_far += match;
		    pending_counts[i] = word_counts[i] - match;
		    unAccountedCharacterCount += pending_counts[i];
		    if (pending_counts[i] < 0) {
			assert(false);
		    } else if (pending_counts[i] > 0) {
			pending_str.append(pending_counts[i], utils::itoc(i));
		    }
		}
		int pending = g->chars - char_matches_so_far;
		//
		// If there are unaccounted characters, add them to the 
		// constraint vector.  For instance, if the history contains
		//     guess: "abc" char match: 2
		//     base contains 'a', but not 'b' or 'c'
		//     pending_str = "bc", pending count = 1
		// Now setup a constraint so that only dictionary words that
		// have a 'b' or a 'c' will remain.
		//
		if (unAccountedCharacterCount > 0) {
		    DictionaryConstraint *dc 
			= new CharMatchConstraint(pending_counts, pending);
		    rc->push_back(dc);
		}

		if (debug) {
		    cout << "So Far         : @" << base_guess << "@" << consts::eol
			 << "Previous Guess : @" << g->word << "@" << consts::eol
			 << "Pending        : @" << pending_str << "@" << consts::eol
			 << "Stats: (matches: " <<  g->chars << "), (so far: "
			 << char_matches_so_far << "), (pending: "
			 << pending << ")" << consts::eol;
		    for (int i=0; i<27; ++i) {
			if (word_counts[i] > 0) {
			    cout << "(" << utils::itoc(i) << ": " 
				 << word_counts[i] << ", "
				 << base_counts[i] << ", "
				 << pending_counts[i]  << ")";
			}
		    }
		    cout << consts::eol;
		}
	    }

	    return (*rc);
	}

    private:
	Dictionary *dict;
	int maxLength;
	int minLength;
	PassPhrase *p;
	bool debug;
};


int main (int argc, char **argv) 
{
    // Get the phrases from the file
    Dictionary phrases;
    phrases.initialize(consts::phraseFileName);

    // Get the words for the dictionary from the file
    Dictionary dict;
    dict.initialize(consts::dictionaryFileName);
    if (!dict.isValid()) {
	cout << "Dictionary is not valid" << consts::eol;
    }

    Mastermind *m = new Mastermind(&dict);

    for (int i=0; i<phrases.getWordCount(); ++i) {
	const string phrase = phrases.getWord(i);
	PassPhrase *secret = new PassPhrase(phrase);
	string guess = m->guess(secret);
	analytics->setState(GuessAnalytics::PHRASETEST);

	bool resp = secret->match(guess);
	if (resp != true) {
	    cout << "Testing :" << phrase << consts::eol;
	    cout << "Guessed :" << guess << consts::eol;
	}
    }
    analytics->printAnalysis();
}
