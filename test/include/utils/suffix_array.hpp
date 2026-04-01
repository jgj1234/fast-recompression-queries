#include <string>
#include <space_efficient_vector.hpp>
#include <packed_pair.hpp>
#include <space_efficient_vector_2D.hpp>
#include <typedefs.hpp>
using namespace std;
class SuffixArray {
    public:
        c_size_t n;
        string s;
        space_efficient_vector<c_size_t> p, c, tp, tc, lcp, ct, where;
        space_efficient_vector_2D<c_size_t> indexRMQ, lcpRMQ;
        
        SuffixArray(string st) {
            s = st;
            s += "$";
            n = s.size();
            p.resize(n);
            c.resize(n);
            tp.resize(n);
            tc.resize(n);
            where.resize(n);
            ct.resize(max(c_size_t(128), n));
            lcp.resize(n - 1);
            build();
            buildRMQ(indexRMQ, p);
            genLCP();
            buildRMQ(lcpRMQ, lcp);
        }
        void buildRMQ(space_efficient_vector_2D<c_size_t>& rmq, space_efficient_vector<c_size_t>& baseArr){
            c_size_t sz = baseArr.size();
            c_size_t lg = 64 - __builtin_clzll(sz);
            space_efficient_vector<c_size_t> rmqBuckets(sz, lg);
            rmq = space_efficient_vector_2D<c_size_t> (rmqBuckets);
            for (c_size_t i = 0; i < sz; i++) {
                rmq(i, 0) = baseArr[i];
            }
            for (c_size_t j = 1; j < lg; j++) {
                for (c_size_t i = 0; i + (c_size_t(1) << j) - 1 < sz; i++) {
                    rmq(i, j) = min(rmq(i, j - 1), rmq(i + (c_size_t(1) << (j - 1)), j - 1));
                }
            }
        }

        c_size_t query(c_size_t l, c_size_t r, space_efficient_vector_2D<c_size_t>& rmq) {
            //r-- for lcp rmq
            c_size_t lg = 63 - __builtin_clzll(r - l + 1);
            return min(rmq(l, lg), rmq(r - (c_size_t(1) << lg) + 1, lg));
        }
        c_size_t lce(c_size_t i, c_size_t j){
            if (i == j) return n - i;
            return query(min(where[i], where[j]), max(where[i], where[j]) - 1, lcpRMQ);
        }
        packed_pair<c_size_t, c_size_t> getMatchRange(c_size_t index, c_size_t length) {
            c_size_t pos = where[index];
            c_size_t lp = 0, rp = n - 1;
            packed_pair<c_size_t, c_size_t> res;
            while (lp <= rp){
                c_size_t mid = (lp + rp) / 2;
                c_size_t midLCE = lce(index, p[mid]);
                if (midLCE >= length || s[index + midLCE] < s[p[mid] + midLCE]){
                    rp = mid - 1;
                }
                else{
                    lp = mid + 1;
                }
            }
            res.first = lp;
            rp = n - 1;
            while (lp <= rp){
                c_size_t mid = (lp + rp) / 2;
                c_size_t midLCE = lce(index, p[mid]);
                if (midLCE >= length){
                    lp = mid + 1;
                }
                else{
                    rp = mid - 1;
                }
            }
            res.second = rp;
            return res;
        }
        c_size_t countOccurrences(c_size_t index, c_size_t length){
            auto [l, r] = getMatchRange(index, length);
            return r - l + 1;
        }
        c_size_t getLeftMostOccurrence(c_size_t index, c_size_t length){
            auto [l, r] = getMatchRange(index, length);
            return query(l, r, indexRMQ);
        }
        void reportOccurrences(c_size_t index, c_size_t length, space_efficient_vector<c_size_t>& occs){
            auto [l, r] = getMatchRange(index, length);
            occs.resize(r - l + 1);
            for (c_size_t i = l; i <= r; i++) occs[i - l] = p[i];
        }
        void build() {
            for (c_size_t i = 0; i < n; i++) {
                ct[s[i]]++;
            }
            for (c_size_t i = 1; i < ct.size(); i++) ct[i] += ct[i - 1];
            for (c_size_t i = 0; i < n; i++) {
                p[--ct[s[i]]] = i;
            }

            c_size_t classes = 1;
            c[p[0]] = 0;
            for (c_size_t i = 1; i < n; i++) {
                if (s[p[i]] != s[p[i - 1]]) {
                    classes++;
                }
                c[p[i]] = classes - 1;
            }

            for (c_size_t k = 1; k < n; k <<= 1) {
                for (c_size_t i = 0; i < n; i++) {
                    tp[i] = p[i] - k;
                    if (tp[i] < 0) tp[i] += n;
                }
                for (c_size_t i = 0; i < classes; i++) ct[i] = 0;
                for (c_size_t i = 0; i < n; i++) ct[c[i]]++;
                for (c_size_t i = 1; i < classes; i++) ct[i] += ct[i - 1];
                for (c_size_t i = n - 1; i >= 0; i--) {
                    p[--ct[c[tp[i]]]] = tp[i];
                }

                tc = c;
                classes = 1;
                c[p[0]] = 0;
                for (c_size_t i = 1; i < n; i++) {
                    if (tc[p[i]] != tc[p[i - 1]] ||
                        tc[(p[i] + k) % n] != tc[(p[i - 1] + k) % n]) {
                        classes++;
                    }
                    c[p[i]] = classes - 1;
                }
            }

            for (c_size_t i = 0; i < n; i++) where[p[i]] = i;
        }

        void genLCP() {
            c_size_t pointer = 0;
            for (c_size_t i = 0; i < n; i++) {
                if (where[i] == n - 1) {
                    pointer = 0;
                    continue;
                }
                c_size_t nxt = p[where[i] + 1];
                while (i + pointer < n && nxt + pointer < n &&
                    s[i + pointer] == s[nxt + pointer]) {
                    pointer++;
                }
                lcp[where[i]] = pointer;
                pointer = max(pointer - 1, c_size_t(0));
            }
        }
};