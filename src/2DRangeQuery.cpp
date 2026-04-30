#include "2DRangeQuery.hpp"
#include "recompression_definitions.hpp"
#include <sdsl/wt_int.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/int_vector_buffer.hpp>
RangeQuery::RangeQuery(){
    
}
struct FragmentComparator{
    RecompressionRLSLP* rlslp;
    FragmentComparator(RecompressionRLSLP* rlslp): rlslp(rlslp){}
    bool operator()(const pair<Fragment, c_size_t>& a, const pair<Fragment, c_size_t>& b) const{
        const Fragment& x = a.first, y = b.first;
        c_size_t firstX = x.index, firstY = y.index;
        c_size_t lengthX = x.length, lengthY = y.length;
        c_size_t lce = rlslp->lce(firstX, firstY);
        if (lce >= lengthX || lce >= lengthY){
            return lengthX < lengthY;
        }
        char c1 = rlslp->getCharacter(firstX + lce), c2 = rlslp->getCharacter(firstY + lce);
        return c1 < c2;
    }
};
RangeQuery::RangeQuery(
  RecompressionRLSLP* rlslp, space_efficient_vector<Fragment>& X, space_efficient_vector<Fragment>& Y, space_efficient_vector<c_size_t>& weights, 
  c_size_t queryType
) :
  X(X), Y(Y), weights(weights) {
    len_t n = Y.size();
    rankY.resize(n);
    space_efficient_vector<pair<Fragment, c_size_t>> arr(n);
    for (c_size_t i = 0; i < n; i++){
        arr[i] = make_pair(Y[i], i);
    }
    FragmentComparator fc(rlslp);
    arr.sort(fc);
    for (c_size_t i = 0; i < n; i++) this->Y[i] = arr[i].first;
    this->rankY = sdsl::int_vector<> (n);
    for (c_size_t i = 0; i < n; i++) this->rankY[arr[i].second] = i;
    string buffer_name = "temp_buffer";
    sdsl::int_vector_buffer<64> buf(buffer_name, ios::out);
    for (c_size_t i = 0; i < n; i++) buf.push_back(this->rankY[i]);
    this->wavelet_tree = sdsl::wt_int<> (buf, n, weights, queryType == 1, queryType > 0);
    sdsl::remove(buffer_name);
}
space_efficient_vector<c_size_t> RangeQuery::rangeReport(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    space_efficient_vector<c_size_t> res = this->wavelet_tree.range_search_2d(x1, x2, y1, y2);
    return res;
}
c_size_t RangeQuery::rangeMinimum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    return this->wavelet_tree.range_minimum_2d(x1, x2, y1, y2);
}
c_size_t RangeQuery::rangeSum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    return this->wavelet_tree.range_sum_2d(x1, x2, y1, y2);
}