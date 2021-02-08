/**
 * File: rss-index.cc
 * ------------------
 * Presents the implementation of the RSSIndex class, which is
 * little more than a glorified map.
 */

#include "rss-index.h"

#include <algorithm>

using namespace std;

void RSSIndex::add(const Article& article, const vector<string>& words) {
}

static const vector<pair<Article, int> > emptyResult;
vector<pair<Article, int> > RSSIndex::getMatchingArticles(const string& word) const {
  
}
