/***********************************************************************
 * FieldFunctor/Operations/FieldFunctorMeanOperation.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctor_Operations_FieldFunctorMeanOperation_hpp
#define FieldFunctor_Operations_FieldFunctorMeanOperation_hpp

template<typename T>
class FieldFunctorMeanOperation
{
private:

  T nodata_;
  T accumulator_;
  size_t count_;
  size_t state_;

public:

  FieldFunctorMeanOperation(const T& nodata)
    : nodata_(nodata),
      accumulator_(T(0)),
      count_(0),
      state_(1)
  {}

  size_t iterations_remaining(void)
  {
    return state_;
  }

  void append(const T& value)
  {
    accumulator_ += value;
    count_++;
  }

  T get(void)
  {
    state_--;
    if (count_ > 0) {
      return accumulator_ / count_;
    }
    return nodata_;
  }
  
};

#endif

