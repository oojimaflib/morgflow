/***********************************************************************
 * FluxFunctions/SVFluxFunction.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FluxFunctions_SV_Cartesian2DMeshCell2Face_hpp
#define FluxFunctions_SV_Cartesian2DMeshCell2Face_hpp

#include "Kernels/Cartesian2DMeshCell2FaceKernel.hpp"

template<typename T>
class SVFluxFunction<T, Cartesian2DMesh,
		     FieldMapping::Cell, FieldMapping::Face, 3, 4>
  : public FluxFunction<T, Cartesian2DMesh,
			FieldMapping::Cell, FieldMapping::Face, 3, 4>
{
public:

  using MeshType = Cartesian2DMesh;
  static const FieldMapping FromFM = FieldMapping::Cell;
  static const FieldMapping ToFM = FieldMapping::Face;
  static const size_t FromN = 3;
  static const size_t ToN = 4;
  
public:
  
  SVFluxFunction(void)
    : FluxFunction<T, MeshType, FromFM, ToFM, FromN, ToN>()
  {}

  virtual ~SVFluxFunction(void)
  {}

  virtual void calculate(const FieldVector<T,MeshType,FromFM,FromN>& U,
			 const FieldVector<T,MeshType,FromFM,3>& zb,
			 const FieldVector<T,MeshType,FromFM,4>& n,
			 const FieldVector<T,MeshType,FromFM,FromN>& dUdx,
			 const FieldVector<T,MeshType,FromFM,FromN>& dUdy,
			 FieldVector<T,MeshType,ToFM,ToN>& F) const
  {
    // Update dU/dx and dU/dy
    U.at(0).queue().submit([&] (sycl::handler& cgh) {
      auto kernel = SVCartesian2DMeshCell2FaceFluxFunctionKernel<T>(cgh, U, zb, n, dUdx, dUdy, F);
      
      cgh.parallel_for(F.get_range(), kernel);
    });
  }
};

#endif
