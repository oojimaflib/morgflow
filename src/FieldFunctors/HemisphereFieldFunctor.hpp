/***********************************************************************
 * HemisphereFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_HemisphereFieldFunctor_hpp
#define FieldFunctors_HemisphereFieldFunctor_hpp

#include "../FieldFunctor.hpp"

template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class HemisphereFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = false;
  
private:
  
  //  MeshDefn mesh_;
  
  CoordType origin_;
  double centre_z_;
  double radius_;
  bool convex_;

public:

  HemisphereFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
			 //const MeshDefn& mesh,
			 const Config& config)
    : FieldFunctor(),
      //  mesh_(mesh),
      origin_(split_string<double,2>(config.get<std::string>("origin"))),
      centre_z_(config.get<double>("centre z")),
      radius_(config.get<double>("radius")),
      convex_(config.get<bool>("convex"))
  {}
  
  HemisphereFieldFunctor(// const MeshDefn& mesh,
			 const CoordType& origin,
			 const double& centre_z,
			 const double& radius,
			 const bool& convex)
    : FieldFunctor(),
      // mesh_(mesh),
      origin_(origin),
      centre_z_(centre_z),
      radius_(radius),
      convex_(convex)
  {}

  virtual ~HemisphereFieldFunctor(void) {}

  virtual std::string name(void) const
  {
    return "Hemisphere";
  }
  
  void bind(sycl::handler& cgh) {}

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    double dx = coord[0] - origin_[0];
    double dy = coord[1] - origin_[1];
    double distance2 = radius_ * radius_ - dx * dx - dy * dy;
    if (distance2 >= 0.0) {
      return (sycl::sqrt(distance2) + centre_z_) * (convex_ ? 1 : -1);
    } else {
      return nodata;
    }
    (void) time;
  }
  
  T operator()(const double& time,
	       const CoordType& coord,
	       const std::array<double,2>& box_size,
	       const T& nodata) const
  {
    return nodata;
    (void) time;
  }
  
};

#endif
