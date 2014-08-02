#pragma once

#include <memory>
#include <iterator>
#include <algorithm>

template <typename T>
class fixed_vector {
 public:
  typedef       T                               value_type;
  typedef       T&                              reference;
  typedef const T&                              const_reference;
  typedef       T*                              iterator;
  typedef const T*                              const_iterator;
  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::size_t                           size_type;
  typedef std::ptrdiff_t                        difference_type;

  fixed_vector()
      : length(0), buffer(nullptr) {}
  explicit fixed_vector(std::size_t N)
      : length(N), buffer(new T[length]) {}
  fixed_vector(const fixed_vector& o)
      : length(o.size()), buffer(new T[length]) {
    std::copy(o.begin(), o.end(), begin());
  }
  fixed_vector& operator=(fixed_vector&&) = default;
  fixed_vector(fixed_vector&&) = default;

  fixed_vector& operator=(const fixed_vector& o) {
    std::unique_ptr<T[]> tmp(new T[o.length]);
    std::copy(o.begin(), o.end(), tmp.get());
    length = o.length;
    buffer = std::move(tmp);
    return *this;
  }

  std::size_t size()     const { return length; }
  std::size_t capacity() const { return size(); }
  bool empty() const { return false; }

        T& operator[](std::size_t i)       { return data()[i]; }
  const T& operator[](std::size_t i) const { return data()[i]; }

        T& front()       { return *begin(); }
  const T& front() const { return *begin(); }
        T&  back()       { return *(begin()+size()-1); }
  const T&  back() const { return *(begin()+size()-1); }

        T*  data()       { return buffer.get(); }
  const T*  data() const { return buffer.get(); }

  iterator        begin()       { return data(); }
  const_iterator  begin() const { return data(); }
  const_iterator cbegin() const { return data(); }
  iterator          end()       { return data()+size(); }
  const_iterator    end() const { return data()+size(); }
  const_iterator   cend() const { return data()+size(); }

  reverse_iterator        rbegin()       { return {end()}; }
  const_reverse_iterator  rbegin() const { return {end()}; }
  const_reverse_iterator crbegin() const { return {end()}; }
  reverse_iterator          rend()       { return {begin()}; }
  const_reverse_iterator    rend() const { return {begin()}; }
  const_reverse_iterator   crend() const { return {begin()}; }

 private:
  std::size_t length;
  std::unique_ptr<T[]> buffer;
};



/** A sorted vector of constant capacity */
template <typename T, typename Compare = std::less<T> >
class kNN_vector {
 public:
  typedef       T                               value_type;
  typedef       T&                              reference;
  typedef const T&                              const_reference;
  typedef       T*                              iterator;
  typedef const T*                              const_iterator;
  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::size_t                           size_type;
  typedef std::ptrdiff_t                        difference_type;

  explicit kNN_vector(std::size_t N, const Compare& c = Compare())
      : length(0), comp(c,N) {}

  std::size_t size()     const { return length; }
  std::size_t capacity() const { return comp.data.size(); }
  bool empty() const { return size() == 0; }
  bool full()  const { return size() == capacity(); }
  // resize?

  const T& operator[](std::size_t i) const { return data()[i]; }

  const T& front() const { return *begin(); }
  const T&  back() const { return *(begin()+size()-1); }

  const T*  data() const { return comp.data.data(); }

  const_iterator  begin() const { return data(); }
  const_iterator cbegin() const { return data(); }
  const_iterator    end() const { return data()+size(); }
  const_iterator   cend() const { return data()+size(); }

  const_reverse_iterator  rbegin() const { return {end()}; }
  const_reverse_iterator crbegin() const { return {end()}; }
  const_reverse_iterator    rend() const { return {begin()}; }
  const_reverse_iterator   crend() const { return {begin()}; }

  void pop_back() { if(!empty()) --length; }

  /** Push @a v into the sorted vector
   * @pre capacity() > 0
   */
  void push_back(const T& v) {
    if (!full())
      insert(length++, v);
    else if (comp(v,back()))
      insert(size()-1, v);
  }

 private:
  // Starting at index i, insert v into its sorted position
  // TODO: repurpose for an insert(iterator, value) method?
  void insert(int i, const T& v) {
    for ( ; i > 0 && comp(v,comp.data[i-1]); --i)
      comp.data[i] = std::move(comp.data[i-1]);
    comp.data[i] = v;
  }

  // Empty base class optimization for trivial comparators
  struct internal : public Compare {
    internal(const Compare& _comp, std::size_t N) : Compare(_comp), data(N) {}
    fixed_vector<T> data;
  };

  std::size_t length;
  internal comp;
};
