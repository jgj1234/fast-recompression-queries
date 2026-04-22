#include "ReportQueryComponent.hpp"
ReportQueryComponent::ReportQueryComponent(const space_efficient_vector<RLSLPNonterm>& grammar){
    initialize_rhsNodeList(grammar);
}
void ReportQueryComponent::initialize_rhsNodeList(const space_efficient_vector<RLSLPNonterm>& grammar){
    c_size_t nonterm_num = grammar.size();
    space_efficient_vector<c_size_t> rhsNodeListBuckets(nonterm_num);
    for (c_size_t i = 0; i < nonterm_num; i++){
      const RLSLPNonterm& nt = grammar[i];
      if (nt.type == '1'){
        rhsNodeListBuckets[nt.first]++;
        rhsNodeListBuckets[nt.second]++;
      }
      else if (nt.type == '2'){
        rhsNodeListBuckets[nt.first]++;
      }
    }
    rhsNodeList = space_efficient_vector_2D<c_size_t> (rhsNodeListBuckets);
    space_efficient_vector<c_size_t> rhsNodeListIndex(nonterm_num);
    for (c_size_t i = 0; i < nonterm_num; i++){
      const RLSLPNonterm& nt = grammar[i];
      if (nt.type == '1'){
        rhsNodeList.push_back(nt.first, i, rhsNodeListIndex);
        rhsNodeList.push_back(nt.second, i, rhsNodeListIndex);
      }
      else if (nt.type == '2'){
        rhsNodeList.push_back(nt.first, i, rhsNodeListIndex);
      }
    }
}
space_efficient_vector<Node> ReportQueryComponent::enumerateNodes(const space_efficient_vector<RLSLPNonterm>& grammar, c_size_t symbol){
  space_efficient_vector<Node> nodeList;
  stack<packed_pair<c_size_t, Node>> reverse_path;
  reverse_path.push(packed_pair<c_size_t, Node>(symbol, Node(symbol, 0, grammar[symbol].explen)));
  while (!reverse_path.empty()){
    c_size_t currSymbol = reverse_path.top().first;
    Node curr_node = reverse_path.top().second;
    reverse_path.pop();
    if (rhsNodeList.empty(currSymbol)){
      nodeList.push_back(curr_node);
    }
    else{
      for (c_size_t i = 0; i < rhsNodeList.size(currSymbol); i++){
        c_size_t parIdx = rhsNodeList(currSymbol, i);
        const RLSLPNonterm& parNode = grammar[parIdx];
        if (parNode.type == '1'){
          Node nxt_node = Node();
          nxt_node.var = symbol;
          if (parNode.first == currSymbol){
            nxt_node.l = curr_node.l;
            nxt_node.r = curr_node.r;
          }
          else{
            const RLSLPNonterm& sibling = grammar[grammar[parIdx].first];
            nxt_node.l = curr_node.l + sibling.explen;
            nxt_node.r = curr_node.r + sibling.explen;
          }
          reverse_path.push(packed_pair<c_size_t, Node>(parIdx, nxt_node));
        }
        else{
          c_size_t k = parNode.second;
          const RLSLPNonterm& symbolPos = grammar[currSymbol];
          for (c_size_t i = 0; i < k; i++){
            Node nxt_node = Node(symbol, curr_node.l + i * symbolPos.explen, curr_node.r + i * symbolPos.explen);
            reverse_path.push(packed_pair<c_size_t, Node>(parIdx, nxt_node));
          }
        }

      }
    }
  }
  return nodeList;
}