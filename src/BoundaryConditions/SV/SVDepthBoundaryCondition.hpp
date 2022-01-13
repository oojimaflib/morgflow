/***********************************************************************
 * BoundaryConditions/SV/SVDepthBoundaryCondition.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef BoundaryConditions_SV_SVDepthBoundaryCondition_hpp
#define BoundaryConditions_SV_SVDepthBoundaryCondition_hpp

template<typename FuncType>
class DepthSVBoundaryCondition : public BoundaryCondition<SVSolver>
{
public:

  using MeshType = typename SVSolver::MeshType;
  using ValueType = typename SVSolver::ValueType;

protected:

  using HFieldType = CellField<ValueType, MeshType>;
  using HFieldVectorType = CellFieldVector<ValueType, MeshType, 2>;
  
  using FieldModifierType = FieldModifier<HFieldType>;

  FieldModifierType modifier_;
  
  using FieldFunctorType = FuncType;// FixedValueFieldFunctor<ValueType, MeshType>;

  FieldFunctorType functor_;
  
public:

  DepthSVBoundaryCondition(const std::string& name,
			   const MeshSelection<MeshType, FieldMapping::Cell>& sel,
			   const FieldFunctorType& value_functor)
    : BoundaryCondition<SVSolver>(name),
      modifier_(name, sel, 0.0f, 1.0f,
		std::numeric_limits<ValueType>::lowest(),
		std::numeric_limits<ValueType>::max(),
		std::numeric_limits<ValueType>::lowest()),
      functor_(value_functor)
  {}
  
  virtual typename BoundaryCondition<SVSolver>::Variable get_variable(void) const
  {
    return BoundaryCondition<SVSolver>::Variable::h;
  }
  
  virtual void update(TemporalScheme<SVSolver>& ts,
		      const double& t0, const double& t1) const
  {
    // Get the depth boundary field
    CellFieldVector<ValueType, MeshType, 2>& h_in = ts.solver().h_in();

    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FieldFunctorType>
      (modifier_, functor_, t0, h_in.at(0));
    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FieldFunctorType>
      (modifier_, functor_, t1, h_in.at(1));
  }
  
};

#endif
