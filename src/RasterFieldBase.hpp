/***********************************************************************
 * RasterFieldBase.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef RasterFieldBase_hpp
#define RasterFieldBase_hpp

#include "gdal_priv.h"
#include "Config.hpp"

template<typename T>
GDALDataType get_gdal_buf_type(void) {
  return GDT_Unknown;
}

class RasterFieldBase
{
private:
  
  template<typename T>
  static std::shared_ptr<RasterFieldBase> load_gdal(const std::shared_ptr<sycl::queue>& queue,
						    const Config& conf);

public:

  RasterFieldBase() {}
  virtual ~RasterFieldBase() {}
  
};

#endif
