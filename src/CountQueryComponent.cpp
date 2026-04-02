#include "CountQueryComponent.hpp"
CountQueryComponent::CountQueryComponent(const space_efficient_vector<RLSLPNonterm>& grammar){
    initialize_rhsExpList(grammar);
    initialize_countNodes(grammar);
    initialize_CountQueryStructure();
}
c_size_t CountQueryComponent::queryCount(c_size_t symbol, c_size_t m){
  packed_pair<c_size_t, c_size_t> qpair(m + 1, c_size_t(1));
  c_size_t idx = rhsExpList.lower_bound(symbol, qpair);
  if (idx == 0) return 0;
  --idx;
  return countQueryStructure(symbol, idx).first - m * countQueryStructure(symbol, idx).second;
}
void CountQueryComponent::initialize_rhsExpList(const space_efficient_vector<RLSLPNonterm>& grammar){
    c_size_t nonterm_num = grammar.size();
    space_efficient_vector<c_size_t> rhsExpListBuckets(nonterm_num);
    for (c_size_t i = 0; i < nonterm_num; i++){
      const RLSLPNonterm& nt = grammar[i];
      rhsExpListBuckets[i]++;
      if (nt.type == '2'){
        rhsExpListBuckets[nt.first]++;
      }
    }
    rhsExpList = space_efficient_vector_2D<packed_pair<c_size_t, c_size_t>> (rhsExpListBuckets);
    space_efficient_vector<c_size_t> rhsExpListIndex(nonterm_num);
    for (c_size_t i = 0; i < nonterm_num; i++){
      const RLSLPNonterm& nt = grammar[i];
      rhsExpList.push_back(i, packed_pair<c_size_t, c_size_t>(c_size_t(1), c_size_t(0)), rhsExpListIndex);
      if (nt.type == '2'){
        rhsExpList.push_back(nt.first, packed_pair<c_size_t, c_size_t>(nt.second, i), rhsExpListIndex);
      }
    }
    for (c_size_t i = 0; i < nonterm_num; i++){
      rhsExpList.sort(i);
    }
}
void CountQueryComponent::initialize_countNodes(
  const space_efficient_vector<RLSLPNonterm>& grammar
){
    c_size_t num_nodes = grammar.size();
    countNodes.resize(num_nodes);
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
void CountQueryComponent::initialize_CountQueryStructure(){
  c_size_t n = countNodes.size();
  space_efficient_vector<c_size_t> countQueryStructureBuckets(n);
  for (c_size_t i = 0; i < n; i++) countQueryStructureBuckets[i] = rhsExpList.size(i);
  countQueryStructure = space_efficient_vector_2D<packed_pair<c_size_t, c_size_t>> (countQueryStructureBuckets);
  for (c_size_t i = 0; i < n; i++){
    c_size_t suffixK = 0, suffixSum = 0;
    for (c_size_t j = countQueryStructure.size(i) - 1; j >= 0; j--){
      countQueryStructure(i, j) = packed_pair<c_size_t, c_size_t>(suffixK, suffixSum);
      suffixK += rhsExpList(i, j).first * countNodes[rhsExpList(i, j).second];
      suffixSum += countNodes[rhsExpList(i, j).second];
    }
  }
}
void CountQueryComponent::getPotentialHooks(const space_efficient_vector<RLSLPNonterm>& grammar, c_size_t node, c_size_t left, c_size_t right, c_size_t leftY, c_size_t rightY, c_size_t length, space_efficient_vector<Node>& symbols){
  if (min(rightY, right) - max(leftY, left) + 1 < length) return;
  const RLSLPNonterm& nt = grammar[node];
  symbols.push_back(Node(node, left, right));
  if (nt.type == '0') return;
  c_size_t neededPos = leftY + length - 1;
  c_size_t leftLength = grammar[nt.first].explen;
  if (nt.type == '1'){
    if (left + leftLength - 1 >= neededPos){
      getPotentialHooks(grammar, nt.first, left, left + leftLength - 1, leftY, rightY, length, symbols);
    }
    else{
      getPotentialHooks(grammar, nt.second, left + leftLength, right, leftY, rightY, length, symbols);
    }
  }
  else if (nt.type == '2'){
    c_size_t skipPos = left + leftLength * ((neededPos - left) / leftLength);
    getPotentialHooks(grammar, nt.first, skipPos, skipPos + leftLength - 1, leftY, rightY, length, symbols);
  }
}
c_size_t CountQueryComponent::leftMostIPMQuery(c_size_t xl, c_size_t xr, c_size_t yl, c_size_t yr, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp){
    //left most position of x in y relative to y (1 indexed)
    space_efficient_vector<Node> hooks;
    c_size_t n = recomp_rlslp->nonterm.back().explen;
    c_size_t xLength = xr - xl + 1;
    getPotentialHooks(recomp_rlslp->nonterm, recomp_rlslp->nonterm.size() - 1, 0, n - 1, yl, yr, xLength, hooks);
    space_efficient_vector<c_size_t> anchors = recomp_rlslp->getAnchors(xl, xLength);
    c_size_t leftPos = numeric_limits<c_size_t>::max();
    for (c_size_t i = 0; i < anchors.size(); i++){
        c_size_t anchorLength = anchors[i];
        for (c_size_t j = 0; j < hooks.size(); j++){
        Node hook = hooks[j];
        const RLSLPNonterm& nt = recomp_rlslp->nonterm[hook.var];
        if (nt.type == '0'){
            leftPos = min(leftPos, hook.l - (yl - 1));
        }
        else if (nt.type == '1'){
            c_size_t leftLength = recomp_rlslp->nonterm[nt.first].explen;
            c_size_t rightLength = recomp_rlslp->nonterm[nt.second].explen;
            c_size_t occStart = hook.l + leftLength - anchorLength;
            if (occStart >= yl){
                c_size_t leftLCE = min(min(leftLength, anchorLength), reverse_rlslp->lce(n - 1 - (xl + anchorLength - 1), n - 1 - (hook.l + leftLength - 1)));
                c_size_t rightLCE = min(min(rightLength, xLength - anchorLength), recomp_rlslp->lce(xl + anchorLength, hook.l + leftLength));
                if (leftLCE >= anchorLength && rightLCE >= xLength - anchorLength){
                    leftPos = min(leftPos, hook.l + leftLength - anchorLength - (yl - 1));
                }
            }
        }
        else{
            c_size_t childLength = recomp_rlslp->nonterm[nt.first].explen;
            c_size_t k = nt.second;
            c_size_t minI = 1;
            if (hook.l + childLength < yl + anchorLength) {
                c_size_t need = yl + anchorLength - hook.l;
                minI = (need + childLength - 1) / childLength;
            }
            if (minI < k) {
                c_size_t pos = hook.l + minI * childLength;
                c_size_t occStart = pos - anchorLength;
                if (occStart >= yl){
                    c_size_t leftLCE = min(min(anchorLength, childLength), reverse_rlslp->lce(n - 1 - (xl + anchorLength - 1), n -1 - (pos - 1)));
                    c_size_t rightLCE = min(min(childLength * (k - minI), xLength - anchorLength), recomp_rlslp->lce(xl + anchorLength, pos));
                    if (leftLCE >= anchorLength && rightLCE >= xLength - anchorLength){
                        leftPos = min(leftPos, pos - anchorLength - (yl - 1));
                    }
                }
            }
        }
        }
    }
    return leftPos;
}
c_size_t CountQueryComponent::periodQuery(c_size_t l, c_size_t r, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp){
    c_size_t length = r - l + 1;
    c_size_t half = (length + 1) / 2;
    c_size_t leftOcc = leftMostIPMQuery(l, l + half - 1, l + 1, r, recomp_rlslp, reverse_rlslp);
    if (leftOcc == numeric_limits<c_size_t>::max()) return -1; // no period found
    c_size_t lce = recomp_rlslp->lce(l, l + leftOcc);
    if (lce >= length - leftOcc) return leftOcc;
    return -1;
}