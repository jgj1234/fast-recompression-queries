#include "recompression_definitions.hpp"
#include "utils/packed_pair.hpp"
#include "lce_queries.hpp"

#include <fstream>

// #endregion
Node::Node() {

}

Node::Node(
  const c_size_t& var,
  const c_size_t& l,
  const c_size_t& r) : 
  var(var),
  l(l),
  r(r),
  parent(-1),
  indexInParent(-1),
  level(0) {

}

Node::Node(
  const c_size_t& var,
  const c_size_t& l,
  const c_size_t& r, 
  const c_size_t& parent,
  const c_size_t& indexInParent) : 
  var(var),
  l(l),
  r(r),
  parent(parent),
  indexInParent(indexInParent),
  level(0) {

}
RLSLPNonterm::RLSLPNonterm(
  const char_t& type,
  const c_size_t& first,
  const c_size_t& second) :
  type(type),
  first(first),
  second(second),
  explen(0) {

}

RLSLPNonterm::RLSLPNonterm() :
  type('0'),
  first(0),
  second(0),
  explen(0) {

}

RLSLPNonterm::RLSLPNonterm(
  const char_t& type,
  const c_size_t& first,
  const c_size_t& second,
  const c_size_t& explen) : 
  type(type),
  first(first),
  second(second),
  explen(explen) {

}

uint64_t RecompressionRLSLP::ram_use() const {
    return nonterm.ram_use();
}

void RecompressionRLSLP::write_to_file(const string& filename) {
  ofstream ofs(filename, ios::binary);

  if (!ofs) {
    cerr << "Error opening file for writing: " << filename << endl;
    return;
  }

  for (len_t i = 0; i < nonterm.size(); ++i) {
    const RLSLPNonterm& rlslpNonterm = nonterm[i];

    // Write type
    ofs.write(reinterpret_cast<const char*>(&rlslpNonterm.type),
              sizeof(rlslpNonterm.type));

    // Write first
    ofs.write(reinterpret_cast<const char*>(&rlslpNonterm.first),
              sizeof(c_size_t));

    // Write second
    ofs.write(reinterpret_cast<const char*>(&rlslpNonterm.second),
              sizeof(c_size_t));

    // Write level
    ofs.write(reinterpret_cast<const char*>(&rlslpNonterm.level),
              sizeof(c_size_t));
  }

  ofs.close();
}

bool RecompressionRLSLP::read_from_file(const string& filename) {
    ifstream ifs(filename, ios::binary);

    if (!ifs) {
      cerr << "Error opening input file for reading: " << filename << endl;
      return false;
    }

    nonterm.clear();

    //cout << "Reading from file: " << filename << endl;

    while (ifs.peek() != EOF) {
      RLSLPNonterm rlslpNonterm;

      // Read type
      ifs.read(reinterpret_cast<char*>(&rlslpNonterm.type),
               sizeof(rlslpNonterm.type));

      // Read first
      ifs.read(reinterpret_cast<char*>(&rlslpNonterm.first),
               sizeof(c_size_t));

      // Read second
      ifs.read(reinterpret_cast<char*>(&rlslpNonterm.second),
               sizeof(c_size_t));

      // Read level
      ifs.read(reinterpret_cast<char*>(&rlslpNonterm.level), 
              sizeof(c_size_t));
      nonterm.push_back(rlslpNonterm);
    }

    ifs.close();

    computeExplen();
    return true;
}
c_size_t RecompressionRLSLP::computeExplen(const c_size_t i) {
    space_efficient_vector<RLSLPNonterm>& rlslp_nonterm_vec = nonterm;
    // Check if already computed
    if (rlslp_nonterm_vec[i].explen != 0) {
      return rlslp_nonterm_vec[i].explen;
    }

    switch (rlslp_nonterm_vec[i].type) {
      case '0':  // Terminal case
        return rlslp_nonterm_vec[i].explen = 1;

      case '1': {
        
        c_size_t res = rlslp_nonterm_vec[i].explen =
          computeExplen(rlslp_nonterm_vec[i].first) +
          computeExplen(rlslp_nonterm_vec[i].second);
        return res;
      }

      default:{  // Repetition case or other types
        c_size_t res = rlslp_nonterm_vec[i].explen = 
          rlslp_nonterm_vec[i].second *
          computeExplen(rlslp_nonterm_vec[i].first);
        return res;
      }
    }

    // In case of unexpected type
    return 0; 
}

void RecompressionRLSLP::computeExplen() {
  for (len_t i = nonterm.size() - 1; i >= 1; --i) {
    computeExplen(i);
  }
}

void RecompressionRLSLP::initialize_nodes(
  c_size_t node, 
  const c_size_t& i,
  c_size_t left,
  c_size_t right,
  stack<Node>& ancestors,
  const space_efficient_vector<RLSLPNonterm>& grammar,
  Node& v) {
    if (left > right || i < left || i > right)
      return;
    const RLSLPNonterm& nt = grammar[node];
    if (left == i) {
      v = Node(node, left, right + 1);
      return;
    }

    ancestors.push(Node(node, left, right + 1));

    if (nt.type == '0') {
      return;
    } else if (nt.type == '1') {
      const RLSLPNonterm& left_nt = grammar[nt.first];
      const RLSLPNonterm& right_nt = grammar[nt.second];

      initialize_nodes(
        nt.first, i, left, left + left_nt.explen - 1, ancestors, grammar, v);
      initialize_nodes(
        nt.second, i, left + left_nt.explen, right, ancestors, grammar, v);

    } else {
      const RLSLPNonterm& child_nt = grammar[nt.first];
      c_size_t child_explen = child_nt.explen;

      c_size_t newLeft =
        left + ((i - left) / child_explen) * child_explen;
      c_size_t newRight =
        left + ((i - left) / child_explen) * child_explen + child_explen - 1;

      initialize_nodes(nt.first, i, newLeft, newRight, ancestors, grammar, v);
    }

    return;
}
/*void RecompressionRLSLP::constructTrees(){
  c_size_t n = nonterm.size();
  c_size_t totRounds = nonterm[n - 1].level;
  space_efficient_vector<c_size_t> leftBuckets(n), rightBuckets(n);
  BitVector lmask(n, totRounds + 2), rmask(n, totRounds + 2);
  for (c_size_t i = 0; i < n; i++){
    const RLSLPNonterm& nt = nonterm[i];
    if (nt.type == '1'){
      leftBuckets[nt.first]++;
      lmask.setEntry(nt.first, i);
      lmask.setBit(i, nt.level);
      rightBuckets[nt.second]++;
      rmask.setEntry(nt.second, i);
      rmask.setBit(i, nt.level);
    }
    else if (nt.type == '2'){
      leftBuckets[nt.first]++;
      lmask.setEntry(nt.first, i);
      lmask.setBit(i, nt.level);
      rightBuckets[nt.first]++;
      rmask.setEntry(nt.first, i);
      rmask.setBit(i, nt.level);
    }
    else{
      //terminals which are introduced at level 0
      lmask.setBit(i, 0);
      rmask.setBit(i, 0);
    }
  }
  space_efficient_vector_2D<c_size_t> leftg(leftBuckets), rightg(rightBuckets);
  space_efficient_vector<c_size_t> leftIndex(n), rightIndex(n);
  for (c_size_t i = 0; i < n; i++){
    const RLSLPNonterm& nt = nonterm[i];
    if (nt.type == '1'){
      leftg.push_back(nt.first, i, leftIndex);
      rightg.push_back(nt.second, i, rightIndex);
    }
    else if (nt.type == '2'){
      leftg.push_back(nt.first, i, leftIndex);
      rightg.push_back(nt.first, i, rightIndex);
    }
  }
  leftMT = MacroMicroTree(n, leftg);
  leftMT.levelMask = lmask;
  rightMT = MacroMicroTree(n, rightg);
  rightMT.levelMask = rmask;
}*/
void RecompressionRLSLP::initialize_firstNodes(
  c_size_t node, 
  c_size_t left,
  c_size_t right,
  const space_efficient_vector<RLSLPNonterm>& grammar, 
  space_efficient_vector<Node>& firstNodes
){

    const RLSLPNonterm& nt = grammar[node];
    Node& curr_node = firstNodes[node];
    if (curr_node.var != -1){
      return;
    }
    curr_node = Node(node, left, right + 1);
    if (nt.type == '0'){
      return;
    }
    else if (nt.type == '1'){
      const RLSLPNonterm& left_nt = grammar[nt.first];
      const RLSLPNonterm& right_nt = grammar[nt.second];
      initialize_firstNodes(nt.first, left, left + left_nt.explen - 1, grammar, firstNodes);
      initialize_firstNodes(nt.second, left + left_nt.explen, right, grammar, firstNodes);
    }
    else{
      const RLSLPNonterm& child_nt = grammar[nt.first];
      c_size_t child_explen = child_nt.explen;
      
      initialize_firstNodes(nt.first, left, left + child_explen - 1, grammar, firstNodes);
    }
}
void RecompressionRLSLP::initStructures(){
    c_size_t nonterm_num = nonterm.size();
    c_size_t totRounds = nonterm[nonterm_num - 1].level;
    pCompMask = BitVector(nonterm_num, totRounds + 2);
    bCompMask = BitVector(nonterm_num, totRounds + 2);
    for (c_size_t i = 0; i < nonterm_num; i++){
      const RLSLPNonterm& nt = nonterm[i];
      if (nt.type == '1'){
        c_size_t lev = nt.level;
        pCompMask.setBit(nt.first, (lev - 1) / 2);
      }
      else if (nt.type == '2'){
        c_size_t lev = nt.level;
        bCompMask.setBit(nt.first, (lev - 1) / 2);
      }
    }
    firstNodes.resize(nonterm_num, Node(-1, -1, -1));
    initialize_firstNodes(nonterm_num - 1, 0, nonterm.back().explen - 1, nonterm, firstNodes);
}
void RecompressionRLSLP::reverseRLSLP(c_size_t node, RecompressionRLSLP* rev_rlslp){
    if (rev_rlslp->nonterm[node].first != -1) return;
    const RLSLPNonterm& nt = nonterm[node];
    rev_rlslp->nonterm[node].explen = nt.explen;
    rev_rlslp->nonterm[node].type = nt.type;
    rev_rlslp->nonterm[node].level = nt.level;
    if (nt.type == '0'){
        rev_rlslp->nonterm[node].first = nt.first;
        rev_rlslp->nonterm[node].second = nt.second;
    }
    else if (nt.type == '1'){
        reverseRLSLP(nt.first, rev_rlslp);
        reverseRLSLP(nt.second, rev_rlslp);
        rev_rlslp->nonterm[node].first = nt.second;
        rev_rlslp->nonterm[node].second = nt.first;
    }
    else{
        reverseRLSLP(nt.first, rev_rlslp);
        rev_rlslp->nonterm[node].first = nt.first;
        rev_rlslp->nonterm[node].second = nt.second;
    }
}
Node RecompressionRLSLP::getLeftMostChild(
  Node v, const space_efficient_vector<RLSLPNonterm>& grammar) {
  char type = grammar[v.var].type;

  const RLSLPNonterm& nt = grammar[v.var];

  c_size_t left = v.l;
  c_size_t right = v.r - 1;

  if(type == '1') {
    const RLSLPNonterm& left_nt = grammar[nt.first];
    const RLSLPNonterm& right_nt = grammar[nt.second];
    return Node(nt.first, left, left + left_nt.explen);
  } else if(type == '2') {
    const RLSLPNonterm& child_nt = grammar[nt.first];
    c_size_t child_explen = child_nt.explen;
    return Node(nt.first, left, left + child_explen);
  }

  return Node();
}

c_size_t RecompressionRLSLP::getChildCount(
  const Node& parent, const space_efficient_vector<RLSLPNonterm>& grammar) {
  const RLSLPNonterm& nt = grammar[parent.var];
  switch (nt.type) {
    case '0': return 1;
    case '1': return 2;
    default: return nt.second;
  }
}

c_size_t RecompressionRLSLP::getChildIndex(
  const Node& parent,
  const Node& v,
  const space_efficient_vector<RLSLPNonterm>& grammar) {
  const RLSLPNonterm& nt = grammar[parent.var];

  switch (nt.type) {
    case '0': 
      return 1;

    case '1': 
      return (parent.l == v.l) ? 1 : 2;

    default:
      c_size_t span = v.r - v.l;
      return (v.l - parent.l) / span + 1;
  }
}

Node RecompressionRLSLP::getKthSibling(
  const Node& parent, const Node& v, c_size_t k) {
  c_size_t segmentLength = v.r - v.l;
  c_size_t newLeft = parent.l + (k - 1) * segmentLength;
  c_size_t newRight = newLeft + segmentLength;

  return Node(v.var, newLeft, newRight);
}

Node RecompressionRLSLP::replaceWithHighestStartingAtPosition(
  const Node& v,
  stack<Node>& ancestors,
  const space_efficient_vector<RLSLPNonterm>& grammar) {
    Node child = v;
    while (!ancestors.empty() && ancestors.top().r == v.r) {
      child = ancestors.top();
      ancestors.pop();
    }

    if (ancestors.empty()) {
      cout << "ANCESTORS is EMPTY -- Exiting" << endl;
      exit(1);
      return Node(); // Or some other appropriate action
    }

    Node ancestor = ancestors.top();
    const RLSLPNonterm& nt = grammar[ancestor.var];

    if (nt.type == '1') {
        return Node(nt.second, child.r, ancestor.r);
    } else {
        c_size_t childIndex = getChildIndex(ancestor, child, grammar);
        return getKthSibling(ancestor, child, childIndex + 1);
    }

    return Node();
}

c_size_t RecompressionRLSLP::LCE(
  Node& v1, 
  Node& v2, 
  c_size_t i, 
  stack<Node>& v1_ancestors,
  stack<Node>& v2_ancestors,
  const space_efficient_vector<RLSLPNonterm>& grammar) {
  c_size_t exp_len_v1 = v1.r - v1.l;
  c_size_t exp_len_v2 = v2.r - v2.l;
  if (exp_len_v1 == 1 and exp_len_v1 == exp_len_v2 and v1.var != v2.var) {
    return v1.l - i;
  } else if (exp_len_v1 > exp_len_v2) {
    v1_ancestors.push(v1);
    v1 = getLeftMostChild(v1, grammar);
  } else if (exp_len_v1 < exp_len_v2) {
    v2_ancestors.push(v2);
    v2 = getLeftMostChild(v2, grammar);
  } else if (v1.var != v2.var) {
    v2_ancestors.push(v2);
    v1_ancestors.push(v1);
    v1 = getLeftMostChild(v1, grammar);
    v2 = getLeftMostChild(v2, grammar);
  } else {
    Node v1_parent = v1_ancestors.top();
    Node v2_parent = v2_ancestors.top();

    c_size_t j1 = getChildIndex(v1_parent, v1, grammar);
    c_size_t j2 = getChildIndex(v2_parent, v2, grammar);


    c_size_t d1 = getChildCount(v1_parent, grammar);
    c_size_t d2 = getChildCount(v2_parent, grammar);

    c_size_t lambda = min(d1 - j1, d2 - j2);

    if (lambda <= 1) {
      v1 = replaceWithHighestStartingAtPosition(v1, v1_ancestors, grammar);

      if (v2.r >= grammar.back().explen) {
          return v1.l - i;
      }

      v2 = replaceWithHighestStartingAtPosition(v2, v2_ancestors, grammar);
    } else {
      v1 = getKthSibling(v1_parent, v1, j1 + lambda);
      v2 = getKthSibling(v2_parent, v2, j2 + lambda);
    }
  }

  return LCE(v1, v2, i, v1_ancestors, v2_ancestors, grammar);
}

c_size_t RecompressionRLSLP::lce(c_size_t i, c_size_t j) {
    if (i == j) {
      return nonterm.back().explen - i;
    }
    if (i > j) swap(i, j);
    Node v1, v2;
    stack<Node> v1_ancestors, v2_ancestors;
    // v1_ancestors.push(Node(grammar.size()-1, 0, 33));
    // v2_ancestors.push(Node(grammar.size()-1, 0, 33));
    initialize_nodes(
      nonterm.size() - 1, i, 0, nonterm.back().explen - 1, v1_ancestors,
      nonterm, v1);
    initialize_nodes(
      nonterm.size() - 1, j, 0, nonterm.back().explen - 1, v2_ancestors,
      nonterm, v2);
    return LCE(v1, v2, i, v1_ancestors, v2_ancestors, nonterm);
}

Node RecompressionRLSLP::getPosition(c_size_t node, c_size_t par, c_size_t idxInPar, c_size_t left, c_size_t right, c_size_t pos, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar){
  Node curr_node(node, left, right, par, idxInPar); 
  curr_node.level = grammar[node].level;
  ancestors.push(curr_node);
  //pushToStack(curr_node, ancestors);
  if (left == right){
    return curr_node;
  }
  const RLSLPNonterm& nt = grammar[node];
  c_size_t leftNode = nt.first;
  c_size_t leftLength = grammar[leftNode].explen;
  if (nt.type == '1'){
    if (left + leftLength > pos){
      return getPosition(leftNode, node, 0, left, left + leftLength - 1, pos, ancestors, grammar);
    }
    else{
      return getPosition(nt.second, node, -1, left + leftLength, right,  pos, ancestors, grammar);
    }
  }
  c_size_t skipTimes = (pos - left) / leftLength;
  c_size_t skip = left + leftLength * ((pos - left) / leftLength);
  return getPosition(leftNode, node, skipTimes == nt.second - 1 ? -1 : skipTimes, skip, skip + leftLength - 1, pos, ancestors, grammar);
}
/*packed_pair<c_size_t, c_size_t> RecompressionRLSLP::First(c_size_t var, c_size_t level){
  c_size_t k = leftMT.levelMask.getSuffCount(var, level + 1);
  return packed_pair<c_size_t, c_size_t>(leftMT.level_ancestor(var, leftMT.depths[var] - k), leftMT.level_ancestor(var, leftMT.depths[var] - k + 1));
}
packed_pair<c_size_t, c_size_t> RecompressionRLSLP::Last(c_size_t var, c_size_t level){
  c_size_t k = rightMT.levelMask.getSuffCount(var, level + 1);
  return packed_pair<c_size_t, c_size_t>(rightMT.level_ancestor(var, rightMT.depths[var] - k), rightMT.level_ancestor(var, rightMT.depths[var] - k + 1));
}*/
bool RecompressionRLSLP::sameParent(Node& x, Node& y, const space_efficient_vector<RLSLPNonterm>& grammar){
  if (grammar[x.parent].level - 1 > x.level|| grammar[y.parent].level - 1 > y.level) return false;
  if (x.parent != y.parent) return false;
  const RLSLPNonterm& par = grammar[x.parent];
  if (par.type == '1'){
    if (y.r != x.r + grammar[y.var].explen) return false;
    return x.var == par.first;
  }
  if (x.indexInParent == -1) return false;
  c_size_t length = grammar[x.var].explen;
  c_size_t diff = (y.indexInParent == -1 ? par.second - 1 : y.indexInParent) - x.indexInParent;
  return x.r + length * diff == y.r;
}
c_size_t RecompressionRLSLP::getSymbol(c_size_t node, c_size_t left, c_size_t right, c_size_t pos, const space_efficient_vector<RLSLPNonterm>& grammar){
  if (left == right) return node;
  const RLSLPNonterm& nt = grammar[node];
  c_size_t leftNode = nt.first;
  c_size_t leftLength = grammar[leftNode].explen;
  if (nt.type == '1'){
    if (left + leftLength > pos){
      return getSymbol(nt.first, left, left + leftLength - 1, pos, grammar);
    }
    else{
      return getSymbol(nt.second, left + leftLength, right, pos, grammar);
    }
  }
  c_size_t skipTimes = (pos - left) / leftLength;
  c_size_t skip = left + leftLength * ((pos - left) / leftLength);
  return getSymbol(leftNode, skip, skip + leftLength - 1, pos, grammar);
}
c_size_t RecompressionRLSLP::getSymbol(c_size_t pos){
  return getSymbol(nonterm.size() - 1, 0, nonterm.back().explen - 1, pos, nonterm);
}
char RecompressionRLSLP::getCharacter(c_size_t pos){
  return static_cast<char> (nonterm[getSymbol(pos)].first);
}
void RecompressionRLSLP::pushToStack(Node& x, stack<Node>& ancestors){
    if (!ancestors.empty() && x.indexInParent <= 0 && x.indexInParent == ancestors.top().indexInParent) ancestors.pop();
    ancestors.push(x);
}
// Given a node `node` covering [node.l, node.r] at recompression level `node.level` in the compressed parse tree
// represented by `grammar`, with its compressed path to the root stored in `ancestors`, updates `ancestors` so that
// it contains the path from the root down to `node`'s parent.
void RecompressionRLSLP::updateStack(Node& node, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar){
  /*Node parent;
  if (ancestors.top().var == node.var){
    ancestors.pop();
    if (ancestors.top().var == node.parent){
      return;
    }
  }
  parent.var = node.parent;
  const RLSLPNonterm& nt = grammar[parent.var];
  parent.level = nt.level;
  if (nt.type == '1'){
    if (node.var == nt.first){
      parent.l = node.l;
      parent.r = node.r + grammar[nt.second].explen;
    }
    else{
      parent.l = node.l - grammar[nt.first].explen;
      parent.r = node.r;
    }
  }
  else{
    c_size_t length = grammar[node.var].explen;
    c_size_t times = nt.second;
    parent.l = node.l - length * (node.indexInParent == -1 ? times - 1: node.indexInParent);
    parent.r = node.r + length * (node.indexInParent == -1 ? 0: times - 1 - node.indexInParent);
  }
  const RLSLPNonterm& stackNT = grammar[ancestors.top().var];
  parent.indexInParent = node.indexInParent;
  c_size_t parLevel = grammar[parent.var].level;
  if (parent.indexInParent == 0){
    c_size_t k = leftMT.levelMask.getSuffCount(ancestors.top().var, parent.level + 1);
    parent.parent = leftMT.level_ancestor(ancestors.top().var, leftMT.depths[ancestors.top().var] - (k - 1));
  }
  else{
    c_size_t k = rightMT.levelMask.getSuffCount(ancestors.top().var, parent.level + 1);
    parent.parent = rightMT.level_ancestor(ancestors.top().var, rightMT.depths[ancestors.top().var] - (k - 1));
  }
  pushToStack(parent, ancestors);*/
  ancestors.pop();
}
Node RecompressionRLSLP::constructParent(Node& node, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar){
  if (node.level < grammar[node.parent].level - 1){
    node.level++;
    return node;
  }
  updateStack(node, ancestors, grammar);
  return ancestors.top();
}
c_size_t RecompressionRLSLP::rext(Node& x, const space_efficient_vector<RLSLPNonterm> &grammar){
  const RLSLPNonterm& parNT = grammar[x.parent];
  if (parNT.type == '1'){
    return 0;
  }
  else{
    if (x.indexInParent == -1) return 0;
    return parNT.second - 1 - x.indexInParent;
  }
}
c_size_t RecompressionRLSLP::lext(Node& x, const space_efficient_vector<RLSLPNonterm> &grammar){
  const RLSLPNonterm& parNT = grammar[x.parent];
  if (parNT.type == '1'){
    return 0;
  }
  else{
    if (x.indexInParent == -1) return parNT.second - 1;
    return x.indexInParent;
  }
}
// Given a node `x` covering [x.l, x.r] at recompression level `x.level` in the compressed
// parse tree represented by `grammar`, with its compressed path to the root stored in `ancestors`, returns the nearest
// node at the same level immediately to the left of `x`
// and updates `ancestors` to reflect the path to the returned node.
Node RecompressionRLSLP::Left(Node& x, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar){
  /*if (x.indexInParent != 0){
    const RLSLPNonterm& parNT = grammar[x.parent];
    c_size_t lev = x.level;
    x.level = parNT.level - 1;
    updateStack(x, ancestors, grammar);
    x.level = lev;
    c_size_t childVar = parNT.first;
    if (grammar[childVar].level <= lev){
      c_size_t k = parNT.type == '1' ? 1: parNT.second - 1;
      c_size_t nxtIdx = (x.indexInParent == -1 ? k - 1 : x.indexInParent - 1);
      Node res_node(childVar, x.l - grammar[childVar].explen, x.l - 1, x.parent, nxtIdx);
      res_node.level = lev;
      pushToStack(res_node, ancestors);
      return res_node;
    }
    else{
      c_size_t k = parNT.type == '1' ? 1: parNT.second - 1;
      c_size_t nxtIdx = (x.indexInParent == -1 ? k - 1 : x.indexInParent - 1);
      Node first_child(childVar, x.l - grammar[childVar].explen, x.l - 1, x.parent, nxtIdx);
      first_child.level = grammar[childVar].level;
      pushToStack(first_child, ancestors);
      auto [nxtVar, nxtPar] = Last(childVar, lev);
      Node res_node(nxtVar, x.l - grammar[nxtVar].explen, x.l - 1, nxtPar, -1);
      res_node.level = lev;
      pushToStack(res_node, ancestors);
      return res_node;
    }
  }
  ancestors.pop();
  Node lcaChild = ancestors.top();
  updateStack(lcaChild, ancestors, grammar);
  if (lcaChild.indexInParent > 0){
    lcaChild.indexInParent--;
    c_size_t length = grammar[lcaChild.var].explen;
    lcaChild.l -= length;
    lcaChild.r -= length;
  }
  else{
    if (grammar[lcaChild.parent].type == '1'){
      lcaChild.indexInParent = 0;
      lcaChild.var = grammar[lcaChild.parent].first;
      lcaChild.r = lcaChild.l - 1;
      lcaChild.l = lcaChild.l - grammar[lcaChild.var].explen;
    }
    else{
      lcaChild.indexInParent = grammar[lcaChild.parent].second - 2;
      c_size_t length = grammar[lcaChild.var].explen;
      lcaChild.l -= length;
      lcaChild.r -= length;
    }
  }
  lcaChild.level = grammar[lcaChild.var].level;
  pushToStack(lcaChild, ancestors);
  if (lcaChild.level <= x.level){
    lcaChild.level = x.level;
    return lcaChild;
  }
  auto [nodeIdx, par] = Last(lcaChild.var, x.level);
  Node res_node(nodeIdx, x.l - grammar[nodeIdx].explen, x.l - 1, par, -1);
  res_node.level = x.level;
  pushToStack(res_node, ancestors);
  return res_node;*/
  while (ancestors.top().indexInParent == 0){
    ancestors.pop();
  }
  const RLSLPNonterm& lcaNT = grammar[ancestors.top().parent];
  c_size_t sib = lcaNT.first;
  c_size_t nidx = lcaNT.type == '1' ? 0 : ancestors.top().indexInParent - 1;
  if (nidx == -2) nidx = lcaNT.second - 2;
  Node leftSibling(sib, x.l - grammar[sib].explen, x.l - 1, ancestors.top().parent, nidx);
  leftSibling.level = grammar[sib].level;
  ancestors.pop();
  ancestors.push(leftSibling);
  while (grammar[ancestors.top().var].level > x.level){
    const RLSLPNonterm& nt = grammar[ancestors.top().var];
    c_size_t rightChild = nt.type == '1' ? nt.second : nt.first;
    Node curr_node(rightChild, x.l - grammar[rightChild].explen, x.l - 1, ancestors.top().var, -1);
    curr_node.level = grammar[rightChild].level;
    ancestors.push(curr_node);
  }
  ancestors.top().level = x.level;
  return ancestors.top();
}
// Given a node `x` covering [x.l, x.r] at recompression level `x.level` in the compressed
// parse tree represented by `grammar`, with its compressed path to the root stored in `ancestors`, returns the nearest
// node at the same level immediately to the right of `x`
// and updates `ancestors` to reflect the path to the returned node.
Node RecompressionRLSLP::Right(Node& x, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar){
  /*if (x.indexInParent != -1){
    const RLSLPNonterm& parNT = grammar[x.parent];
    c_size_t lev = x.level;
    x.level = parNT.level - 1;
    updateStack(x, ancestors, grammar);
    x.level = lev;
    c_size_t childVar = parNT.type == '1' ? parNT.second : parNT.first;
    if (grammar[childVar].level <= lev){
      c_size_t k = parNT.type == '1' ? 1: parNT.second - 1;
      Node res_node(childVar, x.r + 1, x.r + grammar[childVar].explen, x.parent, x.indexInParent == k - 1 ? -1 : x.indexInParent + 1);
      res_node.level = x.level;
      pushToStack(res_node, ancestors);
      return res_node;
    }
    else{
      c_size_t k = parNT.type == '1' ? 1: parNT.second - 1;
      Node first_child(childVar, x.r + 1, x.r + grammar[childVar].explen, x.parent, x.indexInParent == k - 1 ? -1 : x.indexInParent + 1);
      first_child.level = grammar[childVar].level;
      pushToStack(first_child, ancestors);
      auto [nxtVar, nxtPar] = First(childVar, lev);
      Node res_node(nxtVar, x.r + 1, x.r + grammar[nxtVar].explen, nxtPar, 0);
      res_node.level = lev;
      pushToStack(res_node, ancestors);
      return res_node;
    }
  }
  ancestors.pop();
  Node lcaChild = ancestors.top(); 
  updateStack(lcaChild, ancestors, grammar);
  if (grammar[lcaChild.parent].type == '2'){
    lcaChild.indexInParent++;
    if (lcaChild.indexInParent == grammar[lcaChild.parent].second - 1) lcaChild.indexInParent = -1;
    c_size_t length = grammar[lcaChild.var].explen;
    lcaChild.l += length;
    lcaChild.r += length;
  }
  else{
      lcaChild.indexInParent = -1;
      lcaChild.var = grammar[lcaChild.parent].second;
      lcaChild.l = lcaChild.r + 1;
      lcaChild.r = lcaChild.r + grammar[lcaChild.var].explen;
  }
  lcaChild.level = grammar[lcaChild.var].level;
  pushToStack(lcaChild, ancestors);
  if (lcaChild.level <= x.level){
    lcaChild.level = x.level;
    return lcaChild;
  }
  auto [nodeIdx, par] = First(lcaChild.var, x.level);
  Node res_node(nodeIdx, x.r + 1, x.r + grammar[nodeIdx].explen, par, 0);
  res_node.level = x.level;
  pushToStack(res_node, ancestors);
  return res_node;*/
  while (ancestors.top().indexInParent == -1){
    ancestors.pop();
  }
  const RLSLPNonterm& lcaNT = grammar[ancestors.top().parent];
  c_size_t sib = lcaNT.type == '1' ? lcaNT.second : ancestors.top().var;
  c_size_t nidx = ancestors.top().indexInParent + 1;
  if (lcaNT.type == '1' || nidx == lcaNT.second - 1) nidx = -1;
  Node rightSibling(sib, x.r + 1, x.r + grammar[sib].explen, ancestors.top().parent, nidx);
  rightSibling.level = grammar[sib].level;
  ancestors.pop();
  ancestors.push(rightSibling);
  while (grammar[ancestors.top().var].level > x.level){
    const RLSLPNonterm& nt = grammar[ancestors.top().var];
    c_size_t leftChild = nt.first;
    Node curr_node(leftChild, x.r + 1, x.r + grammar[leftChild].explen, ancestors.top().var, 0);
    curr_node.level = grammar[leftChild].level;
    ancestors.push(curr_node);
  }
  ancestors.top().level = x.level;
  return ancestors.top();
}
space_efficient_vector<packed_pair<c_size_t, c_size_t>> RecompressionRLSLP::getPoppedSequence(c_size_t left, c_size_t right, const space_efficient_vector<RLSLPNonterm>& grammar){
    stack<Node> lstack, rstack;
    Node P = getPosition(grammar.size() - 1, -1, -2, 0, grammar.back().explen - 1, left, lstack, grammar), Q = getPosition(grammar.size() - 1 , -1, -2, 0, grammar.back().explen - 1, right, rstack, grammar);
    
    space_efficient_vector<packed_pair<c_size_t, c_size_t>> S, T;
    c_size_t height = grammar.back().level;
    for (c_size_t j = 0; j <= height && P.r < Q.l; j++){
      if (sameParent(P, Q, grammar)){
        if (grammar[Q.parent].type == '1'){
          S.push_back(packed_pair<c_size_t, c_size_t>(P.var, c_size_t(1)));
          T.push_back(packed_pair<c_size_t, c_size_t>(Q.var, c_size_t(1)));
        }
        else{
          S.push_back(packed_pair<c_size_t, c_size_t>(P.var, (Q.indexInParent == -1 ? grammar[Q.parent].second - 1 : Q.indexInParent) - P.indexInParent + 1));
        }
        break;
      }
      if (((j & 1) && (!pCompMask.getBit(P.var, j / 2))) || 
          (((j & 1) == 0) && bCompMask.getBit(P.var, j / 2))){

            c_size_t rightEq = (j & 1 ? 0 : rext(P, grammar)) + 1;
            S.push_back(packed_pair<c_size_t, c_size_t>(P.var, rightEq));
            Node pPar = constructParent(P, lstack, grammar);
            P = Right(pPar, lstack, grammar);
      }
      else{
          P = constructParent(P, lstack, grammar);
      }
      if (((j & 1) && ((pCompMask.getBit(Q.var, j / 2)))) || 
            (((j & 1) == 0) && bCompMask.getBit(Q.var, j / 2))){ //there's only one level where this variable can be involved in BComp, probably don't need BCompMask
              c_size_t leftEq = (j & 1 ? 0: lext(Q, grammar)) + 1;
              T.push_back(packed_pair<c_size_t, c_size_t>(Q.var, leftEq));
              Node qPar = constructParent(Q, rstack, grammar);
              Q = Left(qPar, rstack, grammar);
        }
        else{
            Q = constructParent(Q, rstack, grammar);
        }
    }
    if (P.l == Q.l && P.r == Q.r){
      S.push_back(packed_pair<c_size_t, c_size_t>(P.var, c_size_t(1)));
    }
    T.reverse();
    for (len_t j = 0; j < T.size(); j++) S.push_back(T[j]);
    return S;
}

space_efficient_vector<c_size_t> RecompressionRLSLP::getAnchors(c_size_t index, c_size_t length){
  space_efficient_vector<packed_pair<c_size_t, c_size_t>> poppedSequence = getPoppedSequence(index, index + length - 1, nonterm);
  len_t sz = poppedSequence.size();
  space_efficient_vector<c_size_t> anchors;
  c_size_t firstLength = nonterm[poppedSequence[0].first].explen;
  if (firstLength != length)
    anchors.push_back(firstLength);
  c_size_t pref = 0;
  for (len_t i = 0; i < sz; i++){
    auto [sig, exp] = poppedSequence[i];
    pref += nonterm[sig].explen * exp;
    if (pref != firstLength && pref != length) anchors.push_back(pref);
  }
  return anchors;
}
