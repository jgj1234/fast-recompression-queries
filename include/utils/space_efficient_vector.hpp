/**
 * @file    space_efficient_vector.hpp
 * @section LICENCE
 *
 * This file is part of Lazy-AVLG v0.1.0
 * See: https://github.com/dominikkempa/lz77-to-slp
 *
 * Copyright (C) 2021
 *   Dominik Kempa <dominik.kempa (at) gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 **/

#ifndef __SPACE_EFFICIENT_VECTOR_HPP_INCLUDED
#define __SPACE_EFFICIENT_VECTOR_HPP_INCLUDED

#include <cstdint>
#include <algorithm>
        #include <iostream>

#include <new> // for placement new if needed

#include "utils.hpp"

//=============================================================================
// A class that implements essentially the same functionality as std::vector,
// but with much smaller peak RAM usage during reallocation (which in most
// implementations is  not inplace). Slowdown of access it very moderate.
//=============================================================================
template<typename ValueType>
class space_efficient_vector {
  public:

    //=========================================================================
    // Declare types.
    //=========================================================================
    typedef ValueType value_type;

  private:

    //=========================================================================
    // Set the number of blocks. This controls the peak RAM use.
    //=========================================================================
    static const std::uint64_t max_blocks = 32;

    //=========================================================================
    // Class members.
    //=========================================================================
    value_type **m_blocks;
    std::uint64_t m_block_size_log;
    std::uint64_t m_block_size_mask;
    std::uint64_t m_block_size;
    std::uint64_t m_allocated_blocks;
    std::uint64_t m_cur_block_id;
    std::uint64_t m_cur_block_filled;
    std::uint64_t m_size;

  public:

    //=========================================================================
    // Constructor.
    //=========================================================================
    space_efficient_vector() {
      m_size = 0;
      m_block_size_log = 0;
      m_block_size_mask = 0;
      m_block_size = 1;
      m_allocated_blocks = 1;
      m_cur_block_filled = 0;
      m_cur_block_id = 0;
      // top-level array of pointers (use existing utils for pointer-array)
      m_blocks = utils::allocate_array<value_type*>(max_blocks);
      // allocate the first block and construct one element
      m_blocks[0] = new value_type[m_block_size];
    }

    //=========================================================================
    // Copy constructor (deep copy)
    //=========================================================================
    space_efficient_vector(const space_efficient_vector &other) {
      m_block_size_log = other.m_block_size_log;
      m_block_size_mask = other.m_block_size_mask;
      m_block_size = other.m_block_size;
      m_allocated_blocks = other.m_allocated_blocks;
      m_cur_block_id = other.m_cur_block_id;
      m_cur_block_filled = other.m_cur_block_filled;
      m_size = other.m_size;

      // allocate top-level pointer array
      m_blocks = utils::allocate_array<value_type*>(max_blocks);

      // copy blocks (copy only filled part for the last block)
      for (std::uint64_t b = 0; b < m_allocated_blocks; ++b) {
        // allocate block (value-constructed)
        m_blocks[b] = new value_type[m_block_size];
        std::uint64_t to_copy = (b == m_cur_block_id) ? m_cur_block_filled : m_block_size;
        // copy initialized entries; if other blocks are full this copies whole block
        for (std::uint64_t k = 0; k < to_copy; ++k)
          m_blocks[b][k] = other.m_blocks[b][k];
        // initialize rest deterministically
        for (std::uint64_t k = to_copy; k < m_block_size; ++k)
          m_blocks[b][k] = value_type();
      }
      // set remaining top-level pointers to nullptr for safety
      for (std::uint64_t b = m_allocated_blocks; b < max_blocks; ++b)
        m_blocks[b] = nullptr;
    }

    //=========================================================================
    // Copy assignment (deep copy + self-assign safe)
    //=========================================================================
    space_efficient_vector& operator=(const space_efficient_vector &other) {
      if (this == &other) return *this;

      // free existing data
      if (m_blocks) {
        for (std::uint64_t block_id = 0; block_id < m_allocated_blocks; ++block_id)
          if (m_blocks[block_id]) delete[] m_blocks[block_id];
        utils::deallocate(m_blocks);
      }

      // copy the scalar fields
      m_block_size_log = other.m_block_size_log;
      m_block_size_mask = other.m_block_size_mask;
      m_block_size = other.m_block_size;
      m_allocated_blocks = other.m_allocated_blocks;
      m_cur_block_id = other.m_cur_block_id;
      m_cur_block_filled = other.m_cur_block_filled;
      m_size = other.m_size;

      // allocate and copy blocks
      m_blocks = utils::allocate_array<value_type*>(max_blocks);
      for (std::uint64_t b = 0; b < m_allocated_blocks; ++b) {
        m_blocks[b] = new value_type[m_block_size];
        std::uint64_t to_copy = (b == m_cur_block_id) ? m_cur_block_filled : m_block_size;
        for (std::uint64_t k = 0; k < to_copy; ++k)
          m_blocks[b][k] = other.m_blocks[b][k];
        for (std::uint64_t k = to_copy; k < m_block_size; ++k)
          m_blocks[b][k] = value_type();
      }
      for (std::uint64_t b = m_allocated_blocks; b < max_blocks; ++b)
        m_blocks[b] = nullptr;

      return *this;
    }

    //=========================================================================
    // Move constructor (steal internals)
    //=========================================================================
    space_efficient_vector(space_efficient_vector &&other) noexcept {
      m_blocks = other.m_blocks;
      m_block_size_log = other.m_block_size_log;
      m_block_size_mask = other.m_block_size_mask;
      m_block_size = other.m_block_size;
      m_allocated_blocks = other.m_allocated_blocks;
      m_cur_block_id = other.m_cur_block_id;
      m_cur_block_filled = other.m_cur_block_filled;
      m_size = other.m_size;

      // put other into a safe empty state
      other.m_blocks = nullptr;
      other.m_block_size_log = 0;
      other.m_block_size_mask = 0;
      other.m_block_size = 1;
      other.m_allocated_blocks = 0;
      other.m_cur_block_id = 0;
      other.m_cur_block_filled = 0;
      other.m_size = 0;
    }

    //=========================================================================
    // Move assignment (steal + cleanup)
    //=========================================================================
    space_efficient_vector& operator=(space_efficient_vector &&other) noexcept {
      if (this == &other) return *this;

      // free existing content
      if (m_blocks) {
        for (std::uint64_t block_id = 0; block_id < m_allocated_blocks; ++block_id)
          if (m_blocks[block_id]) delete[] m_blocks[block_id];
        utils::deallocate(m_blocks);
      }

      // steal internals
      m_blocks = other.m_blocks;
      m_block_size_log = other.m_block_size_log;
      m_block_size_mask = other.m_block_size_mask;
      m_block_size = other.m_block_size;
      m_allocated_blocks = other.m_allocated_blocks;
      m_cur_block_id = other.m_cur_block_id;
      m_cur_block_filled = other.m_cur_block_filled;
      m_size = other.m_size;

      // reset other
      other.m_blocks = nullptr;
      other.m_block_size_log = 0;
      other.m_block_size_mask = 0;
      other.m_block_size = 1;
      other.m_allocated_blocks = 0;
      other.m_cur_block_id = 0;
      other.m_cur_block_filled = 0;
      other.m_size = 0;

      return *this;
    }

    //=========================================================================
    // Constructor with initial size.
    //=========================================================================
    space_efficient_vector(
        const std::uint64_t initial_size,
        const value_type &initial_val = value_type()) {
#if 0
      // Slow
      m_size = 0;
      m_block_size = 1;
      m_block_size_log = 0;
      m_block_size_mask = 0;
      m_allocated_blocks = 1;
      m_cur_block_id = 0;
      m_cur_block_filled = 0;
      m_blocks = utils::allocate_array<value_type*>(max_blocks);
      m_blocks[0] = new value_type[1];
      for (std::uint64_t i = 0; i < initial_size; ++i)
        push_back(initial_val);
#else
      // Fast (but ensure we construct elements)
      m_block_size = 1;
      m_block_size_log = 0;
      m_block_size_mask = 0;
      while (m_block_size * max_blocks < initial_size) {
        ++m_block_size_log;
        m_block_size <<= 1;
        m_block_size_mask = m_block_size - 1;
      }
      m_allocated_blocks = (initial_size + m_block_size - 1) / m_block_size;
      m_cur_block_id = m_allocated_blocks - 1;
      m_cur_block_filled = initial_size - (m_allocated_blocks - 1) * m_block_size;
      m_blocks = utils::allocate_array<value_type*>(max_blocks);
      for (std::uint64_t i = 0; i < m_allocated_blocks; ++i) {
        // allocate and construct via new[]
        m_blocks[i] = new value_type[m_block_size];
        // fill constructed elements with initial_val
        std::uint64_t to_fill = (i == m_cur_block_id) ? m_cur_block_filled : m_block_size;
        for (std::uint64_t k = 0; k < to_fill; ++k)
          m_blocks[i][k] = initial_val;
        // initialize remaining elements deterministically
        for (std::uint64_t k = to_fill; k < m_block_size; ++k)
          m_blocks[i][k] = value_type();
      }
      m_size = initial_size;
#endif
    }

    //=========================================================================
    // Destructor.
    //=========================================================================
    ~space_efficient_vector() {
      if (m_blocks) {
        for (std::uint64_t block_id = 0;
            block_id < m_allocated_blocks; ++block_id)
          if (m_blocks[block_id]) delete[] m_blocks[block_id];
        utils::deallocate(m_blocks);
      }
    }

    inline std::uint64_t size() const {
      return m_size;
    }

    inline bool empty() const {
      return m_size == 0;
    }

    void set_empty() {
      m_size = 0;
      m_cur_block_filled = 0;
      m_cur_block_id = 0;
    }
    inline void pop_back() {
      --m_size;
      --m_cur_block_filled;
      if (m_cur_block_filled == 0 && m_cur_block_id > 0) {
        --m_cur_block_id;
        m_cur_block_filled = m_block_size;
      }
    }
    inline void push_back(const value_type &value) {
      if (m_cur_block_filled == m_block_size &&
          m_cur_block_id + 1 == max_blocks) {
        const std::uint64_t new_block_size = m_block_size * 2;
        for (std::uint64_t block_id = 0;
            block_id < m_allocated_blocks; block_id += 2) {
          value_type *newblock = new value_type[new_block_size];
          std::copy(m_blocks[block_id],
              m_blocks[block_id] + m_block_size, newblock);
          std::copy(m_blocks[block_id + 1],
              m_blocks[block_id + 1] + m_block_size,
              newblock + m_block_size);
          delete[] m_blocks[block_id];
          delete[] m_blocks[block_id + 1];
          m_blocks[block_id / 2] = newblock;
        }
        m_allocated_blocks = max_blocks / 2;
        m_block_size = new_block_size;
        m_block_size_mask = new_block_size - 1;
        ++m_block_size_log;
        m_cur_block_id = m_allocated_blocks - 1;
        m_cur_block_filled = new_block_size;
      }

      if (m_cur_block_filled == m_block_size) {
        ++m_cur_block_id;
        m_cur_block_filled = 0;
        if (m_cur_block_id == m_allocated_blocks) {
          ++m_allocated_blocks;
          m_blocks[m_cur_block_id] = new value_type[m_block_size];
        }
      }

      m_blocks[m_cur_block_id][m_cur_block_filled++] = value;
      ++m_size;
    }

    void resize(
        const std::uint64_t newsize,
        const value_type &initial_val = value_type()) {

      for (std::uint64_t block_id = 0;
          block_id < m_allocated_blocks; ++block_id)
        if (m_blocks[block_id]) delete[] m_blocks[block_id];
      utils::deallocate(m_blocks);

#if 0
      for (std::uint64_t i = 0; i < newsize; ++i)
        push_back(initial_val);
#else
      m_block_size = 1;
      m_block_size_log = 0;
      m_block_size_mask = 0;
      while (m_block_size * max_blocks < newsize) {
        ++m_block_size_log;
        m_block_size <<= 1;
        m_block_size_mask = m_block_size - 1;
      }
      m_allocated_blocks = (newsize + m_block_size - 1) / m_block_size;
      m_cur_block_id = m_allocated_blocks - 1;
      m_cur_block_filled = newsize - (m_allocated_blocks - 1) * m_block_size;
      m_blocks = utils::allocate_array<value_type*>(max_blocks);
      for (std::uint64_t i = 0; i < m_allocated_blocks; ++i) {
        m_blocks[i] = new value_type[m_block_size];
        std::uint64_t to_fill = (i == m_cur_block_id) ? m_cur_block_filled : m_block_size;
        for (std::uint64_t k = 0; k < to_fill; ++k)
          m_blocks[i][k] = initial_val;
        for (std::uint64_t k = to_fill; k < m_block_size; ++k)
          m_blocks[i][k] = value_type();
      }
      m_size = newsize;
#endif
    }

    void reserve(const std::uint64_t size_requested) {
      for (std::uint64_t block_id = 0;
          block_id < m_allocated_blocks; ++block_id)
        if (m_blocks[block_id]) delete[] m_blocks[block_id];
      utils::deallocate(m_blocks);

      m_block_size = 1;
      m_block_size_log = 0;
      m_block_size_mask = 0;
      while (m_block_size * max_blocks < size_requested) {
        ++m_block_size_log;
        m_block_size <<= 1;
        m_block_size_mask = m_block_size - 1;
      }
      m_allocated_blocks = (size_requested + m_block_size - 1) / m_block_size;
      m_blocks = utils::allocate_array<value_type*>(max_blocks);
      for (std::uint64_t i = 0; i < m_allocated_blocks; ++i)
        m_blocks[i] = new value_type[m_block_size];

      m_cur_block_id = 0;
      m_cur_block_filled = 0;
      m_size = 0;
    }

    inline value_type& front() {
      return m_blocks[0][0];
    }

    inline const value_type& front() const {
      return m_blocks[0][0];
    }

    // TODO: Can add functionality to search in a range.
    uint64_t lower_bound(value_type& search_element)
    {
      uint64_t low = 0;
      uint64_t high = size() - 1;

      while(high - low > 1) {
          uint64_t mid = low + (high - low)/2;

          if((*this)[mid] < search_element) {
              low = mid + 1;
          }
          else {
              high = mid;
          }
      }

      if(search_element <= (*this)[low]) return low;
      if(search_element <= (*this)[high]) return high;

      return size();
    }

    //=========================================================================
    // Return the reference to the last element.
    //=========================================================================
    inline value_type& back() {
      return m_blocks[m_cur_block_id][m_cur_block_filled - 1];
    }

    //=========================================================================
    // Return the reference to the last element.
    //=========================================================================
    inline const value_type& back() const {
      return m_blocks[m_cur_block_id][m_cur_block_filled - 1];
    }

    //=========================================================================
    // Access operator.
    //=========================================================================
    inline value_type& operator[] (const std::uint64_t i) {
      const std::uint64_t block_id = (i >> m_block_size_log);
      const std::uint64_t block_offset = (i & m_block_size_mask);
      //const std::uint64_t block_id = (i / m_block_size);
      //const std::uint64_t block_offset = (i % m_block_size);
      return m_blocks[block_id][block_offset];
    }

    //=========================================================================
    // Access operator.
    //=========================================================================
    inline const value_type& operator[] (const std::uint64_t i) const {
      const std::uint64_t block_id = (i >> m_block_size_log);
      const std::uint64_t block_offset = (i & m_block_size_mask);
      //const std::uint64_t block_id = (i / m_block_size);
      //const std::uint64_t block_offset = (i % m_block_size);
      return m_blocks[block_id][block_offset];
    }

    //=========================================================================
    // Remove all elements and deallocate.
    //=========================================================================
    void clear() {
      for (std::uint64_t block_id = 0;
          block_id < m_allocated_blocks; ++block_id)
        if (m_blocks[block_id]) delete[] m_blocks[block_id];

      m_size = 0;
      m_block_size_log = 0;
      m_block_size_mask = 0;
      m_block_size = 1;
      m_allocated_blocks = 1;
      m_cur_block_filled = 0;
      m_cur_block_id = 0;
      // allocate a fresh first block
      m_blocks[0] = new value_type[m_block_size];
    }

    //=========================================================================
    // Write contents to given file.
    //=========================================================================
    void write_to_file(const std::string filename) const {
      std::FILE * const f = utils::file_open_nobuf(filename, "w");
      for (std::uint64_t block_id = 0; block_id < m_cur_block_id; ++block_id)
        utils::write_to_file(m_blocks[block_id], m_block_size, f);
      if (m_cur_block_filled > 0)
        utils::write_to_file(m_blocks[m_cur_block_id], m_cur_block_filled, f);
      std::fclose(f);
    }

    //=========================================================================
    // Return used RAM.
    //=========================================================================
    std::uint64_t ram_use() const {
      const std::uint64_t blocks_ram =
        sizeof(value_type) * m_allocated_blocks * m_block_size;
      const std::uint64_t total = blocks_ram;
      return total;
    }

    //=========================================================================
    // Reverse the vector.
    //=========================================================================
    void reverse() {
      const std::uint64_t all = size();
      const std::uint64_t half = all / 2;
      for (std::uint64_t i = 0; i < half; ++i)
        std::swap((*this)[i], (*this)[all - 1 - i]);
    }

  private:

    inline void swap(
        const std::uint64_t i,
        const std::uint64_t j) {
      value_type ival = (*this)[i];
      value_type jval = (*this)[j];
      (*this)[i] = jval;
      (*this)[j] = ival;
    }
    template<typename Compare>
    std::uint64_t partition(
        const std::int64_t p,
        const std::int64_t r,
        Compare comp) {
      std::int64_t i = p - 1;
      std::int64_t j = r + 1;
      std::int64_t pivot_idx = utils::random_int<std::uint64_t>(
          (std::uint64_t)p,
          (std::uint64_t)r);
      value_type pivot = (*this)[pivot_idx];
      this->swap(p, pivot_idx);
      while (i < j) {
        while (comp((*this)[++i],  pivot));
        while (comp(pivot, (*this)[--j]));
        if (i < j) this->swap(i, j);
      }
      return (std::uint64_t)j;
    }
    template<typename Compare>
    void sort(
        const std::uint64_t p,
        const std::uint64_t r,
        Compare comp) {
      if (p < r) {
        const std::uint64_t q = partition(p, r, comp);
        sort(p, q, comp);
        sort(q + 1, r, comp);
      }
    }
    std::uint64_t partition(
        const std::int64_t p,
        const std::int64_t r) {
      std::int64_t i = p - 1;
      std::int64_t j = r + 1;
      std::int64_t pivot_idx = utils::random_int<std::uint64_t>(
          (std::uint64_t)p,
          (std::uint64_t)r);
      value_type pivot = (*this)[pivot_idx];
      this->swap(p, pivot_idx);
      while (i < j) {
        while ((*this)[++i] < pivot);
        while (pivot < (*this)[--j]);
        if (i < j) this->swap(i, j);
      }
      return (std::uint64_t)j;
    }

    void sort(
        const std::uint64_t p,
        const std::uint64_t r) {
      if (p < r) {
        const std::uint64_t q = partition(p, r);
        sort(p, q);
        sort(q + 1, r);
      }
    }

  public:
    template<typename Compare>
    void sort(Compare comp) {
#if 1
      /*std::vector<value_type> before_sort;
      for (std::uint64_t i = 0; i < m_size; ++i)
        before_sort.push_back((*this)[i]);*/

      if (m_size > 0)
        sort(0, m_size - 1, comp);

      /*std::vector<value_type> after_sort;
      for (std::uint64_t i = 0; i < m_size; ++i)
        after_sort.push_back((*this)[i]);
      std::sort(before_sort.begin(), before_sort.end());

      bool sorted = true;
      for (std::int64_t i = 0; i + 1 < m_size; ++i) {
        if (after_sort[i + 1] < after_sort[i]) {
          sorted = false;
          break;
        }
      }

      std::sort(after_sort.begin(), after_sort.end());
      bool eq = (after_sort == before_sort);*/

      /*if (!eq || !sorted) {
        fprintf(stderr, "\nChecks failed:\n");
        fprintf(stderr, "\teq = %lu\n", (std::uint64_t)eq);
        fprintf(stderr, "\tsorted = %lu\n", (std::uint64_t)sorted);
        std::exit(EXIT_FAILURE);
      }*/
#else
      for (std::uint64_t i = 1; i < m_size; ++i) {
        std::uint64_t j = i;
        while (j > 0 && (*this)[j] < (*this)[j - 1]) {
          this->swap(j - 1, j);
          --j;
        }
      }
#endif
    }
    void sort() {
#if 1
      /*std::vector<value_type> before_sort;
      for (std::uint64_t i = 0; i < m_size; ++i)
        before_sort.push_back((*this)[i]);*/

      if (m_size > 0)
        sort(0, m_size - 1);

      /*std::vector<value_type> after_sort;
      for (std::uint64_t i = 0; i < m_size; ++i)
        after_sort.push_back((*this)[i]);
      std::sort(before_sort.begin(), before_sort.end());

      bool sorted = true;
      for (std::int64_t i = 0; i + 1 < m_size; ++i) {
        if (after_sort[i + 1] < after_sort[i]) {
          sorted = false;
          break;
        }
      }

      std::sort(after_sort.begin(), after_sort.end());
      bool eq = (after_sort == before_sort);*/

      /*if (!eq || !sorted) {
        fprintf(stderr, "\nChecks failed:\n");
        fprintf(stderr, "\teq = %lu\n", (std::uint64_t)eq);
        fprintf(stderr, "\tsorted = %lu\n", (std::uint64_t)sorted);
        std::exit(EXIT_FAILURE);
      }*/
#else
      for (std::uint64_t i = 1; i < m_size; ++i) {
        std::uint64_t j = i;
        while (j > 0 && (*this)[j] < (*this)[j - 1]) {
          this->swap(j - 1, j);
          --j;
        }
      }
#endif
    }
};

#endif  // __SPACE_EFFICIENT_VECTOR_HPP_INCLUDED