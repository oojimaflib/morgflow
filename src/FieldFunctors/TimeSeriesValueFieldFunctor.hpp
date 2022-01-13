/***********************************************************************
 * TimeSeriesValueFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_TimeSeriesValueFieldFunctor_hpp
#define FieldFunctors_TimeSeriesValueFieldFunctor_hpp

#include "../FieldFunctor.hpp"
#include "../TimeSeries.hpp"

template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class TimeSeriesValueFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = false;
  
private:

  TimeSeriesAccessor<T> ts_;

public:

  TimeSeriesValueFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
			      const Config& config)
    : FieldFunctor(),
      ts_(*(GlobalConfig::instance().get_time_series_ptr(queue, config.get<std::string>("series"))))
  {
  }

  virtual ~TimeSeriesValueFieldFunctor(void) {}
  
  virtual std::string name(void) const
  {
    return "Time Series";
  }
  
  void bind(sycl::handler& cgh) {
    ts_.bind(cgh);
  }

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    return ts_(time, nodata);
    (void) coord;
  }
  
  T operator()(const double& time,
	       const CoordType& coord,
	       const std::array<double,2>& box_size,
	       const T& nodata) const
  {
    return ts_(time, nodata);
  }
  
};

#endif
