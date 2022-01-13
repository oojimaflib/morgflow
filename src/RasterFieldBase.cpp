/***********************************************************************
 * RasterFieldBase.cpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "RasterField.hpp"

#include "Config.hpp"

template<>
GDALDataType get_gdal_buf_type<float>(void) { return GDT_Float32; }
template<>
GDALDataType get_gdal_buf_type<double>(void) { return GDT_Float64; }
template<>
GDALDataType get_gdal_buf_type<int32_t>(void) { return GDT_Int32; }
template<>
GDALDataType get_gdal_buf_type<uint32_t>(void) { return GDT_UInt32; }

template<typename T>
std::shared_ptr<RasterFieldBase>
RasterFieldBase::load_gdal(const std::shared_ptr<sycl::queue>& queue,
			   const Config& conf)
{
  return RasterField<T>::load_gdal(queue, conf);
}
