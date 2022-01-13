/***********************************************************************
 * InterpolatedTimeSeriesValueFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_InterpolatedTimeSeriesValueFieldFunctor_hpp
#define FieldFunctors_InterpolatedTimeSeriesValueFieldFunctor_hpp

#include "../FieldFunctor.hpp"
#include "../TimeSeries.hpp"

template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class InterpolatedTimeSeriesValueFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = false;
  
private:

  struct LocatedTimeSeries {
    TimeSeriesAccessor<T> ts;
    CoordType loc;

    LocatedTimeSeries(const std::shared_ptr<sycl::queue>& queue,
		      const std::string& series_name,
		      const CoordType& location)
      : ts(*(GlobalConfig::instance().get_time_series_ptr(queue, series_name))),
	loc(location)
    {}
    
  };

  std::vector<LocatedTimeSeries> lts_;

public:

  InterpolatedTimeSeriesValueFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
					  const Config& config)
    : FieldFunctor(), lts_()
  {
    auto erange = config.equal_range("at");
    for (auto it = erange.first; it != erange.second; ++it) {
      CoordType loc = split_string<double,2>(it->second.get_value<std::string>());
      std::string series_name = it->second.get<std::string>("series");
      lts_.push_back(LocatedTimeSeries(queue, series_name, loc));
    }
  }

  virtual ~InterpolatedTimeSeriesValueFieldFunctor(void) {}

  virtual std::string name(void) const
  {
    return "Interpolated Time Series";
  }

  void bind(sycl::handler& cgh) {
    for (auto&& lts : lts_) {
      lts.ts.bind(cgh);
    }
  }

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    T weighted_value = 0.0;
    double total_weight = 0.0;
    for (auto&& lts : lts_) {
      const TimeSeriesAccessor<T>& tsa = lts.ts;
      const CoordType& loc = lts.loc;
      T value = tsa(time, nodata);

      double xdist = coord[0] - loc[0];
      double ydist = coord[1] - loc[1];
      double d2 = xdist*xdist + ydist*ydist;

      if (d2 < 1e-4) {
	return value;
      }

      double weight = 1.0 / d2;
      weighted_value += weight * value;
      total_weight += weight;
    }
    return weighted_value / total_weight;
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
