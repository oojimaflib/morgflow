/***********************************************************************
 * RasterField.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef RasterField_hpp
#define RasterField_hpp

#include "GlobalConfig.hpp"
#include "DataArray.hpp"

template<typename T>
class RasterField
{
private:

  DataArray<T> values_;

  DataArray<size_t> ncells_;
  DataArray<double> geotrans_;
  
public:

  RasterField(const std::shared_ptr<sycl::queue>& queue,
	      const std::vector<T>& values,
	      const size_t& nxcells, const size_t& nycells,
	      const double* geo_transform,
	      const T& nodata_value)
    : values_(queue, values),
      ncells_(queue, std::vector<size_t>({ nxcells, nycells })),
      geotrans_(queue, std::vector<double>(geo_transform, geo_transform + 6))
  {
    assert(values_.size() == nxcells * nycells);
    values_.host_vector().push_back(nodata_value);
    values_.move_to_device();

    assert(ncells_.size() == 2);
    ncells_.move_to_device();

    assert(geotrans_.size() == 6);
    std::vector<double>& gtvec = geotrans_.host_vector();
    gtvec.push_back(1.0 / gtvec.at(1)); // 6: 1/b
    gtvec.push_back(1.0 / gtvec.at(5)); // 7: 1/f
    gtvec.push_back(1.0 / (gtvec.at(1) * gtvec.at(5))); // 8: 1/(fb)
    gtvec.push_back(1.0 - (gtvec.at(2) * gtvec.at(4)) * gtvec.at(8)); // 9: 1 - (ce)/(fb)
    assert(geotrans_.size() == 10);
    geotrans_.move_to_device();
  }

  virtual ~RasterField(void) {}

  const DataArray<T>& get_values_array(void) const
  {
    return values_;
  }
  
  const DataArray<size_t>& get_ncells_array(void) const
  {
    return ncells_;
  }
  
  const DataArray<double>& get_geotrans_array(void) const
  {
    return geotrans_;
  }
  
};

template<typename T>
class RasterFieldAccessor
{
private:

  using ValueAccessor =
    typename DataArray<T>::
    template Accessor<sycl::access::mode::read,
		      sycl::access::target::global_buffer,
		      sycl::access::placeholder::true_t>;

  using NCellAccessor =
    typename DataArray<size_t>::
    template Accessor<sycl::access::mode::read,
		      sycl::access::target::global_buffer,
		      sycl::access::placeholder::true_t>;

  using GeoTrAccessor =
    typename DataArray<double>::
    template Accessor<sycl::access::mode::read,
		      sycl::access::target::global_buffer,
		      sycl::access::placeholder::true_t>;

  ValueAccessor values_ro_;
  NCellAccessor ncells_ro_;
  GeoTrAccessor geotrans_ro_;

  inline T nodata_value(void) const
  {
    return values_ro_[ncells_ro_[0] * ncells_ro_[1]];
  }

  inline double x_tl(void) const { return geotrans_ro_[0]; }
  inline double x_size(void) const { return geotrans_ro_[1]; }
  inline double x_rot(void) const { return geotrans_ro_[2]; }
  inline double y_tl(void) const { return geotrans_ro_[3]; }
  inline double y_rot(void) const { return geotrans_ro_[4]; }
  inline double y_size(void) const { return geotrans_ro_[5]; }

  inline double inv_x_size(void) const { return geotrans_ro_[6]; }
  inline double inv_y_size(void) const { return geotrans_ro_[7]; }
  inline double inv_xy_size(void) const { return geotrans_ro_[8]; }
  inline double inv_denom(void) const { return geotrans_ro_[9]; }

  double get_fractional_xi(const std::array<double,2>& loc) const
  {
    return ((loc[0] - x_tl()) * inv_x_size() -
	    (loc[1] - y_tl()) * x_rot() * inv_xy_size()) * inv_denom();
  }

  size_t get_xi(const std::array<double,2>& loc) const
  {
    return size_t(get_fractional_xi(loc));
  }
  
  double get_fractional_yi(const std::array<double,2>& loc) const
  {
    return ((loc[1] - y_tl()) * inv_y_size() -
	    (loc[0] - x_tl()) * y_rot() * inv_xy_size()) * inv_denom();
  }
  
  size_t get_yi(const std::array<double,2>& loc) const
  {
    return size_t(get_fractional_yi(loc));
  }
  
  std::vector<size_t> get_pixels_in_polygon(const std::vector<std::array<double,2>>& polygon_vertex_list) const 
  {
    /*
    std::cout << "Getting pixels in polygon: {" << std::endl;
    for (auto&& vx : polygon_vertex_list) {
      std::cout << "( " << vx[0] << "\t" << vx[1] << " )" << std::endl;
    }
    std::cout << "}" << std::endl;
    */
    
    std::vector<size_t> pixel_list;
    for (size_t yi = 0; yi < ncells_ro_[1]; ++yi) {
      std::vector<size_t> nodes_xi;
      size_t j = polygon_vertex_list.size() - 1;
      for (size_t i = 0; i < polygon_vertex_list.size(); ++i) {
	double viy = get_fractional_yi(polygon_vertex_list.at(i));
	double vjy = get_fractional_yi(polygon_vertex_list.at(j));
	if ((viy < (double) yi && vjy >= (double) yi) ||
	    (vjy < (double) yi && viy >= (double) yi)) {
	  double vix = get_fractional_xi(polygon_vertex_list.at(i));
	  double vjx = get_fractional_xi(polygon_vertex_list.at(j));
	  nodes_xi.push_back((size_t) (vix + (yi - viy) / (vjy - viy) * (vjx - vix)));
	}
	j = i;
      }

      if (nodes_xi.size() == 0) continue;

      /*
      std::cout << "row: " << yi << std::endl;
      std::cout << "Node list: {" << std::endl;
      for (auto&& xi : nodes_xi) {
	std::cout << xi << std::endl;
      }
      std::cout << "}" << std::endl;
      */
      
      size_t i = 0;
      while (i < nodes_xi.size() - 1) {
	if (nodes_xi.at(i) > nodes_xi.at(i+1)) {
	  std::swap(nodes_xi.at(i), nodes_xi.at(i+1));
	} else {
	  ++i;
	}
      }

      /*
      std::cout << "Node list (sorted): {" << std::endl;
      for (auto&& xi : nodes_xi) {
	std::cout << xi << std::endl;
      }
      std::cout << "}" << std::endl;
      */
      
      for (size_t i = 0; i < nodes_xi.size(); i += 2) {
	if (nodes_xi.at(i) >= ncells_ro_[0]) break;
	if (nodes_xi.at(i+1) < ncells_ro_[0]) {
	  if (nodes_xi.at(i) > ncells_ro_[0]) {
	    nodes_xi.at(i) = 0;
	  }
	  if (nodes_xi.at(i+1) >= ncells_ro_[0]) {
	    nodes_xi.at(i+1) = ncells_ro_[0];
	  }
	  for (size_t xi = nodes_xi.at(i); xi < nodes_xi.at(i+1); ++xi) {
	    pixel_list.push_back(yi * ncells_ro_[0] + xi);
	  }
	}
      }
    }
    return pixel_list;
  }
  
public:

  RasterFieldAccessor(const RasterField<T>& rf)
    : values_ro_(rf.get_values_array().template get_placeholder_accessor<sycl::access::mode::read, sycl::access::target::global_buffer>()),
      ncells_ro_(rf.get_ncells_array().template get_placeholder_accessor<sycl::access::mode::read, sycl::access::target::global_buffer>()),
      geotrans_ro_(rf.get_geotrans_array().template get_placeholder_accessor<sycl::access::mode::read, sycl::access::target::global_buffer>())
  {}

  void bind(sycl::handler& cgh)
  {
    cgh.require(values_ro_);
    cgh.require(ncells_ro_);
    cgh.require(geotrans_ro_);
  }

  T inspect_point(const std::array<double,2>& loc,
		  const double& nodata) const
  {
    size_t xi = get_xi(loc);
    size_t yi = get_yi(loc);
    if (xi < ncells_ro_[0] and yi < ncells_ro_[1]) {
      T value = values_ro_[yi * ncells_ro_[0] + xi];
      // std::cout << "Rlookup:\t" << xi << "\t" << yi << "\t" << value << "\t" << nodata_value() << "\t" << nodata << std::endl;
      if (std::isnan(value) || value == nodata_value()) {
	return nodata;
      }
      return value;
    }
    return nodata;
  }

  template<typename FFOp>
  T inspect_box(const std::array<double,2>& coord,
		const std::array<double,2>& box_size,
		const double& nodata) const
  {
    std::array<double,2> c0 = {
      coord[0] - 0.5 * box_size[0],
      coord[1] - 0.5 * box_size[1]
    };
    std::array<double,2> c1 = {
      coord[0] + 0.5 * box_size[0],
      coord[1] + 0.5 * box_size[1]
    };
    size_t xi0 = get_xi(c0);
    size_t yi1 = get_yi(c0);
    size_t xi1 = get_xi(c1);
    size_t yi0 = get_yi(c1);

    FFOp function_operation = FFOp(nodata);
    T result;

    while (function_operation.iterations_remaining() > 0) {
      for (size_t xi = xi0; xi < xi1; ++xi) {
	if (xi == ncells_ro_[0]) break;
	if (xi > ncells_ro_[0]+100) continue;
	for (size_t yi = yi0; yi < yi1; ++yi) {
	  if (yi == ncells_ro_[1]) break;
	  if (yi > ncells_ro_[1]+100) continue;
	  T value = values_ro_[yi * ncells_ro_[0] + xi];
	  function_operation.append(value);
	}
      }
      result = function_operation.get();
    }

    return result;
  }
  
  void get_area_statistics(const std::vector<std::array<double,2>>& poly,
			   T* min, T* max, T* mean)
  {
    std::vector<size_t> pixel_indices;
  }
  
};

#endif
