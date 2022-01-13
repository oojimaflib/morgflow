/***********************************************************************
 * FixedValueFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_FixedValueFieldFunctor_hpp
#define FieldFunctors_FixedValueFieldFunctor_hpp

#include "../FieldFunctor.hpp"

template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class FixedValueFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = false;
  
private:

  T value_;

public:

  FixedValueFieldFunctor(const Config& config)
    : FieldFunctor(),
      value_(config.get<T>("value"))
  {
  }
  
  FixedValueFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
			 const Config& config)
    : FieldFunctor(),
      value_(config.get<T>("value"))
  {
  }
  
  FixedValueFieldFunctor(const T& value)
    : FieldFunctor(),
      value_(value)
  {}

  virtual ~FixedValueFieldFunctor(void) {}

  virtual std::string name(void) const
  {
    return "Fixed Value (" + std::to_string(value_) + ")";
  }
  
  void bind(sycl::handler& cgh) {}

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    return value_;
    (void) time;
    (void) coord;
    (void) nodata;
  }
  
  T operator()(const double& time,
	       const CoordType& coord,
	       const std::array<double,2>& box_size,
	       const T& nodata) const
  {
    return value_;
    (void) time;
    (void) nodata;
  }
  
};

#endif
