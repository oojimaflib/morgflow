/***********************************************************************
 * GlobalConfig.cpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "GlobalConfig.hpp"

#include "TimeSeries.hpp"
#include "RasterField.hpp"

#include "RasterFormats/GDAL.hpp"
#include "RasterFormats/NIMROD.hpp"

GlobalConfig* GlobalConfig::global_config_ = nullptr;

GlobalConfig& GlobalConfig::instance(void)
{
  if (global_config_) {
    return *global_config_;
  }
  throw std::logic_error("Global configuration not initialized");
}

void GlobalConfig::init(int argc, char* argv[])
{
  if (global_config_) {
    throw std::logic_error("Multiple initializations of global configuration.");
  }
  std::cout << "Initialising configuration. "
	    << argc << " argument(s). Config file: " << argv[1] << std::endl;
  global_config_ = new GlobalConfig(argc, argv);
}

void GlobalConfig::load_time_series(const std::shared_ptr<sycl::queue>& queue,
				    const std::string& name)
{
  using boost::algorithm::to_lower_copy;
  std::string comp_name = name;
    
  auto ts_crange = config_.equal_range("time series");
  for (auto it = ts_crange.first; it != ts_crange.second; ++it) {
    std::string ts_name = it->second.get_value<std::string>();
    std::string ts_comp_name = to_lower_copy(ts_name);
    if (ts_comp_name == comp_name) {
      std::cout << "Loading time series: " << ts_name << std::endl;

      std::string source_type_str =
	to_lower_copy(it->second.get<std::string>("source", "inline"));

      if (source_type_str == "inline") {
	time_series_[comp_name] =
	  TimeSeries<float>::load_inline(queue, it->second);
      } else if (source_type_str == "csv") {
	time_series_[comp_name] =
	  TimeSeries<float>::load_csv(queue, it->second);
      } else {
	std::cerr << "Unknown source type '" << source_type_str
		  << "' for time series: " << ts_name << std::endl;
	throw std::runtime_error("Unknown source type for time series");
      }
	
      return;
    }
  }
  std::cerr << "Could not find time series with name matching: "
	    << name << std::endl;
  throw std::runtime_error("Could not find time series.");
}

template<typename T>
void GlobalConfig::load_raster_field(const std::shared_ptr<sycl::queue>& queue,
				     std::map<std::string, std::shared_ptr<RasterField<T>>>& field_map,
				     const std::string& name)
{
  using boost::algorithm::to_lower_copy;
  std::string comp_name = name;
  // auto field_map = get_raster_field_map<T>();

  auto rf_crange = config_.equal_range("raster field");
  for (auto it = rf_crange.first; it != rf_crange.second; ++it) {
    std::string rf_name = it->second.get_value<std::string>();
    std::string rf_comp_name = to_lower_copy(rf_name);
    if (rf_comp_name == comp_name) {
      std::cout << "Loading raster field: " << rf_name << std::endl;

      std::string source_type_str =
	to_lower_copy(it->second.get<std::string>("source"));
      stdfs::path userpath = it->second.get<stdfs::path>("filename");
      stdfs::path filepath;
      if (userpath.is_absolute()) {
	filepath = userpath;
      } else {
	filepath = GlobalConfig::instance().simulation_base_path() / userpath;
      }

      if (source_type_str == "gdal") {
	field_map[comp_name] = GDALRasterFormat<T>(filepath, it->second)(queue);
	return;
      } else if (source_type_str == "nimrod") {
	field_map[comp_name] = NIMRODRasterFormat<T>(filepath, it->second)(queue);
	return;
      } else {
	std::cerr << "Unknown source type '" << source_type_str
		  << "' for raster field: " << rf_name << std::endl;
	throw std::runtime_error("Unknown source type for raster field");
      }
	
    }
  }
  std::cerr << "Could not find raster field with name matching: "
	    << name << std::endl;
  throw std::runtime_error("Could not find raster field.");
}

template void GlobalConfig::
load_raster_field<float>(const std::shared_ptr<sycl::queue>& queue,
			 std::map<std::string, std::shared_ptr<RasterField<float>>>& field_map,
			 const std::string& name);
template void GlobalConfig::
load_raster_field<double>(const std::shared_ptr<sycl::queue>& queue,
			  std::map<std::string, std::shared_ptr<RasterField<double>>>& field_map,
			  const std::string& name);

GlobalConfig::DeviceParameters::DeviceParameters(GlobalConfig* gconf)
  : platform_id(0),
    device_id(0)
{
  using boost::algorithm::to_lower_copy;
  const Config& conf = gconf->configuration().get_child("device parameters");

  std::vector<sycl::platform> platforms = sycl::platform::get_platforms();
  platform_id = platforms.size();
  std::string platform_name = "No platform";

  if (conf.count("platform id") == 1) {
    platform_id = conf.get<size_t>("platform id");
    platform_name = "ID = " + std::to_string(platform_id);
  } else {
    std::string requested_platform_list =
      conf.get<std::string>("platforms", "show,cuda,hip,omp");
    std::vector<std::string> requested_platforms =
      split_string<std::string>(requested_platform_list);
	
    for (auto&& req_platform_name : requested_platforms) {
      platform_name = to_lower_copy(req_platform_name);
	  
      if (platform_name == "show") {
	// Show the user a list of platforms
	DisplayTable<std::string, std::string, std::string>
	  platform_table( { {10, "ID", "%|s|"},
			    {10, "Name", "%|s|"},
			    {50, "Vendor", "%|s|"} });
	std::cout << "The following platforms are available: " << std::endl;
	platform_table.write_top_rule();
	platform_table.write_header_row();
	platform_table.write_mid_rule();
	for (size_t i = 0; i < platforms.size(); ++i) {
	  const sycl::platform& p = platforms.at(i);
	  platform_table.write_data_row(std::to_string(i),
					p.get_info<sycl::info::platform::name>(),
					p.get_info<sycl::info::platform::vendor>());
	}
	platform_table.write_bot_rule();
	std::cout << std::endl;
      } else {
	bool found = false;
	for (size_t i = 0; i < platforms.size(); ++i) {
	  if (platform_name == to_lower_copy(platforms.at(i).get_info<sycl::info::platform::name>()) and
	      platforms.at(i).get_devices().size() > 0) {
		
	    platform_id = i;
	    found = true;
	    break;
	  }
	}
	if (found) {
	  break;
	} else {
	  std::cout << "Platform " << req_platform_name
		    << " is not available." << std::endl;
	}
      }
    }
  }

  if (platform_id < platforms.size()) {
    platform = platforms.at(platform_id);
    std::cout << "Using platform " << platform_id << ": "
	      << platform.get_info<sycl::info::platform::name>()
	      << std::endl;
  } else {
    std::cerr << "Platform " << platform_name << " not found." << std::endl;
    throw std::runtime_error("Platform not available.");
  }

  std::vector<sycl::device> devices = platform.get_devices();
  device_id = devices.size();
  std::string device_name = 
    to_lower_copy(conf.get<std::string>("device", "show"));

  if (conf.count("device id") == 1) {
    device_id = conf.get<size_t>("device id");
    device_name = "ID = " + std::to_string(device_id);
  } else {
    if (device_name == "show") {
      // Show the user a list of devices
      DisplayTable<std::string, std::string, std::string>
	device_table( { {10, "ID", "%|s|"},
			{20, "Name", "%|s|"},
			{40, "Vendor", "%|s|"} });
      std::cout << "The following devices are available: " << std::endl;
      device_table.write_top_rule();
      device_table.write_header_row();
      device_table.write_mid_rule();
      for (size_t i = 0; i < devices.size(); ++i) {
	const sycl::device& d = devices.at(i);
	device_table.write_data_row(std::to_string(i),
				    d.get_info<sycl::info::device::name>(),
				    d.get_info<sycl::info::device::vendor>());
      }
      device_table.write_bot_rule();
    } else {
      for (size_t i = 0; i < devices.size(); ++i) {
	if (device_name == to_lower_copy(devices.at(i).get_info<sycl::info::device::name>())) {
	  device_id = i;
	  break;
	}
      }
    }
  }

  if (device_id < devices.size()) {
    device = devices.at(device_id);
    std::cout << "Using device " << device_id << ": "
	      << device.get_info<sycl::info::device::name>()
	      << std::endl;
  } else {
    std::cerr << "Device " << device_name << " not found." << std::endl;
    throw std::runtime_error("Device not available.");
  }
}

GlobalConfig::RunParameters::RunParameters(GlobalConfig* gconf)
  : start_time(0.0),
    end_time(0.0),
    sync_step(60.0),
    display_every(1)
{
  using boost::algorithm::to_lower_copy;
  const Config& conf = gconf->configuration().get_child("run parameters");
      
  double time_factor = gconf->get_time_unit_factor(conf);
      
  start_time = conf.get<double>("start time", start_time) * time_factor;
  end_time = conf.get<double>("end time", end_time) * time_factor;
  try {
    sync_step = conf.get<double>("sync step") * time_factor;
  } catch (bpt::ptree_bad_path& e) {
    sync_step = conf.get<double>("sync step seconds", 60.0);
  }
  display_every = conf.get<size_t>("display every", display_every);
      
  DisplayTable<std::string, std::string, std::string, std::string>
    params({ {40, "Parameter", "%|s|"},
	     {10, "Symbol", "%|s|"},
	     {10, "Default", "%|s|"},
	     {10, "Selected", "%|s|"} });
  std::cout << "   Reading Run Parameters:" << std::endl;
  params.write_top_rule();
  params.write_header_row();
  params.write_mid_rule();
  params.write_data_row("Start Time",
			"tₛ", std::to_string(0.0), std::to_string(start_time));
  params.write_data_row("End Time",
			"tₑ", std::to_string(0.0), std::to_string(end_time));
  params.write_data_row("Synchronization Step",
			"Δtₒ", std::to_string(60.0), std::to_string(sync_step));
  params.write_data_row("Step Display Interval",
			"", std::to_string(1), std::to_string(display_every));
  params.write_bot_rule();
}

GlobalConfig::TimestepParameters::TimestepParameters(GlobalConfig* gconf)
  : dt_type(DtType::undefined),
    time_step(1.0),
    max_time_step(9999.0),
    courant_target(0.999)
{
  using boost::algorithm::to_lower_copy;
  const Config& conf = gconf->configuration().get_child("timestep parameters");

  std::string type = to_lower_copy(conf.get_value<std::string>());
  if (type == "fixed") dt_type = DtType::fixed;
  if (type == "adaptive") dt_type = DtType::adaptive;
  if (dt_type == DtType::undefined) {
    std::cerr << "Timestepping type ('" << type
	      << "') not known or not defined." << std::endl;
  }

  time_step = conf.get<double>("time step", time_step);
  max_time_step = conf.get<double>("max time step", max_time_step);
  courant_target = conf.get<double>("courant target", courant_target);

  if (conf.count("ddt scheme") > 0) {
    ddt_scheme_config = conf.get_child("ddt scheme");
  } else {
    ddt_scheme_config = Config();
    ddt_scheme_config.put_value<std::string>("runge kutta");
    ddt_scheme_config.put<std::string>("runge kutta.method", "ralston4");
  }

  DisplayTable<std::string, std::string, std::string, std::string>
    params({ {40, "Parameter", "%|s|"},
	     {10, "Symbol", "%|s|"},
	     {10, "Default", "%|s|"},
	     {10, "Selected", "%|s|"} });
  std::cout << "   Reading Timestep Parameters:" << std::endl;
  params.write_top_rule();
  params.write_header_row();
  params.write_mid_rule();
  params.write_data_row("Time stepping approach",
			"", "undefined", type);
  params.write_data_row("Time step",
			"Δt", std::to_string(1.0), std::to_string(time_step));
  params.write_data_row("Maximum time step",
			"Δtₘₐₓ", std::to_string(9999.0), std::to_string(max_time_step));
  params.write_data_row("Courant Number Target",
			"Coₘₐₓ", std::to_string(0.999), std::to_string(courant_target));
  params.write_bot_rule();
}
    
GlobalConfig::GlobalConfig(int argc, char* argv[])
{
  if (argc != 2) {
    std::cerr << "ERROR: Expected one simulation file as argument."
	      << std::endl;
    throw std::runtime_error("Wrong argument list");
  }

  std::string config_file_path_str = argv[1];
  stdfs::path config_file_path(config_file_path_str);
  if (config_file_path.has_filename()) {
    try {
      config_filename_ = config_file_path.filename();
      std::cout << config_filename_ << std::endl;
      simulation_base_path_ = config_file_path.parent_path();
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      throw e;
    }
  } else {
    std::cerr << "Could not determine filename from " << config_file_path << std::endl;
  }

  std::cout << "Simulation base directory: " << simulation_base_path_ << std::endl;
  std::cout << "Configuration file name: " << config_filename_ << std::endl;

  bpt::read_mf((simulation_base_path_ / config_filename_).native(),
	       config_);

  global_time_factor_ = 0.0;
  global_time_factor_ = get_time_unit_factor(config_);
  std::cout << "Global time unit factor: " << global_time_factor_ << std::endl;
}

std::optional<Config>
GlobalConfig::write_check_file(const std::string& cf_name) const
{
  using boost::algorithm::to_lower_copy;
  std::string cf_name_comp = to_lower_copy(cf_name);

  {  // Check if this check file explicitly activated
    auto ckrange = config_.equal_range("check");
    for (auto it = ckrange.first; it != ckrange.second; ++it) {
      std::string val = to_lower_copy(it->second.get_value<std::string>());
      if (cf_name_comp == val) {
	return std::optional<Config>(it->second);
      }
    }
  }

  {  // Check if this check file explicitly deactivated
    auto ckrange = config_.equal_range("no check");
    for (auto it = ckrange.first; it != ckrange.second; ++it) {
      std::string val = to_lower_copy(it->second.get_value<std::string>());
      if (cf_name_comp == val) {
	return std::optional<Config>();
      }
    }
  }

  // No explicit selection. Some check files should be output by
  // default:
  if (cf_name_comp == "mesh") {
    return std::optional<Config>(Config());
  }

  // No explicit selection and check file not active by default:
  return std::optional<Config>();
} 

std::map<std::string, double> GlobalConfig::time_unit_factors_ =
  {
    { "seconds", 1.0 },
    { "second", 1.0 },
    { "secs", 1.0 },
    { "sec", 1.0 },
    { "s", 1.0 },
      
    { "minutes", 60.0 },
    { "minute", 60.0 },
    { "mins", 60.0 },
    { "min", 60.0 },
    { "m", 60.0 },
    
    { "hours", 3600.0 },
    { "hour", 3600.0 },
    { "hrs", 3600.0 },
    { "hr", 3600.0 },
    { "h", 3600.0 },
  };

const double GlobalConfig::get_time_unit_factor(const Config& conf) const
{
  using boost::algorithm::to_lower_copy;
  std::string time_unit_str =
    to_lower_copy(conf.get<std::string>("time units", "default"));
  if (time_unit_str == "default") {
    if (global_time_factor_ > 0.0) {
      return global_time_factor_;
    } else {
      return 1.0;
    }
  } else if (time_unit_factors_.count(time_unit_str) > 0) {
    return time_unit_factors_.at(time_unit_str);
  } else {
    std::cerr << "ERROR: Unknown time unit: " << time_unit_str << std::endl;
    throw std::runtime_error("Unknown time unit.");
  }
}


