#include "recompression_definitions.hpp"
class ReportQueryComponent{
    public:
        space_efficient_vector_2D<c_size_t> rhsNodeList;
        ReportQueryComponent(const space_efficient_vector<RLSLPNonterm>& grammar);
        void initialize_rhsExpList(const space_efficient_vector<RLSLPNonterm>& grammar);
        space_efficient_vector<Node> enumerateNodes(const space_efficient_vector<RLSLPNonterm>& grammar, c_size_t symbol);
};