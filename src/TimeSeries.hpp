/***********************************************************************
 * TimeSeries.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef TimeSeries_hpp
#define TimeSeries_hpp

#include "DataArray.hpp"
#include "GlobalConfig.hpp"

#include "rapidcsv/rapidcsv.h"

#include <ctime>

template<typename T>
class TimeSeries
{
private:

  DataArray<double> time_;
  DataArray<T> value_;

public:
  
  TimeSeries(const std::shared_ptr<sycl::queue>& queue,
	     const std::vector<double>& times,
	     const std::vector<T>& values)
    : time_(queue, times),
      value_(queue, values)
  {
    for (size_t i = 0; i < time_.size(); ++i) {
      std::cout << time_.host_vector().at(i) << "\t" 
		<< value_.host_vector().at(i) << std::endl;
    }
    time_.move_to_device();
    value_.move_to_device();
  }

  const DataArray<double>& get_time_array(void) const
  {
    return time_;
  }

  const DataArray<T>& get_value_array(void) const
  {
    return value_;
  }

  static void scale_and_offset(const Config& conf,
			       std::vector<double>& times,
			       std::vector<T>& values)
  {
    double time_factor = conf.get<double>("time factor", 1.0);
    double time_offset = conf.get<double>("time offset", 0.0);
    T value_factor = conf.get<T>("value factor", 1.0);
    T value_offset = conf.get<T>("value offset", 0.0);

    for (auto&& t : times) t += time_offset;
    for (auto&& t : times) t *= time_factor;
    for (auto&& v : values) v += value_offset;
    for (auto&& v : values) v *= value_factor;
  }

  static inline bool is_reserved_key(const std::string& key)
  {
    return ((key == "source" or key == "time factor" or
	     key == "time offset" or key == "value factor" or
	     key == "value offset" or key == "time units" or
	     key == "time format" or key == "time zero"));
  }

  struct TimeParse {
    std::string time_format_str;
    bool formatted_time;
    std::time_t time_zero;
    double time_unit_factor;

    TimeParse(const Config& conf)
    {
      time_format_str = conf.get<std::string>("time format", "");
      formatted_time = (time_format_str.size() > 0);
      if (formatted_time) {
	std::istringstream tzss(conf.get<std::string>("time zero"));
	std::tm tz = {};
	tzss >> std::get_time(&tz, time_format_str.c_str());
	time_zero = std::mktime(&tz);
      }      
      time_unit_factor = GlobalConfig::instance().get_time_unit_factor(conf);
    }

    double parse(const std::string& time_str)
    {
      if (formatted_time) {
	std::istringstream ss(time_str);
	std::tm t = {};
	ss >> std::get_time(&t, time_format_str.c_str());
	std::time_t tt = std::mktime(&t);
	return double(tt - time_zero);
      } else {
	return boost::lexical_cast<double>(time_str) * time_unit_factor;
      }
    }
  };
  
  static std::shared_ptr<TimeSeries<T>>
  load_inline(const std::shared_ptr<sycl::queue>& queue,
	      const Config& conf)
  {
    using boost::algorithm::to_lower_copy;
    std::vector<double> times;
    std::vector<T> values;

    TimeParse tparse(conf);
    
    for (auto&& tsp : conf) {
      std::string key = to_lower_copy(tsp.first);
      if (not is_reserved_key(key)) {
	double time = tparse.parse(key);
	if (times.size() > 0 and time <= times.back()) {
	  std::cerr << "Times in time series must increase." << std::endl;
	  std::cerr << time << " < " << times.back() << std::endl;
	  throw std::runtime_error("Times in time series must increase.");
	}
	times.push_back(time);
	values.push_back(tsp.second.get_value<T>());
      }
    }
    scale_and_offset(conf, times, values);
    return std::make_shared<TimeSeries<T>>(queue, times, values);
  }

  static std::shared_ptr<TimeSeries<T>>
  load_csv(const std::shared_ptr<sycl::queue>& queue,
	   const Config& conf)
  {
    namespace CSV = rapidcsv;
    
    stdfs::path user_filepath = conf.get<stdfs::path>("filename");
    stdfs::path filepath;
    if (user_filepath.is_absolute()) {
      filepath = user_filepath;
    } else {
      filepath = GlobalConfig::instance().simulation_base_path()
	/ user_filepath;
    }
    
    char separator = conf.get<char>("separator", ',');
    char comment_char = conf.get<char>("comment character", '#');
    bool headers = conf.get<bool>("headers", true);
    int skip_rows = conf.get<int>("skip rows", -1);
    int skip_cols = conf.get<int>("skip cols", -1);
    if (headers) {
      skip_rows += 1;
    }

    TimeParse tparse(conf);
    
    size_t time_col;
    size_t value_col;
    
    CSV::Document doc(filepath.native(),
		      CSV::LabelParams(skip_rows, skip_cols),
		      CSV::SeparatorParams(separator, true),
		      CSV::ConverterParams(true),
		      CSV::LineReaderParams(true, comment_char, true));
    
    if (headers) {
      std::string time_header = conf.get<std::string>("time column");
      std::string value_header = conf.get<std::string>("value column");
      time_col = doc.GetColumnIdx(time_header);
      value_col = doc.GetColumnIdx(value_header);
    } else {
      time_col = conf.get<size_t>("time column", 1) - 1;
      value_col = conf.get<size_t>("value column", 2) - 1;
    }

    std::vector<std::string> time_strings =
      doc.GetColumn<std::string>(time_col);
    std::vector<double> times;
    for (auto&& ts : time_strings) {
      times.push_back(tparse.parse(ts));
    }
    std::vector<T> values = doc.GetColumn<T>(value_col);
    scale_and_offset(conf, times, values);
    return std::make_shared<TimeSeries<T>>(queue, times, values);
  }
  
};

template<typename T>
class TimeSeriesAccessor
{
private:
  
  using TimeAccessor =
    typename DataArray<double>::
    template Accessor<sycl::access::mode::read,
		      sycl::access::target::global_buffer,
		      sycl::access::placeholder::true_t>;
  using ValueAccessor =
    typename DataArray<T>::
    template Accessor<sycl::access::mode::read,
		      sycl::access::target::global_buffer,
		      sycl::access::placeholder::true_t>;

  TimeAccessor time_ro_;
  ValueAccessor values_ro_;
  
public:

  TimeSeriesAccessor(const TimeSeries<T>& ts)
    : time_ro_(ts.get_time_array().template get_placeholder_accessor<sycl::access::mode::read,sycl::access::target::global_buffer>()),
      values_ro_(ts.get_value_array().template get_placeholder_accessor<sycl::access::mode::read,sycl::access::target::global_buffer>())
  {
  }

  void bind(sycl::handler& cgh)
  {
    cgh.require(time_ro_);
    cgh.require(values_ro_);
  }
  
  T operator()(const double& time,
	       const T& nodata) const
  {
    size_t i1 = 1;
    for (; i1 < time_ro_.get_count(); ++i1) {
      if (time_ro_[i1] > time) break;
    }
    size_t i0 = i1 - 1;

    double t0 = time_ro_[i0];
    double t1 = time_ro_[i1];
    T v0 = values_ro_[i0];
    T v1 = values_ro_[i1];

    double dvdt = (v1 - v0) / (t1 - t0);
    double dt = time - t0;
    T result = v0 + dvdt * dt;
    
    return result;
  }
  
  
  
};

#endif
