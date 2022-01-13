/***********************************************************************
 * OutputDriver.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef OutputDriver_hpp
#define OutputDriver_hpp

#include "Config.hpp"
#include "OutputFormat.hpp"

template<typename Scheme>
class OutputDriver
{
public:

  using ValueType = typename Scheme::ValueType;
  using MeshType = typename Scheme::MeshType;
  
private:

  std::shared_ptr<OutputFormat<ValueType,MeshType>> format_;

  double start_time_;
  double interval_;
  size_t n_steps_;
  mutable size_t next_step_;

  std::function<std::string(const double&)> time_functor_;
  
  std::vector<std::string> function_names_;

public:

  OutputDriver(const Config& config)
    : format_(create_output_format<ValueType,MeshType>(config))
  {
    double local_time_factor =
      GlobalConfig::instance().get_time_unit_factor(config);
    if (config.count("start time") > 0) {
      start_time_ = config.get<double>("start time") * local_time_factor;
    } else {
      start_time_ = GlobalConfig::instance().get_run_parameters().start_time;
    }

    interval_ = config.get<double>("interval") * local_time_factor;

    double end_time;
    if (config.count("end time") > 0) {
      end_time = config.get<double>("end time") * local_time_factor;
    } else {
      end_time = GlobalConfig::instance().get_run_parameters().end_time;
    }

    n_steps_ = (size_t) std::lround((end_time - start_time_) / interval_);
    if (interval_ * n_steps_ <= (1.0 + end_time - start_time_)) {
      n_steps_++;
    }
    next_step_ = 0;

    time_functor_ = [=] (const double& time) -> std::string
    {
      return std::to_string(time / local_time_factor);
    };

    std::string fn_name_list = config.get<std::string>("variables", "depth");
    function_names_ = split_string<std::string>(fn_name_list, ",");

    std::cout << "Creating output driver writing: " << std::endl
	      << "  " << n_steps_ << " outputs at " << interval_ << "-second intervals." << std::endl
	      << "  first output at " << time_functor_(start_time_) << std::endl
	      << "  last output at " << time_functor_(end_time) << std::endl
	      << "  outputting: ";
    for (auto&& fn_name : function_names_) {
      std::cout << "\"" << fn_name << "\"\t";
    }
    std::cout << std::endl;
  }

  double next_output_time(void) const
  {
    if (next_step_ < n_steps_) {
      return start_time_ + next_step_ * interval_;
    } else {
      return std::numeric_limits<double>::quiet_NaN();
    }
  }

  void output(Scheme& ts) const 
  {
    double time_now = next_output_time();
    for (auto&& fn_str : function_names_) {
      std::shared_ptr<OutputFunction<ValueType,MeshType>> fn =
	ts.get_output_function(fn_str);

      format_->output(fn, time_functor_(time_now));
    }
    next_step_++;
  }
  
};

template<typename Scheme>
std::vector<OutputDriver<Scheme>> create_output_drivers(void)
{
  std::vector<OutputDriver<Scheme>> od;

  std::cout << "Initialising output drivers..." << std::endl;
  const Config& config = GlobalConfig::instance().configuration();
  auto od_crange = config.equal_range("output");
  for (auto it = od_crange.first; it != od_crange.second; ++it) {
    // std::string od_type = it->get_value<std::string>();
    // const Config& od_conf = *it;
    od.push_back(OutputDriver<Scheme>(it->second));
  }

  std::cout << "Initialised " << od.size() << " output drivers." << std::endl;
  return od;
}

#endif
