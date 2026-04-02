#include "recompression_definitions.hpp"
class CountQueryComponent{
    public:
        space_efficient_vector<c_size_t> countNodes;
        space_efficient_vector_2D<packed_pair<c_size_t, c_size_t>> rhsExpList, countQueryStructure;
        CountQueryComponent(const space_efficient_vector<RLSLPNonterm>& grammar);
        void initialize_rhsExpList(const space_efficient_vector<RLSLPNonterm>& grammar);
        void initialize_CountQueryStructure();
        c_size_t queryCount(c_size_t symbol, c_size_t m);
        void initialize_countNodes(const space_efficient_vector<RLSLPNonterm>& grammar);
        void getPotentialHooks(const space_efficient_vector<RLSLPNonterm>& grammar, c_size_t node, c_size_t left, c_size_t right, c_size_t leftY, c_size_t rightY, c_size_t length, space_efficient_vector<Node>& symbols);
        c_size_t leftMostIPMQuery(c_size_t xl, c_size_t xr, c_size_t yl, c_size_t yr, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp);
        c_size_t periodQuery(c_size_t l, c_size_t r, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp);
};