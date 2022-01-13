/***********************************************************************
 * Config_hpp
 ***********************************************************************/

#ifndef Config_hpp
#define Config_hpp

#include <stdexcept>
#include <iostream>

#include "mf_parser.hpp"
namespace bpt = boost::property_tree;
typedef bpt::iptree Config;

//#include <boost/filesystem.hpp>
//namespace stdfs = boost::filesystem;

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

template<typename T>
std::vector<T> split_string(const std::string& in,
			    const std::string& delimiters = ",")
{
  std::vector<std::string> vec;
  boost::algorithm::split(vec, in, boost::algorithm::is_any_of(delimiters));
  std::vector<T> res;
  for (auto&& item : vec) {
    res.push_back(boost::lexical_cast<T>(item));
  }
  return res;
}

template<>
std::vector<std::string> split_string(const std::string& in,
				      const std::string& delimiters);

template<typename T, int D>
std::array<T, D> split_string(const std::string& in,
			      const std::string& delimiters = ",")
{
  std::vector<std::string> vec = split_string<std::string>(in, delimiters);
  if (vec.size() != D) {
    std::cerr << "Expected " << D
	      << " components in vector " << in << std::endl;
    std::cerr << "Actually got " << vec.size() << " components: " << std::endl;
    for (auto&& comp : vec) {
      std::cerr << "\"" << comp << "\"" << std::endl;
    }
    throw std::runtime_error("Unexpected number of components in vector");
  }
  
  std::array<T, D> arr;
  for (size_t i = 0; i < D; ++i) {
    arr[i] = boost::lexical_cast<T>(vec.at(i));
  }
  return arr;
}

#endif
