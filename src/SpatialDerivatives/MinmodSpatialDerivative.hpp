/***********************************************************************
 * SpatialDerivatives/MinmodSpatialDerivative.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef SpatialDerivatives_MinmodSpatialDerivative_hpp
#define SpatialDerivatives_MinmodSpatialDerivative_hpp

#include "../SpatialDerivative.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FromFM,
	 FieldMapping ToFM,
	 size_t N>
class MinmodSpatialDerivative
  : public SpatialDerivative<T, MeshType, FromFM, ToFM, N>
{
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
    throw std::logic_error("This type of spatial derivative not implemented.");
  }
  
};

#include "Minmod/Cartesian2DMeshCell2Cell.hpp"

#endif
