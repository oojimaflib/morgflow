/***********************************************************************
 * FieldFunctor/Operations/FieldFunctorLnStdDevOperation.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctor_Operations_FieldFunctorLnStdDevOperation_hpp
#define FieldFunctor_Operations_FieldFunctorLnStdDevOperation_hpp

template<typename T>
class FieldFunctorLnStdDevOperation
{
private:

  T nodata_;
  T mean_;
  T stddev_;
  size_t count_;
  size_t state_;

public:

  FieldFunctorLnStdDevOperation(const T& nodata)
    : nodata_(nodata),
      mean_(T(0)),
      stddev_(T(0)),
      count_(0),
      state_(2)
  {}

  size_t iterations_remaining(void)
  {
    return state_;
  }

  void append(const T& value)
  {
    switch (state_) {
    case 1:
      // Mean is known, calculate std. dev.
      stddev_ += sycl::pow(sycl::log(value) - mean_, 2);
      count_++;
      break;
    case 2:
      // Mean is not known, calculate mean.
      mean_ += sycl::log(value);
      count_++;
      break;
    default:
      return;
    };
  }

  T get(void)
  {
    state_--;
    if (count_ > 0) {
      switch (state_) {
      case 0:
	// Final answer, return the std. deviation
	return sycl::sqrt(stddev_ / count_);
      case 1:
	// Intemediate answer, return the mean
	mean_ = mean_ / count_;
	count_ = 0;
	return mean_;
      default:
	return nodata_;
      };
    } else {
      return nodata_;
    }
  }
  
};

#endif

