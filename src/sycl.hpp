/***********************************************************************
 * sycl.hpp
 ***********************************************************************/

#ifndef sycl_hpp
#define sycl_hpp

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-conversion"
#pragma GCC diagnostic ignored "-Wignored-attributes"
#include <CL/sycl.hpp>
namespace sycl = cl::sycl;
#pragma GCC diagnostic pop

/*
std::vector<std::string> platform_name_list(void)
{
  std::vector<sycl::platform> platforms = sycl::platform::get_platforms();
  std::vector<std::string> name_list;
  for (auto&& p : platforms) {
    name_list.push_back(p.get_info<sycl::info::platform::name>());
  }
  return name_list;
}
*/
/*
std::vector<std::string> platform_device_name_list(const sycl::platform& p)
{
  std::vector<sycl::device> devices = p.get_devices();
  std::vector<std::string> name_list;
  for (auto&& d : devices) {
    name_list.push_back(d.get_info<sycl::info::device::name>());
  }
  return name_list;
}

*/


#endif
