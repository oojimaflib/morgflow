/***********************************************************************
 * SpatialDerivative.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef SpatialDerivative_hpp
#define SpatialDerivative_hpp

#include "FieldVector.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FromFM,
	 FieldMapping ToFM,
	 size_t N>
class SpatialDerivative
{
public:
  
  SpatialDerivative(void)
  {}

  virtual ~SpatialDerivative(void)
  {}

  virtual void calculate(const FieldVector<T,MeshType,FromFM,N>& U,
			 FieldVector<T,MeshType,ToFM,N>& dUdx,
			 FieldVector<T,MeshType,ToFM,N>& dUdy) const = 0;
  
};

#endif
