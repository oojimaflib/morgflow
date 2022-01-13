/***********************************************************************
 * OutputFormat.cpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "Config.hpp"
#include "OutputFormat.hpp"

#include "OutputFormats/CSVOutputFormat.hpp"

#include "GlobalConfig.hpp"
#include "Meshes/Cartesian2DMesh.hpp"

template<typename ValueType,
	 typename MeshType>
std::shared_ptr<OutputFormat<ValueType,MeshType>>
create_output_format(const Config& config)
{
  std::string format_name = config.get_value<std::string>();

  stdfs::path output_dir =
    config.get<stdfs::path>("output directory",
			    GlobalConfig::instance().output_directory());
  std::string prefix = config.get<std::string>("output prefix", "");
  std::string suffix = config.get<std::string>("output suffix", "");
  
  if (format_name == "csv") {
    std::string gt = config.get<std::string>("geometry", "xyz");
    return std::make_shared<CSVOutputFormat<ValueType,MeshType>>(config,
								 gt, ", ",
								 output_dir,
								 prefix,
								 suffix);
  } else if (format_name == "txt") {
    std::string gt = config.get<std::string>("geometry", "xyz");
    return std::make_shared<CSVOutputFormat<ValueType,MeshType>>(config,
								 gt, "\t",
								 output_dir,
								 prefix,
								 suffix);
  } else {
    std::cerr << "Output format not known: " << format_name << std::endl;
    throw std::runtime_error("Output format not known.");
  }
}

template
std::shared_ptr<OutputFormat<float,Cartesian2DMesh> >
create_output_format(const Config& config);
