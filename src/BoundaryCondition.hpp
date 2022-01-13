/***********************************************************************
 * BoundaryCondition.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef BoundaryCondition_hpp
#define BoundaryCondition_hpp

#include "Config.hpp"
#include "MeshSelection.hpp"
#include "FieldModifier.hpp"

// #include "TimeSeries.hpp"
template<typename Solver>
class TemporalScheme;

template<typename Solver>
class BoundaryCondition {
protected:

  std::string name_;

public:

  BoundaryCondition(const std::string& name)
    : name_(name)
  {}

  virtual ~BoundaryCondition(void)
  {}

  const std::string& name(void) { return name_; }

  virtual void update(TemporalScheme<Solver>& ts,
		      const double& t0, const double& t1) const = 0;
  
};

/*
template<typename Solver>
std::shared_ptr<BoundaryCondition<Solver>>
create_boundary_condition(std::shared_ptr<Solver>& solver,
			  const Config& conf);
*/

template<typename Solver>
std::vector<std::shared_ptr<BoundaryCondition<Solver>>>
create_boundary_conditions(std::shared_ptr<Solver>& solver);

#endif
