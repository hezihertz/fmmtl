#include <iostream>

#include "kNN_vector.hpp"

template <typename T>
std::ostream& operator<<(std::ostream& s, const kNN_vector<T>& v) {
  s << "(";
  if (v.size() >= 1)
    s << v[0];
  for (unsigned i = 1; i < v.size(); ++i)
    s << ", " << v[i];
  return s << ")";
}

int main() {
  kNN_vector<int> kvec(5);

  std::cout << kvec << std::endl;

  std::vector<int> data = {5,1,8,3,4,5,7,0,10};
  for (auto v : data) {
    kvec.push_back(v);
    std::cout << kvec << std::endl;
  }

  kNN_vector<int> othervec = kvec;

  std::cout << othervec << std::endl;

  othervec.pop_back();

  std::cout << othervec << std::endl;

  kvec = othervec;

  std::cout << kvec << std::endl;

  return 0;
}
