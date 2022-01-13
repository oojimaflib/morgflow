/***********************************************************************
 * TemporalDerivatives/SVTemporalDerivative.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef TemporalDerivatives_SVTemporalDerivative_hpp
#define TemporalDerivatives_SVTemporalDerivative_hpp

#include "../TemporalDerivative.hpp"

template<typename T,
	 typename MeshType,
	 FieldMapping FM,
	 size_t N>
class SVTemporalDerivative
  : public TemporalDerivative<T, MeshType, FM, N>
{
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
    throw std::logic_error("This type of flux function is not implemented.");
  }
    
};

#include "SV/Cartesian2DMeshCell.hpp"

#endif
