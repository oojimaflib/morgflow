/***********************************************************************
 * BoundaryConditions/SV/SVSourceBoundaryCondition.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef BoundaryConditions_SV_SVSourceBoundaryCondition_hpp
#define BoundaryConditions_SV_SVSourceBoundaryCondition_hpp

template<typename FuncType>
class SourceSVBoundaryCondition : public BoundaryCondition<SVSolver>
{
public:

  using MeshType = typename SVSolver::MeshType;
  using ValueType = typename SVSolver::ValueType;

protected:

  using QFieldType = CellField<ValueType, MeshType>;
  using QFieldVectorType = CellFieldVector<ValueType, MeshType, 2>;
  
  using FieldModifierType = FieldModifier<QFieldType>;

  FieldModifierType modifier_;
  
  using FieldFunctorType = FuncType;// FixedValueFieldFunctor<ValueType, MeshType>;

  FieldFunctorType functor_;
  
public:

  SourceSVBoundaryCondition(const std::string& name,
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
    return BoundaryCondition<SVSolver>::Variable::Q;
  }
  
  virtual void update(TemporalScheme<SVSolver>& ts,
		      const double& t0, const double& t1) const
  {
    // Get the flow boundary field
    CellFieldVector<ValueType, MeshType, 2>& Q_in = ts.solver().Q_in();

    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FieldFunctorType>
      (modifier_, functor_, t0, Q_in.at(0));
    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FieldFunctorType>
      (modifier_, functor_, t1, Q_in.at(1));
  }
  
};

#endif
