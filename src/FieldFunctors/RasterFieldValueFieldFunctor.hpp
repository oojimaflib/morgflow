/***********************************************************************
 * RasterFieldValueFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_RasterFieldValueFieldFunctor_hpp
#define FieldFunctors_RasterFieldValueFieldFunctor_hpp

#include "../FieldFunctor.hpp"
#include "../RasterField.hpp"

template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class RasterFieldValueFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = false;
  
private:

  RasterFieldAccessor<T> rf_;

public:

  RasterFieldValueFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
			       const Config& config)
    : FieldFunctor(),
      rf_(*(GlobalConfig::instance().get_raster_field_ptr<T>(queue, config.get<std::string>("raster field"))))
  {
  }

  virtual ~RasterFieldValueFieldFunctor(void) {}
  
  virtual std::string name(void) const
  {
    return "Raster Field";
  }

  void bind(sycl::handler& cgh) {
    rf_.bind(cgh);
  }

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    return rf_.inspect_point(coord, nodata);
    (void) time;
  }
  
  T operator()(const double& time,
	       const CoordType& coord,
	       const std::array<double,2>& box_size,
	       const T& nodata) const
  {
    return rf_.template inspect_box<FFOp>(coord, box_size, nodata);
    (void) time;
  }
  
};

#endif

