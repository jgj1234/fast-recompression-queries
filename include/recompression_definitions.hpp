#ifndef RECOMPRESSION_DEFINITIONS_HPP
#define RECOMPRESSION_DEFINITIONS_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <stack>
#include <cassert>

#include "typedefs.hpp"
#include "utils/space_efficient_vector.hpp"
#include "utils/space_efficient_vector_2D.hpp"
#include "utils/packed_pair.hpp"
#include "MacroMicroTree.hpp"
using namespace std;

struct Node;

struct __attribute__((packed)) RLSLPNonterm {
  char_t type;
  c_size_t first;
  c_size_t second;
  c_size_t explen;
  c_size_t level;

  RLSLPNonterm();

  RLSLPNonterm(
    const char_t& type,
    const c_size_t& first,
    const c_size_t& second);
  
  RLSLPNonterm(
    const char_t& type,
    const c_size_t& first,
    const c_size_t& second,
    const c_size_t& explen);
};
struct Fragment{
    c_size_t index, length;
    Fragment(){}
    Fragment(c_size_t index, c_size_t length): index(index), length(length) {}
};
class RecompressionRLSLP {
public:
  space_efficient_vector<RLSLPNonterm> nonterm;
  space_efficient_vector<Node> firstNodes;
  BitVector pCompMask, bCompMask;
  MacroMicroTree leftMT, rightMT;
  uint64_t ram_use() const;
  void write_to_file(const string &filename);
  bool read_from_file(const string &filename);
  void initStructures();
  void computeExplen();
  //void constructTrees();
  void reverseRLSLP(c_size_t node, RecompressionRLSLP* rev_rlslp);
  void initialize_firstNodes(c_size_t node, c_size_t left, c_size_t right, const space_efficient_vector<RLSLPNonterm>& grammar, space_efficient_vector<Node>& firstNodes);
  space_efficient_vector<c_size_t> getAnchors(c_size_t index, c_size_t length);
  c_size_t getSymbol(c_size_t pos);
  char getCharacter(c_size_t pos);
  c_size_t lce(c_size_t i, c_size_t j);

private:
  void initialize_nodes(c_size_t node, const c_size_t& i, c_size_t left, c_size_t right, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar, Node& v);
  bool sameParent(Node& x, Node& y, const space_efficient_vector<RLSLPNonterm>& grammar);
  //packed_pair<c_size_t, c_size_t> First(c_size_t var, c_size_t level);
  //packed_pair<c_size_t, c_size_t> Last(c_size_t var, c_size_t level);
  c_size_t getSymbol(c_size_t node, c_size_t left, c_size_t right, c_size_t pos, const space_efficient_vector<RLSLPNonterm>& grammar);
  char getCharacter(c_size_t node, c_size_t left, c_size_t right, c_size_t pos, const space_efficient_vector<RLSLPNonterm>& grammar);
  Node getLeftMostChild(Node v, const space_efficient_vector<RLSLPNonterm> & grammar);
  c_size_t getChildCount(const Node &parent, const space_efficient_vector<RLSLPNonterm> &grammar);
  c_size_t getChildIndex(const Node &parent, const Node &v, const space_efficient_vector<RLSLPNonterm> &grammar);
  Node getKthSibling(const Node &parent, const Node &v, c_size_t k);
  Node constructParent(Node& node, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar);
  Node replaceWithHighestStartingAtPosition(const Node &v, stack<Node> &ancestors, const space_efficient_vector<RLSLPNonterm> &grammar);
  c_size_t LCE(Node &v1, Node &v2, c_size_t i, stack<Node> & v1_ancestors, stack<Node> & v2_ancestors, const space_efficient_vector<RLSLPNonterm> &grammar);
  c_size_t computeExplen(const c_size_t i);
  void updateStack(Node& node, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar);
  Node Left(Node& x, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar);
  Node Right(Node& x, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm> &grammar);
  c_size_t lext(Node& x, const space_efficient_vector<RLSLPNonterm> &grammar);
  c_size_t rext(Node& x, const space_efficient_vector<RLSLPNonterm> &grammar);
  void pushToStack(Node& x, stack<Node>& ancestors);
  Node getPosition(c_size_t node, c_size_t par, c_size_t idxInPar, c_size_t left, c_size_t right, c_size_t pos, stack<Node>& ancestors, const space_efficient_vector<RLSLPNonterm>& grammar);
  space_efficient_vector<packed_pair<c_size_t, c_size_t>> getPoppedSequence(c_size_t left, c_size_t right, const space_efficient_vector<RLSLPNonterm>& grammar);
};


struct Node {
    // Variable/Non-Terminal
    c_size_t var;
    // [l, r)
    c_size_t l;
    c_size_t r;
    c_size_t parent;
    c_size_t indexInParent;
    c_size_t level;
    Node();
    Node(const c_size_t& var, const c_size_t& l, const c_size_t& r);
    Node(const c_size_t& var, const c_size_t& l, const c_size_t& r, const c_size_t& parent, const c_size_t& indexInParent);

};

#endif
