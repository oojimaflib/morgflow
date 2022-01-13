/***********************************************************************
 * OutputFunction.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef OutputFunction_hpp
#define OutputFunction_hpp

#include "FieldVector.hpp"

template<typename T,
	 typename MeshDefn>
class OutputFunction
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;

  OutputFunction(void)
  {}

  virtual ~OutputFunction(void)
  {}

  virtual std::string name(void) const = 0;

  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const = 0;
  
  virtual size_t output_size(void) const = 0;
  
  virtual typename MeshType::CoordType output_coordinates(size_t i) const = 0;

  virtual std::string output_wkt(size_t i) const = 0;

  virtual std::vector<ValueType> output_values(size_t i) const = 0;
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class FieldMappedOutputFunction;

template<typename T,
	 typename MeshDefn>
class FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Cell>
  : public OutputFunction<T,MeshDefn>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;

  FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Cell>(void)
    : OutputFunction<T,MeshDefn>()
  {}

  virtual ~FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Cell>(void)
  {}

  virtual size_t output_size(void) const
  {
    return this->mesh_definition()->template object_count<FieldMapping::Cell>();
  }
  
  virtual typename MeshType::CoordType output_coordinates(size_t i) const
  {
    return this->mesh_definition()->template get_object_coordinate<FieldMapping::Cell>(i);
  }

  virtual std::string output_wkt(size_t i) const
  {
    return this->mesh_definition()->template get_object_wkt<FieldMapping::Cell>(i);
  }
  
};

template<typename T,
	 typename MeshDefn>
class FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Face>
  : public OutputFunction<T,MeshDefn>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;

  FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Face>(void)
    : OutputFunction<T,MeshDefn>()
  {}

  virtual ~FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Face>(void)
  {}

  virtual size_t output_size(void) const
  {
    return this->mesh_definition()->template object_count<FieldMapping::Face>();
  }
  
  virtual typename MeshType::CoordType output_coordinates(size_t i) const
  {
    return this->mesh_definition()->template get_object_coordinate<FieldMapping::Face>(i);
  }

  virtual std::string output_wkt(size_t i) const
  {
    return this->mesh_definition()->template get_object_wkt<FieldMapping::Face>(i);
  }
  
};

template<typename T,
	 typename MeshDefn>
class FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Vertex>
  : public OutputFunction<T,MeshDefn>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;

  FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Vertex>(void)
    : OutputFunction<T,MeshDefn>()
  {}

  virtual ~FieldMappedOutputFunction<T,MeshDefn,FieldMapping::Vertex>(void)
  {}

  virtual size_t output_size(void) const
  {
    return this->mesh_definition()->template object_count<FieldMapping::Vertex>();
  }
  
  virtual typename MeshType::CoordType output_coordinates(size_t i) const
  {
    return this->mesh_definition()->template get_object_coordinate<FieldMapping::Vertex>(i);
  }

  virtual std::string output_wkt(size_t i) const
  {
    return this->mesh_definition()->template get_object_wkt<FieldMapping::Vertex>(i);
  }
  
};

template<typename T,
  typename MeshDefn,
  FieldMapping FM>
class IsNaNOutputFunction : public FieldMappedOutputFunction<T,MeshDefn,FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using FieldType = Field<T,MeshDefn,FM>;

  std::string name_;
  FieldType* f_ptr_;
  
  IsNaNOutputFunction(const std::string& name,
		      FieldType* f)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      name_(name),
      f_ptr_(f)
  {
    f_ptr_->move_to_host();
  }

  virtual ~IsNaNOutputFunction(void)
  {
    f_ptr_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return name_;
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return f_ptr_->mesh_definition();
  }
  
  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return { ValueType(std::isnan(f_ptr_->host_vector().at(i)) ? 0 : 1) };
  }
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM,
	 typename... Ts>
class MultiFieldOutputFunction
  : public FieldMappedOutputFunction<T, MeshDefn, FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using FieldType = Field<T,MeshDefn,FM>;

  std::string name_;
  std::array<FieldType,sizeof...(Ts)> fields_;

  MultiFieldOutputFunction(const std::string& name,
			   const Field<Ts,MeshDefn,FM>& ...args)
    : FieldMappedOutputFunction<ValueType,MeshType,FM>(),
      name_(name),
      fields_({field_cast<Field<Ts,MeshDefn,FM>,
	  Field<T,MeshDefn,FM>>(args.name(), args)...})
  {
    for (size_t i = 0; i < (sizeof...(Ts)); ++i) {
      fields_.at(i).move_to_host();
    }
  }
  
  virtual ~MultiFieldOutputFunction(void)
  {
  }

  virtual std::string name(void) const
  {
    return name_;
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return fields_.at(0).mesh_definition();
  }
  
  virtual std::vector<ValueType> output_values(size_t j) const
  {
    std::vector<ValueType> v;
    for (size_t i = 0; i < (sizeof...(Ts)); ++i) {
      v.push_back(fields_.at(i).host_vector().at(j));
    }
    return v;
  }
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class SingleFieldOutputFunction
  : public FieldMappedOutputFunction<T,MeshDefn,FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using FieldType = Field<T,MeshDefn,FM>;

  std::shared_ptr<FieldType> f_ptr_;
  
  SingleFieldOutputFunction(const FieldType& f)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      f_ptr_(std::make_shared<FieldType>(f))
  {
    f_ptr_->move_to_host();
  }

  virtual ~SingleFieldOutputFunction(void)
  {
  }

  virtual std::string name(void) const
  {
    return f_ptr_->name();
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return f_ptr_->mesh_definition();
  }
  
  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return { f_ptr_->host_vector().at(i) };
  }
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class DepthOutputFunction : public FieldMappedOutputFunction<T,MeshDefn,FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using DepthFieldType = Field<T,MeshDefn,FM>;

  DepthFieldType* h_ptr_;
  
  DepthOutputFunction(DepthFieldType* h)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      h_ptr_(h)
  {
    h_ptr_->move_to_host();
  }

  virtual ~DepthOutputFunction(void)
  {
    h_ptr_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return "depth";
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return h_ptr_->mesh_definition();
  }
  
  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return { h_ptr_->host_vector().at(i) };
  }
  
};



/*
template<typename Tzbed,
	 typename Th,
	 typename MeshDefn,
	 FieldMapping FM>
class Stage2OutputFunction : public FieldMappedOutputFunction<Th,MeshDefn,FM>
{
public:

  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using ZBedFieldType = Field<Tzbed,MeshDefn,FM>;
  using DepthFieldType = Field<Th,MeshDefn,FM>;

  ZBedFieldType* zb_ptr_;
  DepthFieldType* h_ptr_;
  
  StageOutputFunction(ZBedFieldType* zb,
		      DepthFieldType* h)
    : FieldMappedOutputFunction<Th, MeshType, FM>(),
      zb_ptr_(zb),
      h_ptr_(h)
  {
    zb_ptr_->move_to_host();
    h_ptr_->move_to_host();
  }

  virtual ~StageOutputFunction(void)
  {
    zb_ptr_->move_to_device();
    h_ptr_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return "stage";
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return h_ptr_->mesh_definition();
  }
  
  virtual std::vector<Th> output_values(size_t i) const
  {
    return { Th(h_ptr_->host_vector().at(i) + zb_ptr_->host_vector().at(i)) };
  }
  
};
*/

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class ComponentVelocityOutputFunction
  : public FieldMappedOutputFunction<T,MeshDefn, FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using VelFieldType = Field<T,MeshDefn,FM>;

  VelFieldType* u_ptr_;
  VelFieldType* v_ptr_;
  
  ComponentVelocityOutputFunction(VelFieldType* u, VelFieldType* v)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      u_ptr_(u), v_ptr_(v)
  {
    u_ptr_->move_to_host();
    v_ptr_->move_to_host();
  }

  virtual ~ComponentVelocityOutputFunction(void)
  {
    u_ptr_->move_to_device();
    v_ptr_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return "velocity";
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return u_ptr_->mesh_definition();
  }
  
  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return {
      u_ptr_->host_vector().at(i),
      v_ptr_->host_vector().at(i)
    };
  }
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class DebugBoundaryOutputFunction
  : public FieldMappedOutputFunction<T,MeshDefn,FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using BdyFieldVectorType = FieldVector<T,MeshDefn,FM,2>;

  BdyFieldVectorType* Q_in_;
  BdyFieldVectorType* h_in_;
  
  DebugBoundaryOutputFunction(BdyFieldVectorType* Q_in,
			      BdyFieldVectorType* h_in)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      Q_in_(Q_in), h_in_(h_in)
  {
    Q_in_->move_to_host();
    h_in_->move_to_host();
  }

  virtual ~DebugBoundaryOutputFunction(void)
  {
    Q_in_->move_to_device();
    h_in_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return "debug boundaries";
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return Q_in_->mesh_definition();
  }

  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return {
      Q_in_->at(0).host_vector().at(i),
      Q_in_->at(1).host_vector().at(i),
      h_in_->at(0).host_vector().at(i),
      h_in_->at(1).host_vector().at(i)
    };
  }
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class DebugSlopeOutputFunction
  : public FieldMappedOutputFunction<T,MeshDefn,FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using SlopeFieldVectorType = FieldVector<T,MeshDefn,FM,3>;

  SlopeFieldVectorType* dUdx_;
  SlopeFieldVectorType* dUdy_;
  
  DebugSlopeOutputFunction(SlopeFieldVectorType* dUdx,
			   SlopeFieldVectorType* dUdy)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      dUdx_(dUdx), dUdy_(dUdy)
  {
    dUdx_->move_to_host();
    dUdy_->move_to_host();
  }

  virtual ~DebugSlopeOutputFunction(void)
  {
    dUdx_->move_to_device();
    dUdy_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return "debug slopes";
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return dUdx_->mesh_definition();
  }

  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return {
      dUdx_->at(0).host_vector().at(i),
      dUdx_->at(1).host_vector().at(i),
      dUdx_->at(2).host_vector().at(i),
      dUdy_->at(0).host_vector().at(i),
      dUdy_->at(1).host_vector().at(i),
      dUdy_->at(2).host_vector().at(i)
    };
  }
  
};

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class DebugFluxOutputFunction : public FieldMappedOutputFunction<T,MeshDefn,FM>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  using FluxFieldVectorType = FieldVector<T,MeshDefn,FM,4>;

  FluxFieldVectorType* flux_;
  
  DebugFluxOutputFunction(FluxFieldVectorType* flux)
    : FieldMappedOutputFunction<ValueType, MeshType, FM>(),
      flux_(flux)
  {
    flux_->move_to_host();
  }

  virtual ~DebugFluxOutputFunction(void)
  {
    flux_->move_to_device();
  }

  virtual std::string name(void) const
  {
    return "debug fluxes";
  }
  
  virtual const std::shared_ptr<MeshDefn> mesh_definition(void) const
  {
    return flux_->mesh_definition();
  }

  virtual std::vector<ValueType> output_values(size_t i) const
  {
    return {
      flux_->at(0).host_vector().at(i),
      flux_->at(1).host_vector().at(i),
      flux_->at(2).host_vector().at(i),
      flux_->at(3).host_vector().at(i)
    };
  }
  
};

#endif
