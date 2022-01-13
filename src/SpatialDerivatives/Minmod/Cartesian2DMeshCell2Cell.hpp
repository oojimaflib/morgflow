/***********************************************************************
 * SpatialDerivatives/MinmodSpatialDerivative.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef SpatialDerivatives_Minmod_Cartesian2DMeshCell2Cell_hpp
#define SpatialDerivatives_Minmod_Cartesian2DMeshCell2Cell_hpp

#include "Kernels/Cartesian2DMeshCell2CellKernel.hpp"

template<typename T, size_t N>
class MinmodSpatialDerivative<T, Cartesian2DMesh,
			      FieldMapping::Cell, FieldMapping::Cell, N>
  : public SpatialDerivative<T, Cartesian2DMesh,
			     FieldMapping::Cell, FieldMapping::Cell, N>
{
public:

  using MeshType = Cartesian2DMesh;
  static const FieldMapping FromFM = FieldMapping::Cell;
  static const FieldMapping ToFM = FieldMapping::Cell;
  
private:

  T theta_;
  
public:
  
  MinmodSpatialDerivative(void)
    : SpatialDerivative<T, MeshType, FromFM, ToFM, N>(),
      theta_(2.0)
  {}

  virtual ~MinmodSpatialDerivative(void)
  {}

  virtual void calculate(const FieldVector<T,MeshType,FromFM,N>& U,
			 FieldVector<T,MeshType,ToFM,N>& dUdx,
			 FieldVector<T,MeshType,ToFM,N>& dUdy) const
  {
    // Update dU/dx and dU/dy
    U.at(0).queue().submit([&] (sycl::handler& cgh) {
      auto kernel = MinmodCartesian2DMeshCell2CellSpatialDerivativeKernel<T,N>(cgh, U, dUdx, dUdy, theta_);
      
      cgh.parallel_for(dUdx.get_range(), kernel);
    });
  }
};

#endif
