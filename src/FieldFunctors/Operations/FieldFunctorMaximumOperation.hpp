/***********************************************************************
 * FieldFunctor/Operations/FieldFunctorMaximumOperation.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctor_Operations_FieldFunctorMaximumOperation_hpp
#define FieldFunctor_Operations_FieldFunctorMaximumOperation_hpp

template<typename T>
class FieldFunctorMaximumOperation
{
private:

  constexpr static const T a0 = std::numeric_limits<T>::lowest();
  
  T nodata_;
  T accumulator_;
  size_t state_;

public:

  FieldFunctorMaximumOperation(const T& nodata)
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
    
    if (value > accumulator_) accumulator_ = value;
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

