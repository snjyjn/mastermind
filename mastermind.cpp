#include <iostream>
#include <algorithm>
#include <assert.h>

#include "spacefinder.h"
#include "mastermind.h"

using namespace std;

// The main solver class.
// Constructor - initialize it with a reference to the dictionary,
// so that it can get some basic stats from there.
Mastermind::Mastermind(Dictionary *d) {
    dict = d;
    maxLength = dict->getMaxWordLength();
    minLength = dict->getMinWordLength();
    p = NULL;
    debug = false;
    dictFreq = dict->getCharsByFrequency();
    if (debug)
	cout << dictFreq;
}

Mastermind::~Mastermind() {
}

// The main solver method.  Given a passphrase matcher, solve
// and return the solution.
string
Mastermind::guess(PassPhrase *p) {
    this->p = p;
    TestPatternGenerator *tpg = new TestPatternGenerator(dict);

    // Initialize GuessHistory.

    GuessHistory guessHistory;

    // Setup a collection of constraints that we will want to apply
    // to individual words / sub phrases
    DictConstraints *wordConstraints = NULL;

    // Similarly for phrases
    DictConstraints phraseConstraints;

    //
    // Step 1. Find the spaces and phrase length.  Use SpaceFinder for
    //         this purpose
    //
    int space1 = -1;
    int space2 = -1;
    analytics->setState(GuessAnalytics::WORDLEN);

    // Setup the space finder, including references to the dictionary,
    // guess history collection, test pattern generator, etc.
    SpaceFinder *sf = new SpaceFinder(dict, p, tpg, &guessHistory,
                                      minLength, maxLength);

    // Find the phrase length first
    phraseLength = sf->findPhraseLength();

    // Find the space locations, returns the constraints to be used
    // for the word based sub dictionaries.
    wordConstraints = sf->findSpaces(space1, space2);

    delete sf; sf = NULL;

    // Reduce the dictionary based on the extra information
    // obtained while finding the spaces!
    Dictionary *d0 = dict->getSubDictionary(*wordConstraints);
    analytics->addDictSize(0, 0, d0->getWordCount());

    // For performance, we can reduce constraints once applied
    // guessHistory still contains all the guesses made, so the
    // knowledge is not lost!
    wordConstraints->clear();

    // tpg->debugprint();

    //
    // Step 2. Create a subdictionary per word with the right size
    //         Use a synthetic word guess to reduce the sizes
    //         Use a word guess to further reduce the sizes
    //         Costs: 6 guesses

    // Declarations for variables we will use in this section
    string prefix1; prefix1.append(1+space1, consts::zpc);
    string prefix2; prefix2.append(1+space2, consts::zpc);
    int pos, chars;
    string word, w;
    charCounts wordCounts;

    // Counters
    int count1, count2, count3;
    count1= 0; count2 = 0; count3 = 0;

    DictSizeConstraint word1SizeConstraint(space1);
    DictSizeConstraint word2SizeConstraint(space2-space1-1);
    DictSizeConstraint word3SizeConstraint(phraseLength-space2-1);

    Dictionary *d1 = d0->getSubDictionary(word1SizeConstraint);
    Dictionary *d2 = d0->getSubDictionary(word2SizeConstraint);
    Dictionary *d3 = d0->getSubDictionary(word3SizeConstraint);

    analytics->addDictSize(1, count1, d1->getWordCount());
    analytics->addDictSize(2, count2, d2->getWordCount());
    analytics->addDictSize(3, count3, d3->getWordCount());

    int testPhraseCounter = 0;
    DictionaryConstraint *dc = NULL;
    for (int att=0; att<0; ++att) {
	if (d1->getWordCount() > 1) {
	    analytics->setState(GuessAnalytics::WORD1GUESS);
	    count1++;
	    word = d1->getGuessWord(att);	// synthetic guess word
	    w = appendTestPhrase(word, tpg);
	    p->match(w, pos, chars);
	    guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
	    {
		PosMatchConstraint constr(word, pos);
		Dictionary *tmp = d1->getSubDictionary(constr);
		delete d1; d1 = tmp;
	    }

	    wordCounts.reset(); wordCounts.addToCount(word);
	    dc = new CharMatchWordConstraint(wordCounts, chars);
	    wordConstraints->push_back(dc);
	}

	{
	    Dictionary *tmp = d2->getSubDictionary(*wordConstraints);
	    delete d2; d2 = tmp;
	}
	if (d2->getWordCount() > 1) {
	    analytics->setState(GuessAnalytics::WORD2GUESS);
	    count2++;
	    word = d2->getGuessWord(att);	// synthetic guess word
	    w = prefix1; w.append(word);
	    w = appendTestPhrase(w, tpg);
	    p->match(w, pos, chars);
	    guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
	    {
		PosMatchConstraint constr(word, pos);
		Dictionary *tmp = d2->getSubDictionary(constr);
		delete d2; d2 = tmp;
	    }

	    wordCounts.reset(); wordCounts.addToCount(word);
	    dc = new CharMatchWordConstraint(wordCounts, chars);
	    wordConstraints->push_back(dc);
	}

#if 0
	{
	    Dictionary *tmp = d3->getSubDictionary(*wordConstraints);
	    delete d3; d3 = tmp;
	}
	if (d3->getWordCount() > 1) {
	    analytics->setState(GuessAnalytics::WORD3GUESS);
	    count3++;
	    word = d3->getGuessWord(att);	// synthetic guess word
	    w = prefix2; w.append(word);
	    w = appendTestPhrase(w, tpg);
	    p->match(w, pos, chars);
	    guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
	    d3 = d3->getSubDictionary(word, pos);

	    wordCounts.reset(); wordCounts.addToCount(word);
	    dc = new CharMatchWordConstraint(wordCounts, chars);
	    wordConstraints->push_back(dc);
	}
#endif

	{
	    Dictionary *tmp;
	    tmp = d1->getSubDictionary(*wordConstraints);
	    delete d1; d1 = tmp;

	    tmp = d2->getSubDictionary(*wordConstraints);
	    delete d2; d2 = tmp;

	    tmp = d3->getSubDictionary(*wordConstraints);
	    delete d3; d3 = tmp;
	}

	analytics->addDictSize(1, count1, d1->getWordCount());
	analytics->addDictSize(2, count2, d2->getWordCount());
	analytics->addDictSize(3, count3, d3->getWordCount());
    }
    wordConstraints->clear();
    delete wordConstraints;

    Dictionary *phraseDictionary = new Dictionary();

    //
    // Now we get to the creating the phrase dictionary
    //
    for (int i=0; i<d1->getWordCount(); ++i) {
	string word1 = d1->getWord(i);
	for (int j=0; j<d2->getWordCount(); ++j) {
	    string word2 = d2->getWord(j);
	    string guess;
	    guess.append(word1);
	    guess.append(1, consts::spc);
	    guess.append(word2);
	    guess.append(1, consts::spc);
	    // Constrain the dictionary word 3
	    // From the phrase so far (words 1 and 2), and the guessHistory
	    // create a set of constraints for the remaining word.
	    // Use that to constrain the dictionary, and then create the full
	    // phrase.
	    DictConstraints *constr = createDictConstraints(guess, guessHistory);
	    Dictionary *d4 = d3->getSubDictionary(*constr);
	    constr->clear();
	    delete constr;
	    for (int k=0; k<d4->getWordCount(); ++k) {
		string word3 = d4->getWord(k);
		string phrase; phrase.append(guess);
		phrase.append(word3);
		bool match = true;
		for (int l=0; (match && l<guessHistory.size()); ++l) {
		    GuessHistoryElement *g = guessHistory[l];
		    if (g->phraseMatch(phrase) == false) {
			match = false;
			if (debug) {
			    // TODO
			}
		    }
		}
		if (match)
		    phraseDictionary->addWord(phrase);
	    }
	    delete d4;
	}
    }

    if (debug) {
	cout << d1->getWordCount() << consts::eol;
	cout << d2->getWordCount() << consts::eol;
	cout << d3->getWordCount() << consts::eol;
	cout << phraseDictionary->getWordCount() << consts::eol;
    }

    //
    // Now for the real guesses
    //

    analytics->setState(GuessAnalytics::PHRASEGUESS);
    DictSizeConstraint phraseSizeConstraint(phraseLength);
    Dictionary *phrases
       = phraseDictionary->getSubDictionary(phraseSizeConstraint);

    bool found = false;
    int count4 = 0;
    string phraseguess;
    int attempt = 0;
    while ((found == false) && (phrases->getWordCount() > 1)) {
	analytics->addDictSize(4, count4, phrases->getWordCount());
	if ((attempt == 0) && (phrases->getWordCount() > 50)) {
	    phraseguess = phrases->getGuessWord(1);
	} else {
	    phraseguess = phrases->getGuessWord(2);
	}
	count4++;
	p->match(phraseguess, pos, chars);
	guessHistory.push_back(new GuessHistoryElement(w, pos, chars));
	if (pos == phraseguess.length()) {
	    found = true;
	} else {
	    Dictionary *tmp;
	    MasterMindConstraint mm(phraseguess, chars, pos);
	    tmp = phrases->getSubDictionary(mm);
	    delete phrases;
	    phrases = tmp;
	}
    }
    if (found == false) {
	if (phrases->getWordCount() == 1) {
	    phraseguess = phrases->getWord(0);
	} else {
	    assert(false);
	}
    }

    // We are now done!  Cleanup!
    delete d0;
    delete d1;
    delete d2;
    delete d3;
    delete phrases;
    delete phraseDictionary;
    delete tpg;

    utils::clearGuessHistory(guessHistory);

    return phraseguess;
}

string
Mastermind::appendTestPhrase(string word, TestPatternGenerator *tpg) {
    string rc = word;
    int l = rc.length();
    rc.append(phraseLength - l, consts::zpc);
    string tc = tpg->getNextTestCombo();
    for (int i=0; i< phraseLength; ++i) {
	rc.append(tc);
    }
    return rc;
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
void
Mastermind::findPhraseLength(GuessHistory &guessHistory,
    TestPatternGenerator *tpg) {
    string lengthTestString;

    for (int i=0; i<dictFreq.length(); ++i) {
	lengthTestString.append((3*maxLength), dictFreq[i]);
    }
    // The 2 spaces as well
    lengthTestString.append(2, consts::spc);

    int pos, chars;
    p->match(lengthTestString, pos, chars);

    // Number of characters that match is the phrase length.
    // This includes the spaces between the words
    phraseLength = chars;

    // The count of the most frequent character is 'pos'.
    // we depend on the guess history and test gen to take it into account;
    guessHistory.push_back(new GuessHistoryElement(lengthTestString, pos, chars));
    tpg->setCharCount(0, dictFreq.substr(0,1), pos);
}

// Use guess history, and known information so far from the first
// 2 words to constrain the dictionary.
// See details in the comments in the code!
// Build a vector of constraints, to be applied.
DictConstraints *
Mastermind::createDictConstraints(const string &base_guess,
		      const GuessHistory& hist) {
    // TODO: Should be const??

    DictConstraints *rc = new DictConstraints();

    // Get char counts for the now known information (words 1 and 2)
    charCounts base_counts;
    base_counts.addToCount(base_guess);

    // For each previous guess
    for (GuessHistory::const_iterator it = hist.cbegin();
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

    return rc;
}
