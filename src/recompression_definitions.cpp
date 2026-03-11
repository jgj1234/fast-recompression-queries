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
  r(r) {

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
  indexInParent(indexInParent) {

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
void RecompressionRLSLP::init(c_size_t sz){
  firstNodes.resize(sz, Node(-1, -1, -1));  
  countNodes.resize(sz);
  countQueryStructure.resize(sz);
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
void RecompressionRLSLP::constructTrees(){
  c_size_t n = nonterm.size();
  space_efficient_vector<space_efficient_vector<c_size_t>> leftg(n), rightg(n);
  space_efficient_vector<c_size_t> lmask(n), rmask(n);
  for (c_size_t i = 0; i < n; i++){
    const RLSLPNonterm& nt = nonterm[i];
    if (nt.type == '1'){
      leftg[nt.first].push_back(i);
      lmask[i] = lmask[nt.first] | (c_size_t(1) << nt.level);
      rightg[nt.second].push_back(i);
      rmask[i] = rmask[nt.second] | (c_size_t(1) << nt.level);
    }
    else if (nt.type == '2'){
      leftg[nt.first].push_back(i);
      lmask[i] = lmask[nt.first] | (c_size_t(1) << nt.level);
      rightg[nt.first].push_back(i);
      rmask[i] = rmask[nt.first] | (c_size_t(1) << nt.level);
    }
    else{
      //terminals which are introduced at level 0
      lmask[i] = 1;
      rmask[i] = 1;
    }
  }
  leftMT = MacroMicroTree(n, leftg);
  leftMT.levelMask = move(lmask);
  rightMT = MacroMicroTree(n, rightg);
  rightMT.levelMask = move(rmask);
}
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
void RecompressionRLSLP::initialize_countNodes(
  const space_efficient_vector<RLSLPNonterm>& grammar, 
  space_efficient_vector<c_size_t>& countNodes
){
    c_size_t num_nodes = countNodes.size();
    countNodes.back() = 1; // start rule
    for (c_size_t i = num_nodes - 1; i >= 0; i--){
      const RLSLPNonterm& nt = grammar[i];
      if (nt.type == '1'){
        countNodes[nt.first] += countNodes[i];
        countNodes[nt.second] += countNodes[i];
      }
      else if (nt.type == '2'){
        countNodes[nt.first] += nt.second * countNodes[i];
      }
    }
}
void RecompressionRLSLP::initialize_CountQueryStructure(){
  for (c_size_t i = 0; i < nonterm.size(); i++){
    countQueryStructure[i].resize(rhsExpList[i].size());
    c_size_t suffixK = 0, suffixSum = 0;
    if (countQueryStructure[i].size() == 0) continue;
    for (c_size_t j = countQueryStructure[i].size() - 1; j >= 0; j--){
      countQueryStructure[i][j] = packed_pair(suffixK, suffixSum);
      suffixK += rhsExpList[i][j].first * countNodes[rhsExpList[i][j].second];
      suffixSum += countNodes[rhsExpList[i][j].second];
    }
  }
}
void RecompressionRLSLP::initStructures(){
    c_size_t nonterm_num = nonterm.size();
    pCompMask.resize(nonterm_num);
    bCompMask.resize(nonterm_num);
    rhsNodeList.resize(nonterm_num);
    rhsExpList.resize(nonterm_num);
    for (c_size_t i = 0; i < nonterm_num; i++){
      const RLSLPNonterm& nt = nonterm[i];
      rhsExpList[i].push_back(packed_pair(c_size_t(1), c_size_t(0)));

      if (nt.type == '1'){
        rhsNodeList[nt.first].push_back(i);
        rhsNodeList[nt.second].push_back(i);
        c_size_t lev = nt.level;
        pCompMask[nt.first] |= c_size_t(1) << ((lev - 1) / 2);
      }
      else if (nt.type == '2'){
        rhsNodeList[nt.first].push_back(i);
        rhsExpList[nt.first].push_back(packed_pair(nt.second, i));
        c_size_t lev = nt.level;
        bCompMask[nt.first] |= c_size_t(1) << ((lev - 1) / 2);
      }
    }
    for (c_size_t i = 0; i < nonterm_num; i++){
      rhsExpList[i].sort();
    }
    initialize_firstNodes(nonterm.size() - 1, 0, nonterm.back().explen - 1, nonterm, firstNodes);
    initialize_countNodes(nonterm, countNodes);
    initialize_CountQueryStructure();
}
c_size_t RecompressionRLSLP::queryCount(c_size_t symbol, c_size_t m){
  packed_pair qpair(m + 1, c_size_t(1));
  c_size_t idx = rhsExpList[symbol].lower_bound(qpair);
  if (idx == 0) return 0;
  --idx;
  return countQueryStructure[symbol][idx].first - m * countQueryStructure[symbol][idx].second;
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
void RecompressionRLSLP::getPotentialHooks(c_size_t node, c_size_t left, c_size_t right, c_size_t leftY, c_size_t rightY, c_size_t length, space_efficient_vector<Node>& symbols){
  if (min(rightY, right) - max(leftY, left) + 1 < length) return;
  const RLSLPNonterm& nt = nonterm[node];
  symbols.push_back(Node(node, left, right));
  if (nt.type == '0') return;
  c_size_t neededPos = leftY + length - 1;
  c_size_t leftLength = nonterm[nt.first].explen;
  if (nt.type == '1'){
    if (left + leftLength - 1 >= neededPos){
      getPotentialHooks(nt.first, left, left + leftLength - 1, leftY, rightY, length, symbols);
    }
    else{
      getPotentialHooks(nt.second, left + leftLength, right, leftY, rightY, length, symbols);
    }
  }
  else if (nt.type == '2'){
    c_size_t skipPos = left + leftLength * ((neededPos - left) / leftLength);
    getPotentialHooks(nt.first, skipPos, skipPos + leftLength - 1, leftY, rightY, length, symbols);
  }
}

space_efficient_vector<Node> RecompressionRLSLP::enumerateNodes(c_size_t symbol){
  space_efficient_vector<Node> nodeList;
  stack<packed_pair<c_size_t, Node>> reverse_path;
  reverse_path.push(packed_pair(symbol, Node(symbol, 0, nonterm[symbol].explen)));
  while (!reverse_path.empty()){
    c_size_t currSymbol = reverse_path.top().first;
    Node curr_node = reverse_path.top().second;
    reverse_path.pop();
    if (rhsNodeList[currSymbol].empty()){
      nodeList.push_back(curr_node);
    }
    else{
      for (c_size_t i = 0; i < rhsNodeList[currSymbol].size(); i++){
        c_size_t parIdx = rhsNodeList[currSymbol][i];
        const RLSLPNonterm& parNode = nonterm[parIdx];
        if (parNode.type == '1'){
          Node nxt_node = Node();
          nxt_node.var = symbol;
          if (parNode.first == currSymbol){
            nxt_node.l = curr_node.l;
            nxt_node.r = curr_node.r;
          }
          else{
            const RLSLPNonterm& sibling = nonterm[nonterm[parIdx].first];
            nxt_node.l = curr_node.l + sibling.explen;
            nxt_node.r = curr_node.r + sibling.explen;
          }
          reverse_path.push(packed_pair(parIdx, nxt_node));
        }
        else{
          c_size_t k = parNode.second;
          const RLSLPNonterm& symbolPos = nonterm[currSymbol];
          for (c_size_t i = 0; i < k; i++){
            Node nxt_node = Node(symbol, curr_node.l + i * symbolPos.explen, curr_node.r + i * symbolPos.explen);
            reverse_path.push(packed_pair(parIdx, nxt_node));
          }
        }

      }
    }
  }
  return nodeList;
}
void RecompressionRLSLP::getNodeCover(c_size_t node, c_size_t left, c_size_t right, c_size_t queryLeft, c_size_t queryRight, const space_efficient_vector<RLSLPNonterm>& grammar, space_efficient_vector<packed_pair<c_size_t, c_size_t>>& nodes){
  if (left >= queryLeft && right <= queryRight){
    nodes.push_back(packed_pair(node, c_size_t(1)));
    return;
  }
  if (left > queryRight || right < queryLeft) return;
  const RLSLPNonterm& nt = grammar[node];
  if (nt.type == '1'){
    c_size_t leftNode = nt.first, rightNode = nt.second;
    c_size_t leftLength = grammar[leftNode].explen;
    getNodeCover(leftNode, left, left + leftLength - 1, queryLeft, queryRight, grammar, nodes);
    getNodeCover(rightNode, left + leftLength, right, queryLeft, queryRight, grammar, nodes);
  }
  else{
    c_size_t child = nt.first, exp = nt.second;
    c_size_t childLength = grammar[child].explen;
    c_size_t L = max(queryLeft, left), R = min(queryRight, right);
    c_size_t firstChild = (L - left) / childLength, lastChild = (R - left) / childLength;
    if (firstChild == lastChild){
      c_size_t start_pos = left + firstChild * childLength;
      getNodeCover(child, start_pos, start_pos + childLength - 1, queryLeft, queryRight, grammar, nodes);
    }
    else{
      c_size_t left_start = left + firstChild * childLength, right_start = left + lastChild * childLength;
      getNodeCover(child, left_start, left_start + childLength - 1, queryLeft, queryRight, grammar, nodes);
      if (lastChild - firstChild - 1 > 0) nodes.push_back(packed_pair(child, lastChild - firstChild - 1));
      getNodeCover(child, right_start, right_start + childLength - 1, queryLeft, queryRight, grammar, nodes);
    }
  }
}
Node RecompressionRLSLP::getPosition(c_size_t node, c_size_t par, c_size_t idxInPar, c_size_t left, c_size_t right, c_size_t pos, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar){
  Node curr_node(node, left, right, par, idxInPar); 
  curr_node.level = grammar[node].level;
  if (!ancestors.empty() && idxInPar <= 0 && ancestors.top().indexInParent == idxInPar) ancestors.pop();
  ancestors.push(curr_node);
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
packed_pair<c_size_t, c_size_t> RecompressionRLSLP::First(c_size_t var, c_size_t level){
  c_size_t newMask = leftMT.levelMask[var] & ((c_size_t(1) << (level + 1)) - 1);
  c_size_t k = __builtin_popcountll(leftMT.levelMask[var] ^ newMask);
  return packed_pair(leftMT.level_ancestor(var, leftMT.depths[var] - k), leftMT.level_ancestor(var, leftMT.depths[var] - k + 1));
}
packed_pair<c_size_t, c_size_t> RecompressionRLSLP::Last(c_size_t var, c_size_t level){

  c_size_t newMask = rightMT.levelMask[var] & ((c_size_t(1) << (level + 1)) - 1); 
  c_size_t k = __builtin_popcountll(rightMT.levelMask[var] ^ newMask);
  return packed_pair(rightMT.level_ancestor(var, rightMT.depths[var] - k), rightMT.level_ancestor(var, rightMT.depths[var] - k + 1));
}
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
char RecompressionRLSLP::getCharacter(c_size_t node, c_size_t left, c_size_t right, c_size_t pos, const space_efficient_vector<RLSLPNonterm>& grammar){
  const RLSLPNonterm& nt = grammar[node];
  if (left == right){
    return static_cast<char> (nt.first); 
  }
  c_size_t leftNode = nt.first;
  c_size_t leftLength = grammar[leftNode].explen;
  if (nt.type == '1'){
    if (left + leftLength > pos){
      return getCharacter(nt.first, left, left + leftLength - 1, pos, grammar);
    }
    else{
      return getCharacter(nt.second, left + leftLength, right, pos, grammar);
    }
  }
  c_size_t skipTimes = (pos - left) / leftLength;
  c_size_t skip = left + leftLength * ((pos - left) / leftLength);
  return getCharacter(leftNode, skip, skip + leftLength - 1, pos, grammar);
}
char RecompressionRLSLP::getCharacter(c_size_t pos){
  return getCharacter(nonterm.size() - 1, 0, nonterm.back().explen - 1, pos, nonterm);
}
void RecompressionRLSLP::updateStack(Node& node, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar){
  Node parent;
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
    c_size_t k = __builtin_popcountll(leftMT.levelMask[ancestors.top().var] ^ leftMT.levelMask[parent.var]);
    parent.parent = leftMT.level_ancestor(ancestors.top().var, leftMT.depths[ancestors.top().var] - (k - 1));
  }
  else{
    c_size_t k = __builtin_popcountll(rightMT.levelMask[ancestors.top().var] ^ rightMT.levelMask[parent.var]);
    parent.parent = rightMT.level_ancestor(ancestors.top().var, rightMT.depths[ancestors.top().var] - (k - 1));
  }
  if (!ancestors.empty() && parent.indexInParent <= 0 && parent.indexInParent == ancestors.top().indexInParent) ancestors.pop();
  ancestors.push(parent);
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
Node RecompressionRLSLP::Left(Node& x, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar){
  if (x.indexInParent != 0){
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
      if (!ancestors.empty() && res_node.indexInParent <= 0 && res_node.indexInParent == ancestors.top().indexInParent) {
        ancestors.pop();
      }
      ancestors.push(res_node);
      return res_node;
    }
    else{
      c_size_t k = parNT.type == '1' ? 1: parNT.second - 1;
      c_size_t nxtIdx = (x.indexInParent == -1 ? k - 1 : x.indexInParent - 1);
      Node first_child(childVar, x.l - grammar[childVar].explen, x.l - 1, x.parent, nxtIdx);
      first_child.level = grammar[childVar].level;
      if (!ancestors.empty() && first_child.indexInParent <= 0 && first_child.indexInParent == ancestors.top().indexInParent) ancestors.pop();
      ancestors.push(first_child);
      auto [nxtVar, nxtPar] = Last(childVar, lev);
      Node res_node(nxtVar, x.l - grammar[nxtVar].explen, x.l - 1, nxtPar, -1);
      res_node.level = lev;
      if (!ancestors.empty() && res_node.indexInParent <= 0 && res_node.indexInParent == ancestors.top().indexInParent) {
        ancestors.pop();
      }
      ancestors.push(res_node);
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
  if (!ancestors.empty() && lcaChild.indexInParent <= 0 && lcaChild.indexInParent == ancestors.top().indexInParent) ancestors.pop();
  lcaChild.level = grammar[lcaChild.var].level;
  ancestors.push(lcaChild);
  if (lcaChild.level <= x.level){
    lcaChild.level = x.level;
    return lcaChild;
  }
  auto [nodeIdx, par] = Last(lcaChild.var, x.level);
  Node res_node(nodeIdx, x.l - grammar[nodeIdx].explen, x.l - 1, par, -1);
  res_node.level = x.level;
  if (!ancestors.empty() && res_node.indexInParent <= 0 && res_node.indexInParent == ancestors.top().indexInParent) {
    ancestors.pop();
  }
  ancestors.push(res_node);
  return res_node;

}
Node RecompressionRLSLP::Right(Node& x, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar){
  if (x.indexInParent != -1){
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
      if (!ancestors.empty() && res_node.indexInParent <= 0 && res_node.indexInParent == ancestors.top().indexInParent) {
        ancestors.pop();
      }
      ancestors.push(res_node);
      return res_node;
    }
    else{
      c_size_t k = parNT.type == '1' ? 1: parNT.second - 1;
      Node first_child(childVar, x.r + 1, x.r + grammar[childVar].explen, x.parent, x.indexInParent == k - 1 ? -1 : x.indexInParent + 1);
      first_child.level = grammar[childVar].level;
      if (!ancestors.empty() && first_child.indexInParent <= 0 && first_child.indexInParent == ancestors.top().indexInParent){
        ancestors.pop();
      }
      ancestors.push(first_child);
      auto [nxtVar, nxtPar] = First(childVar, lev);
      Node res_node(nxtVar, x.r + 1, x.r + grammar[nxtVar].explen, nxtPar, 0);
      res_node.level = lev;
      if (!ancestors.empty() && res_node.indexInParent <= 0 && res_node.indexInParent == ancestors.top().indexInParent) {
        ancestors.pop();
      }
      ancestors.push(res_node);
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
  
  if (!ancestors.empty() && lcaChild.indexInParent <= 0 && lcaChild.indexInParent == ancestors.top().indexInParent) ancestors.pop();
  lcaChild.level = grammar[lcaChild.var].level;
  ancestors.push(lcaChild);
  if (lcaChild.level <= x.level){
    lcaChild.level = x.level;
    return lcaChild;
  }
  auto [nodeIdx, par] = First(lcaChild.var, x.level);
  Node res_node(nodeIdx, x.r + 1, x.r + grammar[nodeIdx].explen, par, 0);
  res_node.level = x.level;
  if (!ancestors.empty() && res_node.indexInParent <= 0 && res_node.indexInParent == ancestors.top().indexInParent) {
    ancestors.pop();
  }
  ancestors.push(res_node);
  return res_node;
}
space_efficient_vector<packed_pair<c_size_t, c_size_t>> RecompressionRLSLP::getPoppedSequence(c_size_t left, c_size_t right, const space_efficient_vector<RLSLPNonterm>& grammar){
    stack<Node> lstack, rstack;
    Node P = getPosition(grammar.size() - 1, -1, -2, 0, grammar.back().explen - 1, left, lstack, grammar), Q = getPosition(grammar.size() - 1 , -1, -2, 0, grammar.back().explen - 1, right, rstack, grammar);
    
    space_efficient_vector<packed_pair<c_size_t, c_size_t>> S, T;
    c_size_t height = grammar.back().level;
    for (c_size_t j = 0; j <= height && P.r < Q.l; j++){
      if (sameParent(P, Q, grammar)){
        if (grammar[Q.parent].type == '1'){
          S.push_back(packed_pair(P.var, c_size_t(1)));
          T.push_back(packed_pair(Q.var, c_size_t(1)));
        }
        else{
          S.push_back(packed_pair(P.var, (Q.indexInParent == -1 ? grammar[Q.parent].second - 1 : Q.indexInParent) - P.indexInParent + 1));
        }
        break;
      }
      if (((j & 1) && (((pCompMask[P.var] & (c_size_t(1) << (j / 2))) == 0))) || 
          (((j & 1) == 0) && bCompMask[P.var] & (c_size_t(1) << (j / 2)))){

            c_size_t rightEq = (j & 1 ? 0 : rext(P, grammar)) + 1;
            S.push_back(packed_pair(P.var, rightEq));
            Node pPar = constructParent(P, lstack, grammar);
            P = Right(pPar, lstack, grammar);
      }
      else{
          P = constructParent(P, lstack, grammar);
      }
      if (((j & 1) && ((pCompMask[Q.var] & (c_size_t(1) << (j / 2))))) || 
            (((j & 1) == 0) && bCompMask[Q.var] & (c_size_t(1) << (j / 2)))){ //there's only one level where this variable can be involved in BComp, probably don't need BCompMask
              c_size_t leftEq = (j & 1 ? 0: lext(Q, grammar)) + 1;
              T.push_back(packed_pair(Q.var, leftEq));
              Node qPar = constructParent(Q, rstack, grammar);
              Q = Left(qPar, rstack, grammar);
        }
        else{
            Q = constructParent(Q, rstack, grammar);
        }
    }
    if (P.l == Q.l && P.r == Q.r){
      S.push_back(packed_pair(P.var, c_size_t(1)));
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
