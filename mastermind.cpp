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
	    dictFreq = dict->getCharsByFrequency();
	    if (debug)
		cout << dictFreq;
	}

	// The main solver method.  Given a passphrase matcher, solve
	// and return the solution.
	string guess(PassPhrase *p) {
	    this->p = p;

	    // Initialize GuessHistory.  
	    // TODO: Explore the possibility of doing that, and of 
	    // introducing extra text in there to get more information

	    vector<GuessHistoryElement *> guessHistory;

	    //
	    // Step 0. Find the length of the passphrase
	    //
	    analytics->setState(GuessAnalytics::PHRASELEN);
	    char firstChar; int firstCharCount;
	    findPhraseLength(guessHistory, firstChar, firstCharCount);
	    string first;
	    first.append(firstCharCount, firstChar);
	    charCounts firstCount;
	    firstCount.addToCount(first);

	    //
	    // Step 1. Find the spaces.  Use SpaceFinder for this purpose
	    //
	    int space1 = -1;
	    int space2 = -1;
	    analytics->setState(GuessAnalytics::WORDLEN);
	    SpaceFinder *sf = new SpaceFinder(p, dict, minLength, maxLength, phraseLength);
	    DictConstraints wordConstraints 
		= sf->findSpaces(guessHistory, space1, space2);
	    DictionaryConstraint *dc 
		= new CharMatchWordConstraint(firstCount, firstCharCount);
	    wordConstraints.push_back(dc);

	    // Reduce the dictionary based on the extra information 
	    // obtained while finding the spaces!
	    Dictionary *d0 = dict->getSubDictionary(wordConstraints);
	    analytics->addDictSize(3, 0, d0->getWordCount());
	    wordConstraints.clear();


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
	    int count1, count2, count3;
	    count1= 0; count2 = 0; count3 = 0;
	    Dictionary *d1 = d0->getSubDictionary(space1);
	    Dictionary *d2 = d0->getSubDictionary(space2 - space1 - 1);
	    Dictionary *d3 = d0->getSubDictionary(phraseLength - space2 - 1);
	    analytics->addDictSize(0, count1, d1->getWordCount());
	    analytics->addDictSize(1, count2, d2->getWordCount());
	    analytics->addDictSize(2, count3, d3->getWordCount());

	    string prefix1; prefix1.append(1+space1, consts::zpc);
	    string prefix2; prefix2.append(1+space2, consts::zpc);
	    int pos, chars;
	    string word, w;
	    charCounts wordCounts;

	    int wordsFound = 0;
	    while (wordsFound < 2) {
		wordsFound = 0;
		if (d1->getWordCount() > 1) {
		    analytics->setState(GuessAnalytics::WORD1GUESS);
		    word = d1->getGuessWord(count1++);
		    p->match(word, pos, chars);
		    guessHistory.push_back(
			new GuessHistoryElement(word, pos, chars));
		    d1 = d1->getSubDictionary(word, pos);

		    wordCounts.reset(); wordCounts.addToCount(word);
		    dc = new CharMatchWordConstraint(wordCounts, chars);
		    wordConstraints.push_back(dc);
		} else {
		    wordsFound++;
		}

		d2 = d2->getSubDictionary(wordConstraints);
		if (d2->getWordCount() > 1) {
		    analytics->setState(GuessAnalytics::WORD2GUESS);
		    word = d2->getGuessWord(count2++);
		    w = prefix1; w.append(word);
		    p->match(w, pos, chars);
		    guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
		    d2 = d2->getSubDictionary(word, pos);

		    wordCounts.reset(); wordCounts.addToCount(word);
		    dc = new CharMatchWordConstraint(wordCounts, chars);
		    wordConstraints.push_back(dc);
		} else {
		    wordsFound++;
		}

#if 0
		d3 = d3->getSubDictionary(wordConstraints);
		if (d3->getWordCount() > 1) {
		    analytics->setState(GuessAnalytics::WORD3GUESS);
		    word = d3->getGuessWord(count3++);
		    w = prefix2; w.append(word);
		    p->match(w, pos, chars);
		    guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
		    d3 = d3->getSubDictionary(word, pos);

		    wordCounts.reset(); wordCounts.addToCount(word);
		    dc = new CharMatchWordConstraint(wordCounts, chars);
		    wordConstraints.push_back(dc);
		} else {
		    wordsFound++;
		}
#endif
		d1 = d1->getSubDictionary(wordConstraints);
		d2 = d2->getSubDictionary(wordConstraints);
		d3 = d3->getSubDictionary(wordConstraints);
		wordConstraints.clear();

		analytics->addDictSize(0, count1, d1->getWordCount());
		analytics->addDictSize(1, count2, d2->getWordCount());
#if 0
		analytics->addDictSize(2, count3, d3->getWordCount());
#else
		++count3;
		analytics->addDictSize(2, count3, d3->getWordCount());
#endif
	    }
	    string word1 = d1->getGuessWord(1);
	    string word2 = d2->getGuessWord(1);

	    // Add the word and space to the guess
	    base_guess.append(word1);
	    base_guess.append(" ");

	    // Add the second word, and space to the guess
	    base_guess.append(word2);
	    base_guess.append(" ");

	    wordConstraints = createDictConstraints(base_guess, guessHistory);

	    d3 = d3->getSubDictionary(wordConstraints);
	    analytics->addDictSize(2, count3++, d3->getWordCount());

	    analytics->setState(GuessAnalytics::WORD3GUESS);
	    bool found = false;
	    string word3;
	    while ((found == false) && (d3->getWordCount() > 1)) {
		    word3 = d3->getGuessWord(count3++);
		string w = prefix2;
		w.append(word3);
		int pos, chars;
		p->match(w, pos, chars);
		guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
		if (pos == word3.length()) {
		    found = true;
		} else {
		    d3 = d3->getSubDictionary(word3, pos);
		}
		analytics->addDictSize(2, count3, d3->getWordCount());
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

	// Find the length of the phrase,and the count of the most frequent
	// character (frequency is based on the dictionary, and not the 
	// phrase).
	// The string contains (maxPossibleLength) instances of each character
	// in the alphabet, starting with the most frequent (the others could
	// be in any order).  It also contains at least 2 spaces.
	// The matcher will respond with position matches = count of most freq
	// and character matches = length of string
	// Thanks to Roy for this wonderful idea!
	void findPhraseLength(vector<GuessHistoryElement *> &guessHistory,
	    char &firstChar, int &firstCharCount) {
	    string lengthTestString;

	    for (int i=0; i<dictFreq.length(); ++i) {
		lengthTestString.append((3*maxLength), dictFreq[i]);
	    }
	    firstChar = dictFreq[0];
	    lengthTestString.append(2, consts::spc);
	    int pos, chars;
	    p->match(lengthTestString, pos, chars);
	    // Number of characters that match is the phrase length.
	    // This includes the spaces between the words
	    phraseLength = chars;

	    // The count of the most frequent character is 'pos'.  
	    // we depend on the guess history to take it into account;
	    firstCharCount = pos;
	    guessHistory.push_back(new GuessHistoryElement(lengthTestString, pos, chars));
	}

	// Use guess history, and known information so far from the first
	// 2 words to constrain the dictionary.
	// See details in the comments in the code!
	// Build a vector of constraints, to be applied.
	DictConstraints&
	createDictConstraints(const string &base_guess, 
			      const vector<GuessHistoryElement *>& hist) {
	    DictConstraints *rc = new DictConstraints();

	    // Get char counts for the now known information (words 1 and 2)
	    charCounts base_counts;
	    base_counts.addToCount(base_guess);

	    // For each previous guess
	    for (vector<GuessHistoryElement *>::const_iterator it = hist.cbegin();
		it != hist.cend(); ++it) {
		GuessHistoryElement *g = (*it);

		// Get char counts for the guess string
		charCounts word_counts;
		word_counts.addToCount(g->word);

		charCounts pending_counts;

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
		for (int i=0; i<charCounts::charArraySize; ++i) {
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
		    for (int i=0; i<charCounts::charArraySize; ++i) {
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
	string dictFreq;	// Characters in dict, sorted by count of words
				// they show up in.

	int phraseLength;
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
