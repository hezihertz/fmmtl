#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>

#include "fmmtl/Kernel.hpp" //Ugh
#include "fmmtl/Direct.hpp"
#include "fmmtl/numeric/random.hpp"

template <typename T, std::size_t K, typename Compare = std::less<T> >
class ordered_vector {
  static_assert(K > 0, "ordered_vector must have K > 0");

  T data[K];
  unsigned size;
  const Compare comp;

  void insert(int i, const T& t) {
    for ( ; i > 0 && comp(t,data[i-1]); --i)
      data[i] = data[i-1];
    data[i] = t;
  }

 public:
  // Default construction
  ordered_vector(const Compare& _comp = Compare())
      : size(0), comp(_comp) {
  }

  ordered_vector& operator+=(const T& t) {
    if (size < K) {
      insert(size, t);
      ++size;
    } else if (comp(t,data[K-1])) {
      insert(K-1, t);
    }
    return *this;
  }

  friend std::ostream& operator<<(std::ostream& s, const ordered_vector& ov) {
    s << "(";
    if (ov.size >= 1)
      s << ov.data[0];
    for (unsigned i = 1; i < ov.size; ++i)
      s << ", " << ov.data[i];
    return s << ")";
  }
};


template <std::size_t K>
struct kNN : public fmmtl::Kernel<kNN<K> >{
  typedef int source_type;
  typedef int target_type;
  typedef int charge_type;
  typedef ordered_vector<int,K> result_type;

  typedef int kernel_value_type;

  kernel_value_type operator()(const target_type& t,
                               const source_type& s) const {
    (void) t;
    return s;
    //return std::abs(s-t);
  }
};




int main() {
  /*
  ordered_vector<int, 5> ov;
  std::cout << ov << std::endl;

  std::vector<int> values = {5, 2, 7, 3, 6, 9, 1, 4, 10};
  for (auto&& v : values) {
    ov += v;
    std::cout << ov << std::endl;
  }
  */

  typedef kNN<5> Kernel;
  typedef typename Kernel::source_type source_type;
  typedef typename Kernel::charge_type charge_type;
  typedef typename Kernel::target_type target_type;
  typedef typename Kernel::result_type result_type;

  Kernel K;

  int N = 1000;
  int M = 1000;

  std::vector<source_type> sources(M);
  for (auto&& s : sources)
    s = fmmtl::random<source_type>::get();
  sources[3] = 3;
  sources[1] = 1;

  std::vector<charge_type> charges(M);
  for (auto&& c : charges)
    c = 1;

  std::vector<target_type> targets(N);
  for (auto&& t : targets)
    t = fmmtl::random<target_type>::get();

  std::vector<result_type> results(N);

  Direct::matvec(K, sources, charges, targets, results);

  for (auto&& r : results)
    std::cout << r << std::endl;

  return 0;
}
