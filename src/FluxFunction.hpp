/***********************************************************************
 * FluxFunction.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FluxFunction_hpp
#define FluxFunction_hpp

#include "FieldVector.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FromFM,
	 FieldMapping ToFM,
	 size_t FromN,
	 size_t ToN>
class FluxFunction
{
public:

  FluxFunction(void)
  {}

  virtual ~FluxFunction(void)
  {}

  virtual void calculate(const FieldVector<T,MeshType,FromFM,FromN>& U,
			 const FieldVector<T,MeshType,FromFM,3>& zb,
			 const FieldVector<T,MeshType,FromFM,4>& n,
			 const FieldVector<T,MeshType,FromFM,FromN>& dUdx,
			 const FieldVector<T,MeshType,FromFM,FromN>& dUdy,
			 FieldVector<T,MeshType,ToFM,ToN>& F) const = 0;

};

#endif

