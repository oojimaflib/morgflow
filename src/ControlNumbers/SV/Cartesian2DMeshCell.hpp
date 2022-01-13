/***********************************************************************
 * ControlNumbers/SV/Cartesian2DMeshCell.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef ControlNumbers_SV_Cartesian2DMeshCell_hpp
#define ControlNumbers_SV_Cartesian2DMeshCell_hpp

template<typename T>
class SVControlNumber<T, Cartesian2DMesh, FieldMapping::Cell, 3>
  : public ControlNumber<T, Cartesian2DMesh, FieldMapping::Cell, 3>
{
public:

  using MeshType = Cartesian2DMesh;
  static const FieldMapping FM = FieldMapping::Cell;
  static const size_t N = 3;

  SVControlNumber(void)
    : ControlNumber<T, MeshType, FM, N>()
  {}

  virtual ~SVControlNumber(void)
  {}

  virtual T calculate(const FieldVector<T,MeshType,FM,N>& U,
		      const double& timestep) const
  {
    T mcn = 0.0;
    sycl::buffer<T> maxCN_buf(&mcn, 1);
    
    U.at(0).queue().submit([&] (sycl::handler& cgh) {
      auto U_ro = U.get_read_accessor(cgh);

      auto maxCN = sycl::reduction(maxCN_buf.get_access(cgh), sycl::maximum<T>());

      typename MeshType::CoordType cs = U.mesh_definition()->cell_size();
      T dx = cs[0];
      T dy = cs[1];

      cgh.parallel_for(U.get_range(), maxCN,
		       [=](sycl::id<1> id, auto& max) {
			 T h = sycl::fmax(U_ro[0][id], 0.0f);
			 T u = sycl::fabs(U_ro[1][id]);
			 T v = sycl::fabs(U_ro[2][id]);
			 T c = sycl::sqrt(9.81f * h);
			 T cn = timestep * (((u+c) / dx) + ((v+c) / dy));
			 max.combine(cn);
		       });
    });
    return maxCN_buf.get_host_access()[0];
  }
  
};

#endif
