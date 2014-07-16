

#include "fmmtl/tree/NDTree.hpp"

template <typename T, typename Comp = std::less<T> >
class ordered_vector {
  std::vector<T> data;

  ordered_vector() {}

  template <typename T>
  void operator+=(const T& t) {

  }
};


struct kNN {
  typedef Vec<3,double> source_type;
  typedef Vec<3,double> target_type;

  typedef double charge_type;

  typedef ordered_vector<> result_type;

  kNN(unsigned k) : k_(k) {}


};




int main() {



  return 0;
}
