/***********************************************************************
 * FieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctor_hpp
#define FieldFunctor_hpp

class FieldFunctor
{
public:

  FieldFunctor(void) {}

  virtual ~FieldFunctor(void) {}

  virtual std::string name(void) const = 0;

};

#include "FieldFunctors/Operations/FieldFunctorCountOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorLnMeanOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorLnStdDevOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorMaximumOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorMeanOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorMinimumOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorStdDevOperation.hpp"
#include "FieldFunctors/Operations/FieldFunctorSumOperation.hpp"

#include "FieldFunctors/FixedValueFieldFunctor.hpp"
#include "FieldFunctors/RandomValueFieldFunctor.hpp"
#include "FieldFunctors/TimeSeriesValueFieldFunctor.hpp"
#include "FieldFunctors/InterpolatedTimeSeriesValueFieldFunctor.hpp"
#include "FieldFunctors/HemisphereFieldFunctor.hpp"
#include "FieldFunctors/SlopeFieldFunctor.hpp"
#include "FieldFunctors/RasterFieldValueFieldFunctor.hpp"



#endif

