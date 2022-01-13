/***********************************************************************
 * FieldFunctor/Operations/FieldFunctorSumOperation.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctor_Operations_FieldFunctorSumOperation_hpp
#define FieldFunctor_Operations_FieldFunctorSumOperation_hpp

template<typename T>
class FieldFunctorSumOperation
{
private:

  constexpr static const T a0 = T(0);
  
  T nodata_;
  T accumulator_;
  size_t state_;

public:

  FieldFunctorSumOperation(const T& nodata)
    : nodata_(nodata), accumulator_(a0), state_(1)
  {}

  size_t iterations_remaining(void)
  {
    return state_;
  }

  void append(const T& value)
  {
    if (nodata_ != a0) {
      nodata_ = a0;
    }
    accumulator_ += value;
  }

  T get(void) {
    state_--;
    if (accumulator_ == a0) {
      return nodata_;
    }
    return accumulator_;
  }
  
};

#endif

