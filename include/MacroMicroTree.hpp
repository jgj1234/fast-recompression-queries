
using namespace std;
#include <utils/space_efficient_vector.hpp>
#include <typedefs.hpp>

class MacroMicroTree{
public:
    space_efficient_vector<space_efficient_vector<c_size_t>> g, ladders, jumpPointers, dfsPath;
    space_efficient_vector<c_size_t> par, depths, height, jumpNodes, ladderIdx, ladderPos, dfsOrd, dfsPos, shapes, bct;
    space_efficient_vector<c_size_t> levelMask;
    space_efficient_vector<space_efficient_vector<space_efficient_vector<c_size_t>>> shapeTables;
    c_size_t n, micro_threshold;
    MacroMicroTree(){}
    MacroMicroTree(c_size_t n, space_efficient_vector<space_efficient_vector<c_size_t>>& graph);
    c_size_t level_ancestor(c_size_t node, c_size_t depth);
    MacroMicroTree(MacroMicroTree&&) = default;
    MacroMicroTree& operator=(MacroMicroTree&&) = default;
    MacroMicroTree(const MacroMicroTree&) = delete;
    MacroMicroTree& operator=(const MacroMicroTree&) = delete;
private:
    c_size_t init_dfs(c_size_t node, c_size_t parent);
    void constructLadders(c_size_t node);
    void getShape(c_size_t node, c_size_t nearJump, c_size_t& mask, c_size_t& index, c_size_t& dfsIdx, space_efficient_vector<c_size_t>& ord);
    void constructTable(c_size_t& node, c_size_t par, c_size_t mask, c_size_t& index, c_size_t depth, space_efficient_vector<space_efficient_vector<c_size_t>>& table);
};