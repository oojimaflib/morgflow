/***********************************************************************
 * ControlNumber.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef ControlNumber_hpp
#define ControlNumber_hpp

#include "FieldVector.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FM,
	 size_t N>
class ControlNumber
{
public:

  ControlNumber(void)
  {}

  virtual ~ControlNumber(void)
  {}

  virtual T calculate(const FieldVector<T,MeshType,FM,N>& U,
		      const double& timestep) const = 0;
};

#endif

