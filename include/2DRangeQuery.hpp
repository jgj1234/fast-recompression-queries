
#include "typedefs.hpp"
#include "utils/space_efficient_vector.hpp"
#include "utils/packed_pair.hpp"
#include "recompression_definitions.hpp"
using namespace std;
/*
class RangeQuery{
public:
    space_efficient_vector<c_size_t> X, Y, B, C;
    c_size_t lambda, mu, M, n;
    RangeQuery(){}
    RangeQuery(space_efficient_vector<c_size_t>& X, space_efficient_vector<c_size_t>& Y, space_efficient_vector<c_size_t>& weights);
    c_size_t rangeCount(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
    c_size_t rangeMinimum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
    space_efficient_vector<c_size_t> rangeReport(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
    c_size_t rangeSum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
private:
    c_size_t onesBefore(len_t pos);
    c_size_t newPos(c_size_t dir, c_size_t block, c_size_t pos, c_size_t width);
    c_size_t Path(space_efficient_vector<c_size_t>& A, c_size_t q);
    c_size_t cumulativeCount(c_size_t cut, c_size_t init, c_size_t path, c_size_t dir);
};*/
class RangeQuery{
    public:
        space_efficient_vector<Fragment> X, Y;
        space_efficient_vector<c_size_t> weights, rankY;
        RangeQuery();
        RangeQuery(RecompressionRLSLP* rlslp, space_efficient_vector<Fragment>& X, space_efficient_vector<Fragment>& Y, space_efficient_vector<c_size_t>& weights);
        c_size_t rangeMinimum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
        space_efficient_vector<c_size_t> rangeReport(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
        c_size_t rangeSum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2);
};