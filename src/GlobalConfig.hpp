/***********************************************************************
 * GlobalConfig.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef GlobalConfig_hpp
#define GlobalConfig_hpp

#include "Config.hpp"

#include "Display/DisplayTable.hpp"
//#include "TimeSeries.hpp"

template<typename T>
class TimeSeries;

// class RasterFieldBase;
template<typename T>
class RasterField;

#include "sycl.hpp"

#include <optional>

class GlobalConfig
{
private:

  void load_time_series(const std::shared_ptr<sycl::queue>& queue,
			const std::string& name);

  template<typename T>
  void load_raster_field(const std::shared_ptr<sycl::queue>& queue,
			 std::map<std::string, std::shared_ptr<RasterField<T>>>& field_map,
			 const std::string& name);
  
public:

  struct DeviceParameters
  {
    size_t platform_id;
    size_t device_id;

    sycl::platform platform;
    sycl::device device;

    DeviceParameters(GlobalConfig* gconf);
  };
  
  struct RunParameters
  {
    double start_time;
    double end_time;
    double sync_step;
    
    size_t display_every;

    RunParameters(GlobalConfig* gconf);
  };

  struct TimestepParameters
  {
    enum class DtType {
      undefined,
      fixed,
      adaptive
    } dt_type;
    double time_step;
    double max_time_step;
    double courant_target;

    Config ddt_scheme_config;

    TimestepParameters(GlobalConfig* gconf);
  };
  
protected:

  GlobalConfig(int argc, char* argv[]);

  static GlobalConfig* global_config_;

  stdfs::path config_filename_;
  stdfs::path simulation_base_path_;
  Config config_;

  double global_time_factor_;

  std::shared_ptr<DeviceParameters> device_params_;
  std::shared_ptr<RunParameters> run_params_;
  std::shared_ptr<TimestepParameters> dt_params_;

  std::map<std::string, std::shared_ptr<TimeSeries<float>>> time_series_;
  // std::map<std::string, std::shared_ptr<RasterField<float>>> raster_fields_;
  std::tuple< std::map<std::string, std::shared_ptr<RasterField<float>>>,
	      std::map<std::string, std::shared_ptr<RasterField<double>>>,
	      std::map<std::string, std::shared_ptr<RasterField<int32_t>>>,
	      std::map<std::string, std::shared_ptr<RasterField<uint32_t>>>> raster_fields_;

  template<typename T>
  std::map<std::string, std::shared_ptr<RasterField<T>>>& get_raster_field_map(void);

  template<>
  std::map<std::string, std::shared_ptr<RasterField<float>>>& get_raster_field_map(void)
  {
    return std::get<0>(raster_fields_);
  }
  template<>
  std::map<std::string, std::shared_ptr<RasterField<double>>>& get_raster_field_map(void)
  {
    return std::get<1>(raster_fields_);
  }
  template<>
  std::map<std::string, std::shared_ptr<RasterField<int32_t>>>& get_raster_field_map(void)
  {
    return std::get<2>(raster_fields_);
  }
  template<>
  std::map<std::string, std::shared_ptr<RasterField<uint32_t>>>& get_raster_field_map(void)
  {
    return std::get<3>(raster_fields_);
  }
  
  static std::map<std::string, double> time_unit_factors_;

public:
  
  GlobalConfig(GlobalConfig&) = delete;
  void operator=(const GlobalConfig&) = delete;

  static GlobalConfig& instance(void);
  static void init(int argc, char* argv[]);

  const stdfs::path& simulation_base_path(void) const
  {
    return simulation_base_path_;
  }

  const std::string name(void) const
  {
    if (config_.count("name") == 1) {
      return config_.get<std::string>("name");
    } else {
      return config_filename_.stem().native();
    }
  }

  const stdfs::path output_directory(void) const
  {
    std::string out_dir_name =
      config_.get<std::string>("output directory", "output");
    return simulation_base_path_ / out_dir_name;
  }

  const stdfs::path get_check_file_path(void) const
  {
    std::string check_dir_name =
      config_.get<std::string>("check file directory", "check");
    return simulation_base_path_ / check_dir_name;
  }

  std::optional<Config> write_check_file(const std::string& cf_name) const;
  
  const Config& configuration(void) const { return config_; }
  // const std::map<std::string, double>& time_unit_factors(void) const
  // {
  //   return time_unit_factors_;
  // }

  const double global_time_unit_factor(void) const
  {
    return global_time_factor_;
  }
  
  const double get_time_unit_factor(const Config& conf) const;
  
  const DeviceParameters& get_device_parameters(void) 
  {
    if (!device_params_) {
      device_params_ = std::make_shared<DeviceParameters>(this);
    }
    return *device_params_;
  }

  const RunParameters& get_run_parameters(void)
  {
    if (!run_params_) {
      run_params_ = std::make_shared<RunParameters>(this);
    }
    return *run_params_;
  }

  const TimestepParameters& get_timestep_parameters(void)
  {
    if (!dt_params_) {
      dt_params_ = std::make_shared<TimestepParameters>(this);
    }
    return *dt_params_;
  }

  const std::shared_ptr<TimeSeries<float>>
  get_time_series_ptr(const std::shared_ptr<sycl::queue>& queue,
		      const std::string& name)
  {
    using boost::algorithm::to_lower_copy;
    std::string comp_name = to_lower_copy(name);
    if (time_series_.count(comp_name) == 0) {
      load_time_series(queue, comp_name);
    }
    return time_series_[comp_name];
  }

  template<typename T>
  const std::shared_ptr<RasterField<T>>
  get_raster_field_ptr(const std::shared_ptr<sycl::queue>& queue,
		       const std::string& name)
  {
    using boost::algorithm::to_lower_copy;
    std::string comp_name = to_lower_copy(name);
    auto& field_map = get_raster_field_map<T>();
    if (field_map.count(comp_name) == 0) {
      std::cout << "Loading raster field: " << comp_name << std::endl;
      load_raster_field<T>(queue, field_map, comp_name);
      assert(field_map.count(comp_name) == 1);
    } else {
      std::cout << "Raster field " << comp_name << " is already loaded." << std::endl;
    }
    return field_map[comp_name];
  }

};

#endif

