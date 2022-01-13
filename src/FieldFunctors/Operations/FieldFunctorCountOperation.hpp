/***********************************************************************
 * FieldFunctor/Operations/FieldFunctorCountOperation.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctor_Operations_FieldFunctorCountOperation_hpp
#define FieldFunctor_Operations_FieldFunctorCountOperation_hpp

template<typename T>
class FieldFunctorCountOperation
{
private:

  size_t count_;
  size_t state_;

public:

  FieldFunctorCountOperation(const T& nodata)
    : count_(0), state_(1)
  {
    (void) nodata;
  }

  size_t iterations_remaining(void)
  {
    return state_;
  }

  void append(const T& value)
  {
    count_++;
  }

  T get(void)
  {
    state_--;
    return T(count_);
  }
  
};

#endif

