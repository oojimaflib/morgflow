/***********************************************************************
 * CSVOutputFormat.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef OutputFormats_CSVOutputFormat_hpp
#define OutputFormats_CSVOutputFormat_hpp

#include "../OutputFormat.hpp"

template<typename ValueType,
	 typename MeshType>
class CSVOutputFormat : public OutputFormat<ValueType,MeshType>
{
public:

  enum class GeometryType {
    XYZ,
    WKT
  };

private:

  GeometryType geom_type_;
  
  std::string delimiter_;
  
public:

  CSVOutputFormat(const Config& conf,
		  const std::string& geom_type_in_str,
		  const std::string& delimiter,
		  const stdfs::path& output_dir,
		  const std::string& prefix = "",
		  const std::string& suffix = ".txt")
    : OutputFormat<ValueType,MeshType>(output_dir, prefix, suffix),
      geom_type_(GeometryType::XYZ),
      delimiter_(delimiter)
  {
    using boost::algorithm::to_lower_copy;
    std::string geom_type_str =
      to_lower_copy(conf.get<std::string>("geometry", geom_type_in_str));
    if (geom_type_str == "xyz" or geom_type_str == "xy") {
      geom_type_ = GeometryType::XYZ;
    } else if (geom_type_str == "wkt") {
      geom_type_ = GeometryType::WKT;
    } else {
      std::cerr << "Unknown geometry type, '" << geom_type_in_str
		<< "' for CSV output." << std::endl;
      throw std::runtime_error("Unknown geometry type for CSV output.");
    }
    delimiter_ = conf.get<std::string>("delimiter", delimiter);
  }

  virtual ~CSVOutputFormat(void)
  {}

  virtual void output(std::shared_ptr<OutputFunction<ValueType,MeshType>>& func,
		      const std::string& time_tag) const
  {
    std::ofstream ofs = this->open(func, time_tag);

    for (size_t i = 0; i < func->output_size(); ++i) {
      if (geom_type_ == GeometryType::XYZ) {
	for (auto&& coord : func->output_coordinates(i)) {
	  ofs << coord << delimiter_;
	}
      } else {
	ofs << "\"" << func->output_wkt(i) << "\"" << delimiter_;
      }
      for (auto&& val : func->output_values(i)) {
	ofs << val << delimiter_;
      }
      ofs << std::endl;
    }

    ofs.close();
  }
  
};

#endif
