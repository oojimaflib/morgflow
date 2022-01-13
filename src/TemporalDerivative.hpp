/***********************************************************************
 * TemporalDerivative.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef TemporalDerivative_hpp
#define TemporalDerivative_hpp

#include "FieldVector.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FM,
	 size_t N>
class TemporalDerivative
{
public:

  TemporalDerivative(void)
  {}

  virtual ~TemporalDerivative(void)
  {}

  virtual void calculate(const FieldVector<T,MeshType,FM,N>& U,
			 const FieldVector<T,MeshType,FieldMapping::Cell,3>& zb,
			 const FieldVector<T,MeshType,FieldMapping::Cell,4>& n,
			 const FieldVector<T,MeshType,FieldMapping::Cell,2>& Q_in,
			 const FieldVector<T,MeshType,FieldMapping::Cell,2>& h_in,
			 const FieldVector<T,MeshType,FieldMapping::Face,4>& flux,
			 FieldVector<T,MeshType,FM,N>& dUdt,
			 const double& time_now, const double& timestep,
			 const double& bdy_t0, const double& bdy_t1) const = 0;

};

#endif

