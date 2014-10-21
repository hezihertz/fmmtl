#include <iostream>
#include <vector>
#include <array>
#include <functional>
#include <algorithm>
#include <stack>

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
    internal(const Compare& comp, unsigned size)
        : Compare(comp), size_(size) {}
    unsigned size_;
    std::array<T,K> data_;
  };
  internal int_;


  void insert(int i, const T& v) {
    for ( ; i > 0 && int_(v,int_.data_[i-1]); --i)
      int_.data_[i] = int_.data_[i-1];
    int_.data_[i] = v;
  }

 public:
  using value_type = T;
  using const_iterator = typename std::array<T,K>::const_iterator;

  // Default construction
  ordered_vector(const Compare& comp = Compare())
      : int_(comp, 0) {
  }

  const value_type& operator[](unsigned i) const {
    return int_.data_[i];
  }
  const_iterator begin() const {
    return int_.data_.begin();
  }
  const_iterator end() const {
    return int_.data_.begin() + int_.size_;
  }

  ordered_vector& operator+=(const T& v) {
    if (int_.size_ < K)
      insert(int_.size_++, v);
    else if (int_(v,int_.data_[K-1]))
      insert(K-1, v);
    return *this;
  }

  operator std::vector<T>() const {
    return std::vector<T>(begin(), end());
  }

  bool operator==(const ordered_vector& v) {
    return std::equal(begin(), end(), v.begin());
  }
};

template <class T, std::size_t K, class C>
std::ostream& operator<<(std::ostream& s, const ordered_vector<T,K,C>& ov) {
  s << "(";
  auto first = ov.begin();
  auto last = ov.end();
  if (first < last)
    s << *first;
  for (++first; first < last; ++first)
    s << ", " << *first;
  return s << ")";
}


/** kNN Kernel implementing
 * r_i += K(t_i, s_j) c_j
 * where K(t_i, s_j) = ||t_i - s_j||^2
 * r_i is a sorted vector of the K smallest values seen
 * and c_j = 1 (or perhaps the original index and multiplication becomes append)
 */
template <std::size_t K>
struct kNN {
  typedef Vec<1,double> source_type;
  typedef Vec<1,double> target_type;
  typedef unsigned      charge_type;

  struct dist_idx_pair {
    double distance_sq;
    unsigned index;
    bool operator<(const dist_idx_pair& other) const {
      return distance_sq < other.distance_sq;
    }
    bool operator==(const dist_idx_pair& other) const {
      return distance_sq == other.distance_sq && index == other.index;
    }
    friend std::ostream& operator<<(std::ostream& s, const dist_idx_pair& dip) {
      return s << "(" << dip.distance_sq << ", " << dip.index << ")";
    }
  };
  struct kernel_value_type {
    double distance_sq;
    dist_idx_pair operator*(const charge_type& c) const {
      return {distance_sq, c};
    }
  };

  typedef ordered_vector<dist_idx_pair,K> result_type;

  kernel_value_type operator()(const target_type& t,
                               const source_type& s) const {
    return {norm_2_sq(t-s)};
  }
};




/**
 * Traverse the binary tree while discarding regions of space
 * with hypersphere-hyperrectange intersections.
 */
template <typename Box, typename Prune, typename Base, typename Visit>
void traverse(const Box& b,
              Prune& prune, Base& base_case, Visit& visit_order) {
  if (prune(b))
    return;

  if (b.is_leaf()) {
    base_case(b);
  } else {
    for (const Box& child : visit_order(b))
      traverse(child, prune, base_case, visit_order);
  }
}


// Quicky typemap from Box child range to Box range
template <typename Box>
struct ChildRange {
  const Box& b;
  auto begin() -> decltype(b.child_begin()) { return b.child_begin(); }
  auto end()   -> decltype(b.child_end())   { return b.child_end();   }
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

  // Define the kernel types
  using Kernel = kNN<5>;
  using source_type = typename Kernel::source_type;
  using charge_type = typename Kernel::charge_type;
  using target_type = typename Kernel::target_type;
  using result_type = typename Kernel::result_type;

  // Define the tree types
  constexpr unsigned DS = fmmtl::dimension<source_type>::value;
  constexpr unsigned DT = fmmtl::dimension<target_type>::value;
  using SourceTree = fmmtl::NDTree<DS>;
  using TargetTree = fmmtl::NDTree<DT>;
  using source_box_type  = typename SourceTree::box_type;
  using source_body_type = typename SourceTree::body_type;
  using target_box_type  = typename TargetTree::box_type;
  using target_body_type = typename TargetTree::body_type;

  // Construct the kernel
  Kernel K;

  // Construct the kernel data
  std::vector<source_type> sources = fmmtl::random_n(M);
  std::vector<charge_type> charges(M);

  std::vector<target_type> targets = fmmtl::random_n(N);
  std::vector<result_type> results(N);

  // Charges are the indices of the original sources
  std::iota(charges.begin(), charges.end(), 0);

  // Construct the source tree
  SourceTree source_tree(sources);


  // Hyperrectangular distance struct
  struct hyper_rect_distance {
    //double hyperrect_distance;
    //double farthest_distance;   // XXX: result[k].back()?
    //Vec<D,double> axis;
  };

  // Associate each box of the source tree with a hyperrectangular distance
  auto h_rect = make_box_binding<hyper_rect_distance>(source_tree);
  // Permute the sources and charges
  auto p_sources = make_body_binding(source_tree, sources);
  auto p_charges = make_body_binding(source_tree, charges);

  //
  // Traversal -- Single-Tree
  //

  // For each target
  for (unsigned k = 0; k < targets.size(); ++k) {
    target_type& t = targets[k];
    result_type& r = results[k];

    // Construct a rule for this target
    auto prune = [&](const source_box_type&) {
      return false;
    };
    auto base = [&](const source_box_type& b) {
      // For all the sources/charges of this box
      auto ci = p_charges[b.body_begin()];
      for (source_type& s : p_sources[b]) {
        r += K(t,s) * (*ci);
        ++ci;
      }
    };
    auto visit = [&](const source_box_type& b) {
      return ChildRange<source_box_type>{b};
    };

    traverse(source_tree.root(), prune, base, visit);
  }


  //
  // Complete
  //

  // Check the result
  if (checkErrors) {
    std::cout << "Computing direct matvec..." << std::endl;

    std::vector<result_type> exact(M);

    // Compute the result with a direct matrix-vector multiplication
    fmmtl::direct(K, sources, charges, targets, exact);

    int wrong_results = 0;
    for (unsigned k = 0; k < results.size(); ++k) {
      if (!(exact[k] == results[k])) {
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
