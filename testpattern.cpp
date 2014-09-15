#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <assert.h>

#include "utils.h"
#include "testpattern.h"

using namespace std;

TestPatternGenerator::TestPatternGenerator(Dictionary *d) {
    this->d = d;
    dictFreq = d->getCharsByFrequency();
    phraseLength = 0;
    debug = false;
    initialize();
}

void
TestPatternGenerator::initialize() {
    charCounts freq = DictUtils::getDictCharDistribution(d);

    // Get individual character counts, establish quartiles
    int freqTotal = 0;
    vector<pair<char, int> > freqDist;
    for (int i=0; i<dictFreq.length(); ++i) {
	char c = dictFreq[i];
	freqTotal += freq[utils::ctoi(c)];
	freqDist.push_back(make_pair(c, freqTotal));
    }

    int q1 = -1;
    int q2 = -1;
    int q3 = -1;
    for (int i=0; i<dictFreq.length(); ++i) {
	if ((q1 == -1) && (freqDist[i].second > freqTotal/4)) {
	    q1 = i;
	}
	if ((q2 == -1) && (freqDist[i].second > freqTotal/2)) {
	    q2 = i;
	}
	if ((q3 == -1) && (freqDist[i].second > freqTotal*3/4)) {
	    q3 = i;
	}
    }
    assert(q1 >= 0);
    assert(q2 > 0);
    assert(q3 > 0);

    if (q2 == q1) {
	q2 += 1;
	q3 += 2;
    } else if (q3 == q2) {
	q3 += 1;
    }

    assert(q2 > q1);
    assert(q3 > q2);

    for (int i=0; i< 8; ++i) {
	testCases[i].clear();
    }

    int uniqChars = dictFreq.length();

    int testCount = 0;
    testCases[testCount] = dictFreq;
    testCount++;

    // Alternate Characters
    for (int i=0; i<uniqChars; i+= 2) {
	testCases[testCount].append(1, dictFreq[i]);
    }
    testCount++;

    // Characters 0,1  4,5  8,9
    for (int i=0; i<dictFreq.size(); i+= 4) {
	testCases[testCount].append(1, dictFreq[i]);
	if (i+1 < dictFreq.size())
	    testCases[testCount].append(1, dictFreq[i+1]);
    }
    testCount++;

    // Intersection of 2 previous tests
    for (int i=0; i<dictFreq.size(); ++i) {
	char c = dictFreq[i];
	int m = testCount-1; int n=testCount-2;
	if ((testCases[m].find(c) != -1) && (testCases[n].find(c) != -1)) {
	    testCases[testCount].append(1, c);
	}
    }
    testCount++;

    // skip 1
    testCount++;

    // Top half
    for (int i=0; i<q2; ++i) {
	testCases[testCount].append(1, dictFreq[i]);
    }
    testCount++;

    // Middle
    for (int i=q1; i<q3; ++i) {
	testCases[testCount].append(1, dictFreq[i]);
    }
    testCount++;

    // Intersection of 2 previous tests
    for (int i=0; i<dictFreq.size(); ++i) {
	char c = dictFreq[i];
	int m = testCount-1; int n=testCount-2;
	if ((testCases[m].find(c) != -1) && (testCases[n].find(c) != -1)) {
	    testCases[testCount].append(1, c);
	}
    }
    testCount++;
}

TestPatternGenerator::~TestPatternGenerator() {
}

void
TestPatternGenerator::debugprint() const {
    if (zeroCountChars.uniqCounts() > 0) {
	cout << zeroCountChars.getChars() << consts::tab << 0 << consts::eol;
    }
    for(vector<pair<string,int> >::const_iterator it = alphaCounts.cbegin();
	it != alphaCounts.cend(); ++it) {
	cout << it->first << consts::tab << it->second << consts::eol;
    }
}

// Once the test result is returned, pass the information back to the
// TestPatternGenerator, so that all information can be accounted for,
// while generating more test strings, as well as while constraining the
// dictionary
int
TestPatternGenerator::setCharCount(int counter, string combo, int count) {
    bool zeroFlag = false;  // Anything discovered with zero count?
    counter++;
    vector<pair<string, int> > queue;;

    // All incoming requests are compared with existing information.  This
    // results in the creation of additional information that may have to be
    // tracked, and so on recursively.  To break that recursion, use a queue
    // Add the first request to the queue, and place all subsequent information
    // that has to be added to this queue.
    // The method returns when the queue is empty.
    queue.push_back(make_pair(combo, count));
    if (debug) {
	cout << "setCharCount 1: " << combo << ", " << count << consts::eol;
    }

    while (!queue.empty()) {
	// Remove one element from the queue
	pair<string,int> last_pair = queue.back();
	queue.pop_back();
	if (debug) {
	    cout << "scc head of q: " << last_pair.first << ", " << last_pair.second << consts::eol;
	}

	// If the string has 0 count in the guess to be matched, then all
	// the characters in the string will have a 0 count.
	// Remove them from all the combinations we are tracking!
	if (last_pair.second == 0) {
	    zeroCountChars.addToCount(last_pair.first);
	    for (int i=0; i<28; ++i)
		if (zeroCountChars[i] > 1)
		    zeroCountChars[i] = 1;

	    // Go through all the strings we are tracking, remove the
	    // characters known to be not in the secret phrase.
	    // Reprocess the entire set by pushing it to the queue, and 
	    // clearing alphaCounts!
	    for(vector<pair<string,int> >::iterator it = alphaCounts.begin();
		it != alphaCounts.end(); ++it) {

		charCounts prevCount;;
		prevCount.addToCount(it->first);
		prevCount.removeFromCount(zeroCountChars);
		it->first = prevCount.getChars();
		queue.push_back(make_pair(prevCount.getChars(), it->second));
	    }
	    alphaCounts.clear();

	} else {
	    charCounts comboCount;
	    comboCount.addToCount(last_pair.first);
	    last_pair.first = comboCount.getChars();	
	    // this makes it alphabetical order, not freq order

	    bool flag = true;

	    // Walk through all known character groups - count information.
	    // if a duplicate test is found, ignore this.
	    for(vector<pair<string,int> >::iterator it = alphaCounts.begin();
		((flag == true) && it != alphaCounts.end()); ++it) {
		charCounts prevCount;;
		prevCount.addToCount(it->first);

		if (prevCount.isEqual(comboCount)) {
		    // combo being considered is a repeat
		    flag = false;	// Ignore this
		}
	    }

	    // Walk through all known character groups - count information.
	    // if a case is found which is a superset of this one, find
	    // the difference and add it to the queue.
	    // For instance, if we already know "abcd" has 3 characters, and
	    // then we find that "ab" has 2 characters, we know that "cd" has 1
	    for(vector<pair<string,int> >::iterator it = alphaCounts.begin();
		((flag == true) && it != alphaCounts.end()); ++it) {
		charCounts prevCount;;
		prevCount.addToCount(it->first);

		if (prevCount.isSubSet(comboCount)) {
		    // combo being considered is a subset of a previous test string
		    charCounts deltaCount = prevCount;
		    deltaCount.removeFromCount(comboCount);

		    // Take the difference, and add it to the queue to look at
		    string new_combo = deltaCount.getChars();
		    int new_count = it->second - last_pair.second;
		    queue.push_back(make_pair(new_combo, new_count));
		    if (debug) {
			cout << "scc adding to q (set): " << queue.back().first << ", " << queue.back().second << consts::eol;
		    }
		}

		// Vice versa.
		// ex. if we know "ab" has 2 characters, and now we know
		// "abcd" has 3, then we know that "cd" has 1
		if (comboCount.isSubSet(prevCount)) {
		    // combo being considered is a subset of a previous test string
		    charCounts deltaCount = comboCount;
		    deltaCount.removeFromCount(prevCount);

		    // Take the difference, and add it to the queue to look at
		    string new_combo = deltaCount.getChars();
		    int new_count = last_pair.second - it->second;
		    queue.push_back(make_pair(new_combo, new_count));
		    if (debug) {
			cout << "scc adding to q (set 2): " << queue.back().first << ", " << queue.back().second << consts::eol;
		    }
		}
	    }

	    if (flag && (last_pair.second > 0)) {
		alphaCounts.push_back(last_pair);
	    }
	}
    }

    return counter;
}

string
TestPatternGenerator::getNextTestCombo() {
    return getTestCombo(++lastCounter);
}

//------------------------------------------------------------------------------
// Here is the thinking:
// 0: Entire Dictionary (26 chars)
// 1: Even entries in dictionary (13 chars)
//    When we get the counts back, we now know the counts of even chars as well
//    (setcounts(0) - setcounts(1)
// 2: (1,2,  5,6,  9,10 ...) chars in dictionary
//    When we get the counts back, we now knoe the counts of the other half as
//    well.
//    If we visualize a 2 X 2 square, we now have the sums of the rows, and cols
// 3: Intersect the sets returned by 1&2.  This gives is one of the squares in
//    visualization above, which gives us all 4 quadrants
// 4: Top 13 chars, this also gives us the bottom 13 chars
// 5: From step 3, identify the quadrant with lowest count, intersect with ~4
//    This should give us smaller sets, ...
// 6: From step 3, identify the quadrant with highest count, intersect with 4.
// 7: Hopefully, we are now set!  Fall back to highest freq char, whose count
//    we do now know.
//------------------------------------------------------------------------------

string
TestPatternGenerator::getTestCombo(int counter) {
    lastCounter = counter;
    string rc;
    switch(counter) {
	case 0:		// entire dict
	case 1:		// one half
	case 2:		// a diff half
	case 3:		// intersection of prev 2
	    return testCases[counter];
	    break;
	case 4:
	    {
		int minLen = alphaCounts.begin()->second+1;
		string minCombo;
		for(vector<pair<string,int> >::iterator it = alphaCounts.begin();
		    it != alphaCounts.end(); ++it) {
		    if ((it->second > 0) && (minLen > it->second) &&
			(it->first.length() > 1)) {
			minLen = it->second;
			minCombo = it->first;
		    }
		}
		return minCombo.substr(0, (minCombo.length()/2));
	    }
	    break;
	case 5:
	case 6:
	case 7:
	    return testCases[counter];

	case 8:
	    rc.append(dictFreq.substr(5,1))
	      .append(dictFreq.substr(10,1))
	      .append(dictFreq.substr(15,1))
	      .append(dictFreq.substr(20,1))
	      .append(dictFreq.substr(25,1));
	    return rc;
	case 9:
	    rc.append(dictFreq.substr(4,1))
	      .append(dictFreq.substr(9,1))
	      .append(dictFreq.substr(14,1))
	      .append(dictFreq.substr(19,1))
	      .append(dictFreq.substr(24,1));
	    return rc;
	case 10:
	    rc.append(dictFreq.substr(3,1))
	      .append(dictFreq.substr(8,1))
	      .append(dictFreq.substr(13,1))
	      .append(dictFreq.substr(18,1))
	      .append(dictFreq.substr(23,1));
	    return rc;
	case 11:
	    rc.append(dictFreq.substr(2,1))
	      .append(dictFreq.substr(7,1))
	      .append(dictFreq.substr(12,1))
	      .append(dictFreq.substr(17,1))
	      .append(dictFreq.substr(22,1));
	    return rc;
	case 12: rc = dictFreq.substr(1,1); return rc;
	case 13: rc = dictFreq.substr(6,1); return rc;
	case 14: rc = dictFreq.substr(11,1); return rc;
	case 15: rc = dictFreq.substr(16,1); return rc;
	case 16: rc = dictFreq.substr(21,1); return rc;
	default:
	    return rc;
    }
    return rc;
}

DictConstraints *
TestPatternGenerator::getWordConstraints() const {
    DictConstraints *rc = new DictConstraints();

    if (zeroCountChars.uniqCounts() > 0) {

	DictionaryConstraint *dc = new CharMatchWordConstraint(zeroCountChars, 0);
	rc->push_back(dc);
    }

    for(vector<pair<string,int> >::const_iterator it = alphaCounts.cbegin();
	it != alphaCounts.cend(); ++it) {
	int matchCount = it->second;
	string chars = it->first;
	string temp;
	for (int i=0; i<matchCount; ++i) {
	    temp.append(chars);
	}
	charCounts counts;
	counts.addToCount(temp);
	DictionaryConstraint *dc = new CharMatchWordConstraint(counts, matchCount);
	rc->push_back(dc);
    }
    return rc;
}

//------------------------------------------------------------------------------

vector<int>*
SpaceTestBuilder::getTestVector(const unordered_set<int> &node_set, int seq) {
    vector<int>* rc = new vector<int>();
    int i=1;
    for (unordered_set<int>::const_iterator it=node_set.cbegin();
         it != node_set.cend(); ++it) {
	if ((seq&i) != 0) {
	    rc->push_back(*it);
	}
	i = i<<1;
    }
    return rc;
}

long
SpaceTestBuilder::scorefn(int count0, int count1, int count2) {
    long score = 0;
    score += count0 * (count1 + count2);
    score += count1 * (count0 + count2);
    score += count2 * (count0 + count1);

    return score;
}

void
SpaceTestBuilder::match(const vector<int> &nodes,
      const vector<pair<int, int> > &pairs,
      int& count0, int& count1, int& count2) {

    count0 = 0; count1 = 0; count2 = 0;
    unordered_set<int> matched_nodes;
    for (vector<pair<int, int> >::const_iterator it = pairs.cbegin();
	 it != pairs.cend(); ++it) {
	matched_nodes.clear();
	for (vector<int>::const_iterator node_iterator = nodes.cbegin();
	     node_iterator != nodes.cend(); ++node_iterator) {
	    int n = *node_iterator;
	    if ((n == it->first) || (n == it->second)) {
		matched_nodes.insert(n);
	    }
	}
	switch(matched_nodes.size()) {
	    case 0: count0++; break;
	    case 1: count1++; break;
	    case 2: count2++; break;
	}
    }
    return;
}


vector<int>*
SpaceTestBuilder::getBestTestVector(const vector<pair<int, int> > &pairs) {
    unordered_set<int> node_set;
    for (vector<pair<int, int> >::const_iterator it = pairs.cbegin();
	 it != pairs.cend(); ++it) {
	node_set.insert(it->first);
	node_set.insert(it->second);
    }
    int node_count = node_set.size();
    int pair_count = pairs.size();

    int max_seq = 1<<(node_count);
    int best_score = -1;
    vector<int>* rc = NULL;

    for (int seq=0; seq < max_seq; ++seq) {
	int count0, count1, count2;
	vector<int>* testVector = getTestVector(node_set, seq);
	match(*testVector, pairs, count0, count1, count2);
	long score = scorefn(count0, count1, count2);
	if (score > best_score) {
	    if (rc != NULL)
		delete rc;
	    rc = testVector;
	    best_score = score;
	} else {
	    if (testVector != NULL)
		delete testVector;
	}
    }
    return rc;
}
