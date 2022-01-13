/***********************************************************************
 * BoundaryConditions/SVBoundaryCondition.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

// #ifndef BoundaryConditions_SVBoundaryCondition_hpp
// #define BoundaryConditions_SVBoundaryCondition_hpp

#include "SVBoundaryCondition.hpp"

/*
template<>
std::shared_ptr<BoundaryCondition<SVSolver>>
create_boundary_condition(std::shared_ptr<SVSolver>& solver,
			  const Config& conf)
{
  using MeshType = typename SVSolver::MeshType;
  using ValueType = typename SVSolver::ValueType;
  const FieldMapping MappingType = SVSolver::BCFieldMappingType;

  std::string bc_type_name = conf.get_value<std::string>();
  std::string bc_name = conf.get<std::string>("name");
  MeshSelection<MeshType,MappingType> sel(solver->queue_ptr(),
					  solver->mesh(),
					  conf.get_child("selection"));

  if (bc_type_name == "fixed source") {
    ValueType fixed_value = conf.get<ValueType>("value");
    return std::make_shared<SourceSVBoundaryCondition<FixedValueFieldFunctor<ValueType,MeshType>>>(bc_name, sel, FixedValueFieldFunctor<ValueType,MeshType>(fixed_value));
  } else if (bc_type_name == "source") {
    TimeSeriesValueFieldFunctor<ValueType,MeshType> func(solver->queue_ptr(),
							 conf.get_child("values"));
    return std::make_shared<SourceSVBoundaryCondition<TimeSeriesValueFieldFunctor<ValueType,MeshType>>>(bc_name, sel, func);
  } else if (bc_type_name == "interpolated source") {
    InterpolatedTimeSeriesValueFieldFunctor<ValueType,MeshType> func(solver->queue_ptr(),
								     conf.get_child("values"));
    return std::make_shared<SourceSVBoundaryCondition<InterpolatedTimeSeriesValueFieldFunctor<ValueType,MeshType>>>(bc_name, sel, func);
  } else if (bc_type_name == "depth") {
    TimeSeriesValueFieldFunctor<ValueType,MeshType> func(solver->queue_ptr(),
							 conf.get_child("values"));
    return std::make_shared<DepthSVBoundaryCondition<TimeSeriesValueFieldFunctor<ValueType,MeshType>>>(bc_name, sel, func);    
  } else {
    std::cerr << "Unknown boundary type: " << bc_type_name << std::endl;
    throw std::runtime_error("Unknown boundary type.");
  }
}
*/

template<typename Functor>
std::shared_ptr<BoundaryCondition<SVSolver>>
create_sv_boundary_condition(std::shared_ptr<SVSolver>& solver,
			     const Config& conf)
{
  using MeshType = typename SVSolver::MeshType;
  using ValueType = typename SVSolver::ValueType;
  const FieldMapping MappingType = SVSolver::BCFieldMappingType;

  std::string bc_type_name = conf.get_value<std::string>();
  std::string bc_name = conf.get<std::string>("name");
  MeshSelection<MeshType,MappingType> sel(solver->queue_ptr(),
					  solver->mesh(),
					  conf.get_child("selection"));
  Functor func(solver->queue_ptr(), conf.get_child("values"));
  
  if (bc_type_name == "source") {
    return std::make_shared<SourceSVBoundaryCondition<Functor>>(bc_name, sel, func);
  } else if (bc_type_name == "depth") {
    return std::make_shared<DepthSVBoundaryCondition<Functor>>(bc_name, sel, func);    
  } else {
    std::cerr << "Unknown boundary type: " << bc_type_name << std::endl;
    throw std::runtime_error("Unknown boundary type.");
  }
}

std::shared_ptr<BoundaryCondition<SVSolver>>
create_sv_boundary_condition(std::shared_ptr<SVSolver>& solver,
			     const Config& conf)
{
  using CoordType = typename SVSolver::MeshType::CoordType;
  using ValueType = typename SVSolver::ValueType;
  using boost::algorithm::to_lower_copy;
  
  const Config& value_conf = conf.get_child("values");

  std::string func_type_str =
    to_lower_copy(value_conf.get_value<std::string>());
  if (func_type_str == "fixed") {
    return create_sv_boundary_condition<FixedValueFieldFunctor<ValueType,CoordType>>(solver, conf);
  } else if (func_type_str == "hemisphere") {
    return create_sv_boundary_condition<HemisphereFieldFunctor<ValueType,CoordType>>(solver, conf);
  } else if (func_type_str == "interpolated time series") {
    return create_sv_boundary_condition<InterpolatedTimeSeriesValueFieldFunctor<ValueType, CoordType>>(solver, conf);
  } else if (func_type_str == "raster field") {
    return create_sv_boundary_condition<RasterFieldValueFieldFunctor<ValueType,CoordType>>(solver, conf);
  } else if (func_type_str == "slope") {
    return create_sv_boundary_condition<SlopeFieldFunctor<ValueType,CoordType>>(solver, conf);
  } else if (func_type_str == "time series") {
    return create_sv_boundary_condition<TimeSeriesValueFieldFunctor<ValueType,CoordType>>(solver, conf);
  } else {
    std::cerr << "Unknown boundary value specification type: " << func_type_str << std::endl;
    throw std::runtime_error("Unknown boundary value specification type.");    
  }
}

template<>
std::vector<std::shared_ptr<BoundaryCondition<SVSolver>>>
create_boundary_conditions(std::shared_ptr<SVSolver>& solver)
{
  std::vector<std::shared_ptr<BoundaryCondition<SVSolver>>> bc;

  std::cout << "Initialising boundary conditions..." << std::endl;
  const Config& config = GlobalConfig::instance().configuration();
  auto bc_crange = config.equal_range("boundary");
  for (auto it = bc_crange.first; it != bc_crange.second; ++it) {
    bc.push_back(create_sv_boundary_condition(solver, it->second));
  }

  std::cout << "Initialised " << bc.size()
	    << " boundary conditions." << std::endl;
  return bc;
}

// #endif

