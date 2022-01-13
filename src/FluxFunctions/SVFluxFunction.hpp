/***********************************************************************
 * FluxFunctions/SVFluxFunction.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FluxFunctions_SVFluxFunction_hpp
#define FluxFunctions_SVFluxFunction_hpp

#include "../FluxFunction.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FromFM,
	 FieldMapping ToFM,
	 size_t FromN,
	 size_t ToN>
class SVFluxFunction
  : public FluxFunction<T, MeshType, FromFM, ToFM, FromN, ToN>
{
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
    throw std::logic_error("This type of flux function is not implemented.");
  }
  
};

#include "SV/Cartesian2DMeshCell2Face.hpp"

#endif
