#include "2DRangeQuery.hpp"
#include "recompression_definitions.hpp"
/*
RangeQuery::RangeQuery(){
    
}
RangeQuery::RangeQuery(
  space_efficient_vector<c_size_t>& X, space_efficient_vector<c_size_t>& Y, space_efficient_vector<c_size_t>& weights) :
  X(X), Y(Y) {
        n = X.size();
        c_size_t nxt_pw2 = n == 1 ? 1: 1LL << (64 - __builtin_clzll(n - 1));
        for (len_t j = 0; j < nxt_pw2 - n; j++){
            X.push_back(numeric_limits<c_size_t>::max());
            Y.push_back(numeric_limits<c_size_t>::max());
        }
        n = nxt_pw2;
        lambda = __builtin_ctzll(n);
        mu = 2 * (1 + (63 - __builtin_clzll(lambda)));
        M = 1LL << mu;
        B.resize(n, 0);
        C.resize(n, 0);
        c_size_t fill = 0;
        len_t index = 0;
        space_efficient_vector<packed_pair<c_size_t, c_size_t>> T(n + 1, packed_pair(c_size_t(0), c_size_t(0)));
        for (c_size_t step = 2; step <= n; step <<= 1){
            for (len_t l = 0; l < n; l += step){
                len_t r = l + step - 1;
                c_size_t mid = (l + r) / 2;
                for (int j = l; j <= r; j++) T[j] = Y[j];
                len_t i = l, j = mid + 1, k = l - 1;
                while (i <= mid || j <= r){
                    k++;
                    B[index] <<= 1;
                    if ((i > mid) || (j <= r && T[j] <= T[i])){
                        Y[k] = T[j++];
                        B[index]++;
                        C[index]++;
                    }
                    else{
                        Y[k] = T[i++];
                    }
                    fill++;
                    if (fill == lambda){
                        B[index] *= M;
                        B[index] |= __builtin_popcountll(index);
                        index++;
                        fill = 0;
                        if (index < n) C[index] = C[index - 1];
                    }
                }
            }
        }
        fill = M;
        for (c_size_t index = 0; index < lambda; index++){
            C[index] |= fill;
            fill <<= 1;
        }
}
c_size_t RangeQuery::onesBefore(len_t pos){
    //number of 1s in b[0....pos - 1]
    c_size_t i = pos / lambda;
    c_size_t j = B[i] / (2 * M * (C[lambda * (i + 1) - pos - 1] / M));
    c_size_t z = B[j] - M * (B[j] / M);
    if (i > 0) {
        z += C[i - 1];
        if (i <= lambda) z -=  M * (C[i - 1] / M);
    }
    return z;
}
c_size_t RangeQuery::newPos(c_size_t dir, c_size_t block, c_size_t pos, c_size_t width){
    if (dir == 0){
        return pos - n + onesBefore(block) - onesBefore(pos);
    }
    return block - n + onesBefore(pos) - onesBefore(block) + width / 2;
}
c_size_t RangeQuery::Path(space_efficient_vector<c_size_t>& A, c_size_t q){
    c_size_t l = 0, r = n - 1;
    while (l < r){
        c_size_t k = (l + r) / 2;
        if (q <= A[k]){
            r = k;
        }
        else{
            l = k + 1;
        }
    }
    return l;
}

c_size_t RangeQuery::cumulativeCount(c_size_t cut, c_size_t init, c_size_t path, c_size_t dir){
    c_size_t pos = init, z = 0, block = (lambda - 1) * n, cur = n;
    while (cur >= 2){
        c_size_t bit = (2 * path) / cur - 2 * (path / cur);
        if (cur < cut && bit == dir){
            z += newPos(dir ^ 1, block, pos, cur);
        }
        pos = newPos(bit, block, pos, cur);
        cur /= 2;
        block = block - n + bit * cur;
    }
    return z;
}
c_size_t RangeQuery::rangeCount(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    c_size_t low = Path(Y, y1) + (lambda - 1) * n, high = Path(Y, y2) + (lambda - 1) * n;
    c_size_t left = Path(X, x1) - 1, right = Path(X, x2), cut = n;
    while ((2 * left) / cut == (2 * right) / cut) cut /= 2;
    c_size_t z = cumulativeCount(cut, high, left, 0) + cumulativeCount(cut, high, right, 1);
    return z - cumulativeCount(cut, low, left, 0) - cumulativeCount(cut, low, right, 1);
}
space_efficient_vector<c_size_t> RangeQuery::rangeReport(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    
}

c_size_t RangeQuery::rangeMinimum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){

}
c_size_t RangeQuery::rangeSum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    
}
*/

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
  RecompressionRLSLP* rlslp, space_efficient_vector<Fragment>& X, space_efficient_vector<Fragment>& Y, space_efficient_vector<c_size_t>& weights) :
  X(X), Y(Y), weights(weights) {
    rankY.resize(Y.size());
    space_efficient_vector<pair<Fragment, c_size_t>> arr(Y.size());
    for (c_size_t i = 0; i < Y.size(); i++){
        arr[i] = make_pair(Y[i], i);
    }
    FragmentComparator fc(rlslp);
    arr.sort(fc);
    for (c_size_t i = 0; i < Y.size(); i++){
        this->Y[i] = arr[i].first;
        this->rankY[arr[i].second] = i;
    }
}
space_efficient_vector<c_size_t> RangeQuery::rangeReport(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    space_efficient_vector<c_size_t> occs;
    for (int i = x1; i <= x2; i++){
        if (rankY[i] >= y1 && rankY[i] <= y2){
            occs.push_back(weights[i]);
        }
    }
    return occs;
}
c_size_t RangeQuery::rangeMinimum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    c_size_t mnOcc = numeric_limits<c_size_t>::max();
    for (int i = x1; i <= x2; i++){
        if (rankY[i] >= y1 && rankY[i] <= y2){
            mnOcc = min(mnOcc, weights[i]);
        }
    }
    return mnOcc;
}
c_size_t RangeQuery::rangeSum(c_size_t x1, c_size_t x2, c_size_t y1, c_size_t y2){
    c_size_t result = 0;
    for (int i = x1; i <= x2; i++){
        if (rankY[i] >= y1 && rankY[i] <= y2){
            result += weights[i];
        }
    }
    return result;
}