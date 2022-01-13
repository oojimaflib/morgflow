/***********************************************************************
 * Config_cpp
 ***********************************************************************/

#include "Config.hpp"

template<>
std::vector<std::string> split_string(const std::string& in,
				      const std::string& delimiters)
{
  std::vector<std::string> vec;
  boost::algorithm::split(vec, in, boost::algorithm::is_any_of(delimiters));
  for (auto&& item : vec) {
    boost::algorithm::trim(item);
  }
  return vec;
}

