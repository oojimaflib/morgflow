/***********************************************************************
 * FieldGenerator.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldGenerator_hpp
#define FieldGenerator_hpp

#include "Config.hpp"
#include "FieldModifier.hpp"
#include "Field.hpp"

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM,
	 typename Operation,
	 typename Functor>
void modify_generated_field(Field<T,MeshDefn,FM>& field,
			    Functor func,
			    const double& time,
			    const Config& config)
{
  using FieldType = Field<T,MeshDefn,FM>;
  using FieldModifierType = FieldModifier<FieldType>;

  modify_field<FieldModifierType,Operation,Functor>
    (FieldModifierType(field.queue_ptr(), field.mesh_definition(), config),
     func, time, field);
}

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM,
	 typename Operation,
	 typename FFOp>
void modify_generated_field(Field<T,MeshDefn,FM>& field,
			    const Config& config)
{
  using CoordType = typename MeshDefn::CoordType;
  const std::string& functor_name = config.get_value<std::string>();
  if (functor_name == "fixed") {
    modify_generated_field<T,MeshDefn,FM,
			   Operation,
			   FixedValueFieldFunctor<T,CoordType,FFOp>>
      (field,
       FixedValueFieldFunctor<T,CoordType,FFOp>(field.queue_ptr(),
						config),
       0.0,
       config);
  } else if (functor_name == "random") {
    modify_generated_field<T,MeshDefn,FM,
			   Operation,
			   RandomValueFieldFunctor<T,CoordType,FFOp>>
      (field,
       RandomValueFieldFunctor<T,CoordType,FFOp>(field.queue_ptr(),
						 config),
       0.0,
       config);
  } else if (functor_name == "hemisphere") {
    modify_generated_field<T,MeshDefn,FM,
			   Operation,
			   HemisphereFieldFunctor<T,CoordType,FFOp>>
      (field,
       HemisphereFieldFunctor<T,CoordType,FFOp>(field.queue_ptr(),
						config),
       0.0,
       config);
  } else if (functor_name == "slope") {
    modify_generated_field<T,MeshDefn,FM,
			   Operation,
			   SlopeFieldFunctor<T,CoordType,FFOp>>
      (field,
       SlopeFieldFunctor<T,CoordType,FFOp>(field.queue_ptr(),
					   config),
       0.0,
       config);
  } else if (functor_name == "raster") {
    std::cout << "Modifying field with raster" << std::endl;
    modify_generated_field<T,MeshDefn,FM,
			   Operation,
			   RasterFieldValueFieldFunctor<T,CoordType,FFOp>>
      (field,
       RasterFieldValueFieldFunctor<T,CoordType,FFOp>(field.queue_ptr(),
						      config),
       0.0,
       config);
  } else {
    std::cerr << "Unknown field modification data source: "
	      << functor_name << std::endl;
    throw std::runtime_error("Unknown field modification data source.");
  }
}

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM,
	 typename Operation>
void modify_generated_field(Field<T,MeshDefn,FM>& field,
			    const Config& config)
{
  const std::string& ffop_name =
    config.get<std::string>("operation", "mean");
  if (ffop_name == "mean") {
    std::cout << "Modifying field using mean values of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorMeanOperation<T>>(field, config);
  } else if (ffop_name == "log mean") {
    std::cout << "Modifying field using mean of log of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorLnMeanOperation<T>>(field, config);
  } else if (ffop_name == "std dev") {
    std::cout << "Modifying field using standard deviation of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorStdDevOperation<T>>(field, config);
  } else if (ffop_name == "log std dev") {
    std::cout << "Modifying field using standard deviation of log of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorLnStdDevOperation<T>>(field, config);
  } else if (ffop_name == "min") {
    std::cout << "Modifying field using minimum of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorMinimumOperation<T>>(field, config);
  } else if (ffop_name == "max") {
    std::cout << "Modifying field using maximum of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorMaximumOperation<T>>(field, config);
  } else if (ffop_name == "sum") {
    std::cout << "Modifying field using integration of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorSumOperation<T>>(field, config);
  } else if (ffop_name == "count") {
    std::cout << "Modifying field using count of functor" << std::endl;
    modify_generated_field<T,MeshDefn,FM,Operation,
			   FieldFunctorCountOperation<T>>(field, config);
  } else {
    std::cerr << "Unknown operation for field functor: "
	      << ffop_name << std::endl;
    throw std::runtime_error("Unknown field functor operation.");
  }
}

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
Field<T,MeshDefn,FM> generate_field(std::shared_ptr<sycl::queue>& queue,
				    const std::string& name,
				    std::shared_ptr<MeshDefn>& meshdefn_p,
				    const T& default_value,
				    bool keep_on_device = true)
{
  std::cout << "Generating field \"" << name << "\"" << std::endl;
  Field<T,MeshDefn,FM> field(queue, name, meshdefn_p, true, default_value);
  
  // Get any configuration that's available for this field
  Config empty;
  const Config& conf =
    GlobalConfig::instance().configuration().get_child(name, empty);

  for (auto&& kv : conf) {
    const std::string& key = kv.first;
    const Config& mod_conf = kv.second;
    if (key == "set") {
      modify_generated_field<T,MeshDefn,FM,SetOperation<T>>(field, mod_conf);
    } else if (key == "offset") {
      modify_generated_field<T,MeshDefn,FM,OffsetOperation<T>>(field, mod_conf);
    } else if (key == "factor") {
      modify_generated_field<T,MeshDefn,FM,FactorOperation<T>>(field, mod_conf);
    }
  }

  if (not keep_on_device) {
    field.move_to_host();
  }
  
  return field;
}

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
void generate_field(Field<T, MeshDefn, FM>& field)
{
  std::cout << "Generating field \"" << field.name() << "\"" << std::endl;

  field.move_to_device();
  
  // Get any configuration that's available for this field
  Config empty;
  const Config& conf =
    GlobalConfig::instance().configuration().get_child(field.name(), empty);

  for (auto&& kv : conf) {
    const std::string& key = kv.first;
    const Config& mod_conf = kv.second;
    if (key == "set") {
      modify_generated_field<T,MeshDefn,FM,SetOperation<T>>(field, mod_conf);
    } else if (key == "offset") {
      modify_generated_field<T,MeshDefn,FM,OffsetOperation<T>>(field, mod_conf);
    } else if (key == "factor") {
      modify_generated_field<T,MeshDefn,FM,FactorOperation<T>>(field, mod_conf);
    }
  }
}

#endif

