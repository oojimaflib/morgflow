/***********************************************************************
 * SlopeFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_SlopeFieldFunctor_hpp
#define FieldFunctors_SlopeFieldFunctor_hpp

#include "../FieldFunctor.hpp"

template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class SlopeFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = false;
  
private:
  
  //  MeshDefn mesh_;
  
  CoordType origin_;
  CoordType slope_;
  T origin_value_;

public:

  SlopeFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
		    // const MeshDefn& mesh,
		    const Config& config)
    : FieldFunctor(),
      // mesh_(mesh),
      origin_(split_string<double,2>(config.get<std::string>("origin"))),
      slope_(split_string<double,2>(config.get<std::string>("slope"))),
      origin_value_(config.get<double>("origin value"))
  {}
  
  SlopeFieldFunctor(// const MeshDefn& mesh,
		    const CoordType& origin,
		    const CoordType& slope,
		    const T& origin_value)
    : FieldFunctor(),
      // mesh_(mesh),
      origin_(origin),
      slope_(slope),
      origin_value_(origin_value)
  {}

  virtual ~SlopeFieldFunctor(void) {}

  virtual std::string name(void) const
  {
    return "Slope";
  }
  
  void bind(sycl::handler& cgh) {}

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    double dx = coord[0] - origin_[0];
    double dy = coord[1] - origin_[1];
    return origin_value_ + dx * slope_[0] + dy * slope_[1];
    (void) time;
    (void) nodata;
  }
  
  T operator()(const double& time,
	       const CoordType& coord,
	       const std::array<double,2>& box_size,
	       const T& nodata) const
  {
    return (*this)(time, coord, nodata);
  }
  
};

#endif
