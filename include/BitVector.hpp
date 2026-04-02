#include "utils/space_efficient_vector.hpp"
#include "typedefs.hpp"
class BitVector{
    public:
        static constexpr c_size_t WORD_SIZE = 64;
        space_efficient_vector<uint64_t> bitVec;
        c_size_t n, wordsPerEntry;
        BitVector(){
            
        }
        BitVector(c_size_t n, c_size_t bitsPerEntry): n(n){
            wordsPerEntry = (bitsPerEntry + WORD_SIZE - 1) / WORD_SIZE;
            bitVec.resize(n * wordsPerEntry, 0);
        }
        void setEntry(c_size_t from, c_size_t to){
            for (c_size_t i = 0; i < wordsPerEntry; i++) bitVec[to * wordsPerEntry + i] = bitVec[from * wordsPerEntry + i];
        }
        void setBit(c_size_t idx, c_size_t bitPos){
            c_size_t startPos = idx * wordsPerEntry + bitPos / WORD_SIZE;
            bitVec[startPos] |= 1ULL << (bitPos % WORD_SIZE);
        }
        bool getBit(c_size_t idx, c_size_t bitPos) const {
            c_size_t startPos = idx * wordsPerEntry + bitPos / WORD_SIZE;
            return bitVec[startPos] & 1ULL << (bitPos % WORD_SIZE);
        }
        c_size_t getSuffCount(c_size_t idx, c_size_t bitPos) const {
            c_size_t idxStart = idx * wordsPerEntry;
            c_size_t startPos = idxStart + bitPos / WORD_SIZE;
            c_size_t bitCount = __builtin_popcountll(bitVec[startPos] & (~((1ULL << (bitPos % WORD_SIZE)) - 1)));
            for (c_size_t i = startPos + 1; i < idxStart + wordsPerEntry; i++) bitCount += __builtin_popcountll(bitVec[i]);
            return bitCount;
        }
};