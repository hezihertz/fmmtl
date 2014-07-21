#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <stack>

#include "fmmtl/Kernel.hpp" //Ugh
#include "fmmtl/Direct.hpp"
#include "fmmtl/numeric/random.hpp"
#include "fmmtl/tree/NDTree.hpp"
#include "fmmtl/tree/TreeData.hpp"
#include "fmmtl/tree/TreeRange.hpp"


template <typename T, std::size_t K, typename Compare = std::less<T> >
class ordered_vector {
  static_assert(K > 0, "ordered_vector must have K > 0");

  // Empty base class optimization for trivial comparators
  struct internal : public Compare {
    internal(const Compare& _comp) : Compare(_comp) {}
    T data[K];
  };

  internal comp;
  unsigned size;

  void insert(int i, const T& t) {
    for ( ; i > 0 && comp(t,comp.data[i-1]); --i)
      comp.data[i] = comp.data[i-1];
    comp.data[i] = t;
  }

 public:
  // Default construction
  ordered_vector(const Compare& _comp = Compare())
      : comp(_comp), size(0) {
  }

  ordered_vector& operator+=(const T& t) {
    if (size < K)
      insert(size++, t);
    else if (comp(t,comp.data[K-1]))
      insert(K-1, t);
    return *this;
  }

  operator std::vector<T>() const {
    return std::vector<T>(comp.data, comp.data + K);
  }

  bool operator==(const ordered_vector& v) {
    return std::equal(comp.data, comp.data+K, v.comp.data);
  }
  bool operator!=(const ordered_vector& v) {
    return !(*this == v);
  }

  friend std::ostream& operator<<(std::ostream& s, const ordered_vector& ov) {
    s << "(";
    if (ov.size >= 1)
      s << ov.comp.data[0];
    for (unsigned i = 1; i < ov.size; ++i)
      s << ", " << ov.comp.data[i];
    return s << ")";
  }
};


/** kNN Kernel implementing
 * r_i += K(t_i, s_j) c_j
 * where K(t_i, s_j) = ||t_i - s_j||^2
 * r_i is a sorted vector of the K smallest values seen
 * and c_j = 1 (or perhaps the original index and multiplication becomes append)
 */
template <std::size_t K>
struct kNN : public fmmtl::Kernel<kNN<K> >{
  typedef Vec<1,double> source_type;
  typedef Vec<1,double> target_type;
  typedef int    charge_type;

  typedef double kernel_value_type;
  typedef ordered_vector<double,K> result_type;

  kernel_value_type operator()(const target_type& t,
                               const source_type& s) const {
    return norm_2_sq(t-s);
  }
};




int main(int argc, char** argv) {
  int N = 1000;
  int M = 1000;
  bool checkErrors = true;

  // Parse custom command line args
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i],"-N") == 0) {
      N = atoi(argv[++i]);
    } else if (strcmp(argv[i],"-M") == 0) {
      M = atoi(argv[++i]);
    } else if (strcmp(argv[i],"-nocheck") == 0) {
      checkErrors = false;
    }
  }

  typedef kNN<5> Kernel;
  typedef typename Kernel::source_type source_type;
  typedef typename Kernel::charge_type charge_type;
  typedef typename Kernel::target_type target_type;
  typedef typename Kernel::result_type result_type;

  Kernel K;

  std::vector<source_type> sources(M);
  for (auto&& s : sources)
    s = fmmtl::random<source_type>::get();

  // TODO: Implicit values
  std::vector<charge_type> charges(M);
  for (auto&& c : charges)
    c = 1;

  std::vector<target_type> targets(N);
  for (auto&& t : targets)
    t = fmmtl::random<target_type>::get();

  std::vector<result_type> results(N);

  // Construct the source tree
  constexpr unsigned D = fmmtl::dimension<source_type>::value;
  fmmtl::NDTree<D> source_tree(sources);

  typedef fmmtl::NDTree<D>::box_type  source_box_type;
  typedef fmmtl::NDTree<D>::body_type source_body_type;

  // Hyperrectangular distance struct
  struct hyper_rect_distance {
    double hyperrect_distance;
    double farthest_distance;   // XXX: result[k].back()?
    Vec<D,double> axis;
  };

  // Associate each box of the source tree with a hyperrectangular distance
  auto h_rect = make_box_binding<hyper_rect_distance>(source_tree);
  auto p_sources = make_body_binding(sources, source_tree);
  auto p_charges = make_body_binding(charges, source_tree);

  //
  // Traversal -- Single-Tree
  //

  // For each target
  for (unsigned k = 0; k < targets.size(); ++k) {

    // Start from the root of the source tree
    std::stack<source_box_type> stack;
    stack.push(source_tree.root());

    while (!stack.empty()) {
      source_box_type sbox = stack.top();
      stack.pop();

      // If leaf, compute direct evaluation
      if (sbox.is_leaf()) {
        // TODO: Cleanup with fmmtl::direct rewrite (range/scalar comprehension)
        auto source_range = p_sources[sbox];
        auto charge_range = p_charges[sbox];
        Direct::matvec(K,
                       source_range.first, source_range.second,  // Source range
                       charge_range.first,                       // Source charge
                       targets.begin()+k, targets.begin()+k+1,   // Target range
                       results.begin()+k);                       // Target result
      } else {
        // Update intersection data h_rect[sbox]
        auto& box_data = h_rect[sbox];
        source_box_type pbox = sbox.parent();
        // box_data.hyperrect_distance = ...

        // Attempt to prune this box based on the computed data

        // Sort the children in the order we would like them traversed

        // Add children (in reserve order) to the stack for traversal
      }
    }
  }


  //
  // Complete
  //

  // Check the result
  if (checkErrors) {
    std::cout << "Computing direct matvec..." << std::endl;

    std::vector<result_type> exact(M);

    // Compute the result with a direct matrix-vector multiplication
    Direct::matvec(K, sources, charges, targets, exact);

    int wrong_results = 0;
    for (unsigned k = 0; k < results.size(); ++k) {
      if (exact[k] != results[k]) {
        std::cout << "[" << std::setw(log10(M)+1) << k << "]"
                  << " Exact: " << exact[k]
                  << ", Tree: " << results[k] << std::endl;
        ++wrong_results;
      }
    }
    std::cout << "Wrong counts: " << wrong_results << " of " << M << std::endl;
  }

  return 0;
}
