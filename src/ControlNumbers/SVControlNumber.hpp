/***********************************************************************
 * ControlNumbers/SVControlNumber.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef ControlNumbers_SVControlNumber_hpp
#define ControlNumbers_SVControlNumber_hpp

#include "../ControlNumber.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FM,
	 size_t N>
class SVControlNumber
  : public ControlNumber<T, MeshType, FM, N>
{
public:

  SVControlNumber(void)
    : ControlNumber<T,MeshType,FM,N>()
  {}

  virtual ~SVControlNumber(void)
  {}

  virtual T calculate(const FieldVector<T,MeshType,FM,N>& U,
		      const double& timestep) const
  {
    throw std::logic_error("This control number calculation is not implemented.");
  }
  
};

#include "SV/Cartesian2DMeshCell.hpp"

#endif
