/***********************************************************************
 * OutputFormat.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef OutputFormat_hpp
#define OutputFormat_hpp

#include "Field.hpp"
#include "Mesh.hpp"
#include "OutputFunction.hpp"

template<typename ValueType,
	 typename MeshType>
class OutputFormat
{
protected:

  mutable bool output_dir_exists_;
  stdfs::path output_dir_;
  std::string prefix_;
  std::string suffix_;
  
public:

  OutputFormat(const stdfs::path& output_dir,
	       const std::string& prefix = "",
	       const std::string& suffix = "")
    : output_dir_exists_(false),
      output_dir_(output_dir),
      prefix_(prefix), suffix_(suffix)
  {}

  virtual ~OutputFormat(void)
  {}

  std::ofstream open(std::shared_ptr<OutputFunction<ValueType,MeshType>>& func,
		      const std::string& time_tag) const
  {
    if (not output_dir_exists_) {
      if (not stdfs::exists(output_dir_)) {
	try {
	  stdfs::create_directory(output_dir_);
	} catch (stdfs::filesystem_error& err) {
	  std::cerr << "Could not create output directory: "
		    << output_dir_ << std::endl;
	  std::cerr << err.what() << std::endl;
	  throw err;
	}
      } else if (not stdfs::is_directory(output_dir_)) {
	std::cerr << "Could not create output directory over file: "
		  << output_dir_ << std::endl;
	throw std::runtime_error("File exists.");
      }
      output_dir_exists_ = true;
    }

    std::string output_filename =
      prefix_ + func->name() + "_" + time_tag + suffix_;
    stdfs::path output_path = output_dir_ / output_filename;

    std::ofstream ofs(output_path);
    return ofs;
  }

  virtual void output(std::shared_ptr<OutputFunction<ValueType,MeshType>>& output_function,
		      const std::string& time_tag) const = 0;
  
};

template<typename ValueType,
	 typename MeshType>
std::shared_ptr<OutputFormat<ValueType,MeshType>>
create_output_format(const Config& config);

#endif
