
#include "space_efficient_vector.hpp"
#include "../typedefs.hpp"
#include <cassert>
#include <execinfo.h>
template<typename ValueType>
class space_efficient_vector_2D{
    public:
        space_efficient_vector<c_size_t> bucketStarts;
        space_efficient_vector<ValueType> arr;
        space_efficient_vector_2D(const space_efficient_vector<c_size_t>& buckets){
            c_size_t totalLength = 0;
            c_size_t buckSz = buckets.size();
            for (c_size_t i = 0; i < buckSz; i++) totalLength += buckets[i];
            arr.resize(totalLength);
            bucketStarts.resize(buckSz);
            c_size_t pref = 0;
            for (c_size_t i = 0; i < buckSz; i++){
                bucketStarts[i] = pref;
                pref += buckets[i];
            }
        }
        space_efficient_vector_2D(){
            //initializes empty 2D vector
        }
        void add_vec(){
            bucketStarts.push_back(arr.size());
        }
        uint64_t size(){
            return bucketStarts.size();
        }
        uint64_t size(int i){
            c_size_t currSize = bucketStarts.size();
            if (i == currSize - 1){
                return arr.size() - bucketStarts[i];
            }
            return bucketStarts[i + 1] - bucketStarts[i];
        }
        ValueType& operator()(c_size_t i, c_size_t j) {
            assert(i >= 0 && i < bucketStarts.size() && j >= 0 && j < size(i));
            return arr[bucketStarts[i] + j];
        }
        const ValueType& operator()(c_size_t i, c_size_t j) const {
            assert(i >= 0 && i < bucketStarts.size() && j >= 0 && j < size(i));
            return arr[bucketStarts[i] + j];
        }
        void push_back(const ValueType& value){
            arr.push_back(value);
        }
        void push_back(const space_efficient_vector<ValueType>& vec){
            for (c_size_t i = 0; i < vec.size(); i++) arr.push_back(vec[i]);
        }
        void push_back(c_size_t i, ValueType value, space_efficient_vector<c_size_t>& bucketIndex){
            arr[bucketStarts[i] + bucketIndex[i]] = value;
            bucketIndex[i]++;
        }
        bool empty(c_size_t i){
            return size(i) == 0;
        }
        uint64_t lower_bound(c_size_t i, ValueType& search_element){
            return arr.lower_bound(search_element, bucketStarts[i], bucketStarts[i] + size(i) - 1) - bucketStarts[i];
        }
        void sort(c_size_t i){
            arr.sort(bucketStarts[i], bucketStarts[i] + size(i) - 1);
        }
};