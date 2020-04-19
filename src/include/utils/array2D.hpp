#ifndef FAST_WFC_UTILS_ARRAY2D_HPP_
#define FAST_WFC_UTILS_ARRAY2D_HPP_

#include "assert.h"
#include <vector>

#include "../../../example/src/include/color.hpp"

/**
 * Represent a 2D array.
 * The 2D array is stored in a single array, to improve cache usage.
 */
template <typename T> class Array2D {

public:
  /**
   * Height and width of the 2D array.
   */
  size_t height;
  size_t width;

  /**
   * The array containing the data of the 2D array.
   */
  std::vector<T> data;

  /**
   * Build a 2D array given its height and width.
   * All the array elements are initialized to default value.
   */
  Array2D(size_t height, size_t width) 
      : height(height), width(width), data(width * height) {}

  /**
   * Build a 2D array given its height and width.
   * All the array elements are initialized to value.
   */
  Array2D(size_t height, size_t width, T value) 
      : height(height), width(width), data(width * height, value) {}

  /**
   * Return a const reference to the element in the i-th line and j-th column.
   * i must be lower than height and j lower than width.
   */
  const T &get(size_t i, size_t j) const  {
    assert(i < height && j < width);
    return data[j + i * width];
  }

  /**
   * Return a reference to the element in the i-th line and j-th column.
   * i must be lower than height and j lower than width.
   */
  T &get(size_t i, size_t j)  {
    assert(i < height && j < width);
    return data[j + i * width];
  }

  /**
   * Return the current 2D array reflected along the x axis.
   */
  Array2D<T> reflected() const  {
    Array2D<T> result = Array2D<T>(width, height);
    for (size_t y = 0; y < height; y++) {
      for (size_t x = 0; x < width; x++) {
        result.get(y, x) = get(y, width - 1 - x);
      }
    }
    return result;
  }

  /**
   * Return the current 2D array rotated 90° anticlockwise
   */
  Array2D<T> rotated() const  {
    Array2D<T> result = Array2D<T>(width, height);
    for (size_t y = 0; y < width; y++) {
      for (size_t x = 0; x < height; x++) {
        result.get(y, x) = get(x, width - 1 - y);
      }
    }
    return result;
  }

  /**
   * Return the sub 2D array starting from (y,x) and with size (sub_width,
   * sub_height). The current 2D array is considered toric for this operation.
   */
  Array2D<T> get_sub_array(size_t y, size_t x, size_t sub_width,
                           size_t sub_height) const  {
    Array2D<T> sub_array_2d = Array2D<T>(sub_width, sub_height);
    for (size_t ki = 0; ki < sub_height; ki++) {
      for (size_t kj = 0; kj < sub_width; kj++) {
        sub_array_2d.get(ki, kj) = get((y + ki) % height, (x + kj) % width);
      }
    }
    return sub_array_2d;
  }

  /**
   * Check if two 2D arrays are equals.
   */
  bool operator==(const Array2D<T> &a) const  {
    if (height != a.height || width != a.width) {
      return false;
    }

    for (size_t i = 0; i < data.size(); i++) {
      if (a.data[i] != data[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   * Check if two 2D arrays are not equals.
   */
  bool operator!=(const Array2D<T> &a) const  {
      return !((*this) == a);
  }
};

//this Color override shouldn't be neccesary, but if I don't include it, the
//compiler can't match the original operator== to it. IDK
///**
// * Check if two 2D arrays are equals.
// */
//bool operator==(const Array2D<Color> &left, const Array2D<Color>& right) {
//    if (right.height != left.height || right.width != left.width) {
//        return false;
//    }
//
//    for (size_t i = 0; i < right.data.size(); i++) {
//        if (left.data[i] != right.data[i]) {
//            return false;
//        }
//    }
//    return true;
//}
//
/**
 * Hash function.
 */
namespace std {
template <typename T> class hash<Array2D<T>> {
public:
  size_t operator()(const Array2D<T> &a) const  {
    std::size_t seed = a.data.size();
    for (const T &i : a.data) {
      seed ^= hash<T>()(i) + (size_t)0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
} // namespace std

#endif // FAST_WFC_UTILS_ARRAY2D_HPP_
