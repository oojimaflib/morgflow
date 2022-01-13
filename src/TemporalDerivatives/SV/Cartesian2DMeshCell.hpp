/***********************************************************************
 * TemporalDerivatives/SVTemporalDerivative.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef TemporalDerivatives_SV_Cartesian2DMeshCell_hpp
#define TemporalDerivatives_SV_Cartesian2DMeshCell_hpp

#include "Kernels/Cartesian2DMeshCellKernel.hpp"

template<typename T>
class SVTemporalDerivative<T, Cartesian2DMesh,
		     FieldMapping::Cell, 3>
  : public TemporalDerivative<T, Cartesian2DMesh,
			FieldMapping::Cell, 3>
{
public:

  using MeshType = Cartesian2DMesh;
  static const FieldMapping FM = FieldMapping::Cell;
  static const size_t N = 3;
  
public:
  
  SVTemporalDerivative(void)
    : TemporalDerivative<T, MeshType, FM, N>()
  {}

  virtual ~SVTemporalDerivative(void)
  {}

  virtual void calculate(const FieldVector<T,MeshType,FM,N>& U,
			 const FieldVector<T,MeshType,FieldMapping::Cell,3>& zb,
			 const FieldVector<T,MeshType,FieldMapping::Cell,4>& n,
			 const FieldVector<T,MeshType,FieldMapping::Cell,2>& Q_in,
			 const FieldVector<T,MeshType,FieldMapping::Cell,2>& h_in,
			 const FieldVector<T,MeshType,FieldMapping::Face,4>& flux,
			 FieldVector<T,MeshType,FM,N>& dUdt,
			 const double& time_now, const double& timestep,
			 const double& bdy_t0, const double& bdy_t1) const
  {
    // Update dU/dx and dU/dy
    U.at(0).queue().submit([&] (sycl::handler& cgh) {
      auto kernel = SVCartesian2DMeshCellTemporalDerivativeKernel<T>(cgh, U, zb, n, Q_in, h_in, flux, dUdt, time_now, timestep, bdy_t0, bdy_t1);
      
      cgh.parallel_for(dUdt.get_range(), kernel);
    });
  }
};

#endif
