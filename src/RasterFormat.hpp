/***********************************************************************
 * RasterFormat.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef RasterFormat_hpp
#define RasterFormat_hpp

#include "RasterField.hpp"

template<typename T>
class RasterFormat {
protected:

  virtual const std::vector<T>& values(void) const = 0;
  virtual size_t ncols(void) const = 0;
  virtual size_t nrows(void) const = 0;
  virtual const std::array<double, 6>& geo_transform(void) const = 0;
  virtual T nodata_value(void) const = 0;
  
public:

  RasterFormat(void) {}

  virtual ~RasterFormat(void) {}

  std::shared_ptr<RasterField<T>> operator()(const std::shared_ptr<sycl::queue>& queue)
  {
    return std::make_shared<RasterField<T>>(queue,
					    this->values(),
					    this->ncols(),
					    this->nrows(),
					    this->geo_transform().data(),
					    this->nodata_value());
  }
  
};

#endif
