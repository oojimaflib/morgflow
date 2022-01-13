/***********************************************************************
 * BoundaryConditions/SVBoundaryCondition.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef BoundaryConditions_SVBoundaryCondition_hpp
#define BoundaryConditions_SVBoundaryCondition_hpp

#include "../Meshes/Cartesian2DMesh.hpp"
#include "../BoundaryCondition.hpp"
#include "../TemporalScheme.hpp"
#include "../SVSolver.hpp"

template<>
class BoundaryCondition<SVSolver>
{
protected:

  std::string name_;
  
public:

  using MeshType = typename SVSolver::MeshType;
  using ValueType = typename SVSolver::ValueType;

  enum class Variable {
    Q, h
  };

  BoundaryCondition<SVSolver>(const std::string& name)
    : name_(name)
  {}

  virtual ~BoundaryCondition<SVSolver>(void) {}

  const std::string& name(void) { return name_; }

  virtual void update(TemporalScheme<SVSolver>& ts,
		      const double& t0, const double& t1) const = 0;
  
  virtual Variable get_variable(void) const = 0;

};

#include "SV/SVSourceBoundaryCondition.hpp"
#include "SV/SVDepthBoundaryCondition.hpp"

/*
template<>
std::shared_ptr<BoundaryCondition<SVSolver>>
create_boundary_condition(std::shared_ptr<SVSolver>& solver,
			  const Config& conf);
*/

template<>
std::vector<std::shared_ptr<BoundaryCondition<SVSolver>>>
create_boundary_conditions(std::shared_ptr<SVSolver>& solver);

#endif
