/***********************************************************************
 * FieldModifier.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldModifier_hpp
#define FieldModifier_hpp

#include "Config.hpp"
#include "Field.hpp"

#include "MeshSelection.hpp"
#include "FieldFunctor.hpp"

template<typename FieldType_>
class FieldModifier
{
public:

  using FieldType = FieldType_;
  using ValueType = typename FieldType::ValueType;
  using MeshType = typename FieldType::MeshType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  using MeshSelectionType = MeshSelection<MeshType,FieldMappingType>;

  enum class IntegrationType {
    centroid,
    box,
  };
  
protected:

  std::string name_;

  MeshSelectionType sel_;
  
  ValueType offset_;
  ValueType factor_;
  ValueType min_;
  ValueType max_;
  ValueType nodata_;

  IntegrationType integration_type_;

  std::array<double,2> box_size_;
  
public:

  FieldModifier(const std::shared_ptr<sycl::queue>& queue,
		const std::shared_ptr<MeshType>& meshdefn_p,
		const Config& config)
    : name_(config.get<std::string>("name", "anon")),
      sel_(queue, meshdefn_p, config.get_child("selection", Config())),
      offset_(config.get<ValueType>("offset", 0.0)),
      factor_(config.get<ValueType>("factor", 1.0)),
      min_(config.get<ValueType>("minimum", std::numeric_limits<ValueType>::lowest())),
      max_(config.get<ValueType>("maximum", std::numeric_limits<ValueType>::max())),
      nodata_(config.get<ValueType>("nodata", -9999.0))
  {
    std::string int_type_str = config.get<std::string>("integration type",
						       "centroid");
    if (int_type_str == "centroid") {
      integration_type_ = IntegrationType::centroid;
    } else if (int_type_str == "box") {
      integration_type_ = IntegrationType::box;
      std::string user_box_size = config.get<std::string>("box size", "");
      if (user_box_size != "") {
	box_size_ = split_string<double,2>(user_box_size);
      } else {
	box_size_ = meshdefn_p->cell_size();
      }
    } else {
      std::cerr << "Unknown integration type: " << int_type_str << std::endl;
      throw std::runtime_error("Unknown integration type");
    }
  }
  
  FieldModifier(const std::string& name,
		const MeshSelectionType& sel,
		const ValueType& offset,
		const ValueType& factor,
		const ValueType& min,
		const ValueType& max,
		const ValueType& nodata,
		const IntegrationType& int_type = IntegrationType::centroid)
    : name_(name), sel_(sel),
      offset_(offset), factor_(factor),
      min_(min), max_(max), nodata_(nodata),
      integration_type_(int_type)
  {}

  ~FieldModifier(void) {}

  const std::string& name(void) const { return name_; }

  const MeshSelectionType& selection(void) const
  {
    return sel_;
  }

  const ValueType& offset(void) const { return offset_; }
  const ValueType& factor(void) const { return factor_; }
  const ValueType& min(void) const { return min_; }
  const ValueType& max(void) const { return max_; }
  const ValueType& nodata(void) const { return nodata_; }

  const IntegrationType& integration_type(void) const
  {
    return integration_type_;
  }

  const std::array<double,2>& box_size(void) const
  {
    return box_size_;
  }
  
};

/*
template<typename FieldType,
	 typename Operation>
class FieldModifierOperation : public FieldModifier<FieldType>
{
public:

  using FieldType = FieldType;
  
protected:

  Operation op_;

public:

  FieldModifierOperation(const std::string& name,
			 const MeshSelection<MeshType,FieldMappingType>& sel,
			 const ValueType& offset,
			 const ValueType& factor,
			 const ValueType& min,
			 const ValueType& max,
			 const ValueType& nodata)
    : FieldModifier<FieldType>(name, sel, offset, factor, min, max, nodata),
      op_()
  {}

  virtual ~FieldModifierOperation(void) {}

  virtual void apply(sycl::handler& cgh, FieldType& field) = 0;
  
};
*/

template<typename T>
class SetOperation
{
public:
  T operator()(const T& existing, const T& value) const
  {
    return value;
    (void) existing;
  }

  std::string name(void) const
  {
    return "Set";
  }
};

template<typename T>
class OffsetOperation
{
public:
  T operator()(const T& existing, const T& value) const
  {
    return existing + value;
  }

  std::string name(void) const
  {
    return "Offset";
  }
};

template<typename T>
class FactorOperation
{
public:
  T operator()(const T& existing, const T& value) const
  {
    return existing * value;
  }

  std::string name(void) const
  {
    return "Factor";
  }
};

/*
template<typename FieldModifierType,
	 typename Operation,
	 typename FieldFunctor>
class GlobalFieldModifierKernel
{
public:

  using FieldType = typename FieldModifierType::FieldType;
  using MeshType = typename FieldType::MeshType;
  using CoordType = typename MeshType::CoordType;
  using ValueType = typename FieldType::ValueType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
private:

  FieldModifierType fm_;
  Operation op_;
  FieldFunctor func_;
  double time_;
  MeshType mesh_;

  using Accessor = typename FieldType::
    template Accessor<sycl::access::mode::read_write>;

  Accessor field_rw_;

public:

  GlobalFieldModifierKernel(sycl::handler& cgh,
			    const FieldModifierType& modifier,
			    const Operation& op,
			    const FieldFunctor& func,
			    const double& time,
			    FieldType& field)
    : fm_(modifier), op_(op), func_(func),
      mesh_(*(field.mesh_definition())),
      time_(time),
      field_rw_(field.get_read_write_accessor(cgh))
  {
    // std::cout << "NoData:" << fm_.nodata() << std::endl;
    func_.bind(cgh);
  }

  void operator()(sycl::item<1> item) const
  {
    size_t i = item.get_linear_id();
    
    ValueType value;
    switch (fm_.integration_type()) {
    case FieldModifierType::IntegrationType::centroid:
      {
	CoordType coord = mesh_.template get_object_coordinate<FieldMappingType>(i);
	value = func_(time_, coord, fm_.nodata());
      }
      break;
    case FieldModifierType::IntegrationType::box:
      {
	CoordType coord = mesh_.template get_object_coordinate<FieldMappingType>(i);
	value = func_(time_, coord, fm_.box_size(), fm_.nodata());
      }
      break;
    };
    
    if (!std::isnan(value) and value != fm_.nodata()) {
      ValueType cval = sycl::clamp(fm_.offset() + fm_.factor() * value,
				   fm_.min(), fm_.max());
      // std::cout << "GFM:\t" << coord[0] << "\t" << coord[1] << "\t" << value << "\t" << cval << std::endl;
      field_rw_[i] = op_(field_rw_[i], cval);
    }
  }
};
 
template<typename FieldModifierType,
	 typename Operation,
	 typename FieldFunctor>
class SelectionFieldModifierKernel
{
public:

  using FieldType = typename FieldModifierType::FieldType;
  using MeshType = typename FieldType::MeshType;
  using CoordType = typename MeshType::CoordType;
  using ValueType = typename FieldType::ValueType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
private:

  FieldModifierType fm_;
  Operation op_;
  FieldFunctor func_;
  double time_;
  MeshType mesh_;

  MeshSelectionAccessor sel_ro_;
  
  using Accessor = typename FieldType::
    template Accessor<sycl::access::mode::read_write>;

  Accessor field_rw_;

public:

  SelectionFieldModifierKernel(sycl::handler& cgh,
			       const FieldModifierType& modifier,
			       const Operation& op,
			       const FieldFunctor& func,
			       const double& time,
			       FieldType& field)
    : fm_(modifier), op_(op), func_(func),
      mesh_(*(field.mesh_definition())),
      sel_ro_(modifier.selection().get_read_accessor(cgh)),
      time_(time),
      field_rw_(field.get_read_write_accessor(cgh))
  {
    func_.bind(cgh);
  }

  void operator()(sycl::item<1> item) const
  {
    size_t sel_i = item.get_linear_id();
    size_t i = sel_ro_[sel_i];
    CoordType coord = mesh_.template get_object_coordinate<FieldMappingType>(i);
    ValueType value = func_(time_, coord, fm_.nodata());
    if (!std::isnan(value) and value != fm_.nodata()) {
      ValueType cval = sycl::clamp(fm_.offset() + fm_.factor() * value,
				   fm_.min(), fm_.max());
      field_rw_[i] = op_(field_rw_[i], cval);
    }
  }
};

template<typename FieldType_>
class SetNaNFieldModifierKernel
{
public:

  using FieldType = FieldType_;
  using ValueType = typename FieldType::ValueType;
  using MeshType = typename FieldType::MeshType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
  using MeshSelectionType = MeshSelection<MeshType,FieldMappingType>;
  using CoordType = typename MeshType::CoordType;
  
private:

  MeshSelectionAccessor sel_ro_;
  
  using Accessor = typename FieldType::
    template Accessor<sycl::access::mode::read_write>;

  Accessor field_rw_;

public:

  SetNaNFieldModifierKernel(sycl::handler& cgh,
			    const MeshSelectionType& sel,
			    FieldType& field)
    : sel_ro_(sel.get_read_accessor(cgh)),
      field_rw_(field.get_read_write_accessor(cgh))
  {
  }

  void operator()(sycl::item<1> item) const
  {
    size_t sel_i = item.get_linear_id();
    size_t i = sel_ro_[sel_i];
    field_rw_[i] = std::numeric_limits<ValueType>::quiet_NaN();
  }
};
*/

template<typename FieldModifierType,
	 typename FieldFunctor>
struct ValueCalculator
{
public:

  using FieldType = typename FieldModifierType::FieldType;
  using MeshType = typename FieldType::MeshType;
  using CoordType = typename MeshType::CoordType;
  using ValueType = typename FieldType::ValueType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;

  FieldModifierType fm_;
  FieldFunctor func_;
  double time_;
  MeshType mesh_;

  ValueCalculator(const FieldModifierType& modifier,
		  const FieldFunctor& func,
		  const MeshType& mesh,
		  const double& time)
    : fm_(modifier), func_(func),
      mesh_(mesh),
      time_(time)
  {}
  
  ValueType get_value(const size_t& i) const
  {
    ValueType value;
    switch (fm_.integration_type()) {
    case FieldModifierType::IntegrationType::centroid:
      {
	CoordType coord = mesh_.template get_object_coordinate<FieldMappingType>(i);
	value = func_(time_, coord, fm_.nodata());
      }
      break;
    case FieldModifierType::IntegrationType::box:
      {
	CoordType coord = mesh_.template get_object_coordinate<FieldMappingType>(i);
	value = func_(time_, coord, fm_.box_size(), fm_.nodata());
      }
      break;
    };
    
    if (!std::isnan(value) and value != fm_.nodata()) {
      ValueType cval = sycl::clamp(fm_.offset() + fm_.factor() * value,
				   fm_.min(), fm_.max());
      return cval;
    }
    return std::numeric_limits<ValueType>::quiet_NaN();
  }
  
};

template<typename FieldModifierType,
	 typename Operation,
	 typename FieldFunctor>
class GlobalFieldModifierKernel
{
public:

  using FieldType = typename FieldModifierType::FieldType;
  using MeshType = typename FieldType::MeshType;
  using CoordType = typename MeshType::CoordType;
  using ValueType = typename FieldType::ValueType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
private:

  using Accessor = typename FieldType::
    template Accessor<sycl::access::mode::read_write>;

  ValueCalculator<FieldModifierType,FieldFunctor> vc_;

  Operation op_;
  
  Accessor field_rw_;

public:

  GlobalFieldModifierKernel(sycl::handler& cgh,
			    const FieldModifierType& modifier,
			    const Operation& op,
			    const FieldFunctor& func,
			    const double& time,
			    FieldType& field)
    : vc_(modifier, func, *(field.mesh_definition()), time),
      op_(op), 
      field_rw_(field.get_read_write_accessor(cgh))
  {
    vc_.func_.bind(cgh);
  }

  void operator()(sycl::item<1> item) const
  {
    size_t i = item.get_linear_id();

    ValueType value = vc_.get_value(i);
    if (!std::isnan(value)) {
      field_rw_[i] = op_(field_rw_[i], value);
    }
  }
				
};
 
template<typename FieldModifierType,
	 typename Operation,
	 typename FieldFunctor>
class SelectionFieldModifierKernel
{
public:

  using FieldType = typename FieldModifierType::FieldType;
  using MeshType = typename FieldType::MeshType;
  using CoordType = typename MeshType::CoordType;
  using ValueType = typename FieldType::ValueType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
private:

  using Accessor = typename FieldType::
    template Accessor<sycl::access::mode::read_write>;

  ValueCalculator<FieldModifierType,FieldFunctor> vc_;
  
  Operation op_;

  MeshSelectionAccessor sel_ro_;
  
  Accessor field_rw_;

public:

  SelectionFieldModifierKernel(sycl::handler& cgh,
			       const FieldModifierType& modifier,
			       const Operation& op,
			       const FieldFunctor& func,
			       const double& time,
			       FieldType& field)
    : vc_(modifier, func, *(field.mesh_definition()), time),
      op_(op),
      sel_ro_(modifier.selection().get_read_accessor(cgh)),
      field_rw_(field.get_read_write_accessor(cgh))
  {
    vc_.func_.bind(cgh);
  }

  void operator()(sycl::item<1> item) const
  {
    size_t sel_i = item.get_linear_id();
    size_t i = sel_ro_[sel_i];

    ValueType value = vc_.get_value(i);
    if (!std::isnan(value)) {
      field_rw_[i] = op_(field_rw_[i], value);
    }
  }
};

template<typename FieldType_>
class SetNaNFieldModifierKernel
{
public:

  using FieldType = FieldType_;
  using ValueType = typename FieldType::ValueType;
  using MeshType = typename FieldType::MeshType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
  using MeshSelectionType = MeshSelection<MeshType,FieldMappingType>;
  using CoordType = typename MeshType::CoordType;
  
private:

  MeshSelectionAccessor sel_ro_;
  
  using Accessor = typename FieldType::
    template Accessor<sycl::access::mode::read_write>;

  Accessor field_rw_;

public:

  SetNaNFieldModifierKernel(sycl::handler& cgh,
			    const MeshSelectionType& sel,
			    FieldType& field)
    : sel_ro_(sel.get_read_accessor(cgh)),
      field_rw_(field.get_read_write_accessor(cgh))
  {
  }

  void operator()(sycl::item<1> item) const
  {
    size_t sel_i = item.get_linear_id();
    size_t i = sel_ro_[sel_i];
    field_rw_[i] = std::numeric_limits<ValueType>::quiet_NaN();
  }
};

template<typename FieldModifierType,
	 typename Operation,
	 typename FieldFunctor>
void modify_field(FieldModifierType modifier,
		  const FieldFunctor& func,
		  const double& time,
		  typename FieldModifierType::FieldType& field)
{
  using FieldType = typename FieldModifierType::FieldType;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;

  Operation op;

  if constexpr (FieldFunctor::host_only) {
    std::cout << "Host only." << std::endl;
    bool field_was_on_device = field.is_on_device();
    if (field_was_on_device) field.move_to_host();

    ValueCalculator<FieldModifierType, FieldFunctor> vc(modifier, func, *(field.mesh_definition()), time);
    
    if (modifier.selection().is_global()) {
      for (size_t i = 0; i < field.size(); ++i) {
	auto value = vc.get_value(i);
	if (!std::isnan(value)) {
	  field.host_vector()[i] = op(field.host_vector()[i], value);
	}
      }
    } else {
      auto sel_list_ptr = modifier.selection().list_ptr();
      sel_list_ptr->move_to_host();
      for (size_t sel_i = 0; sel_i < modifier.selection().size(); ++sel_i) {
	size_t i = sel_list_ptr->host_vector()[sel_i];
	auto value = vc.get_value(i);
	if (!std::isnan(value)) {
	  field.host_vector()[i] = op(field.host_vector()[i], value);
	}
      }
      sel_list_ptr->move_to_device();
    }
    
    if (field_was_on_device) field.move_to_device();
  } else {
    if (modifier.selection().is_global()) {
      field.queue().submit([&] (sycl::handler& cgh)
      {
	using KernelType = GlobalFieldModifierKernel<FieldModifierType,Operation,FieldFunctor>;
	KernelType kernel(cgh, modifier, op, func, time, field);
	cgh.parallel_for(sycl::range<1>(field.mesh_definition()->template object_count<KernelType::FieldMappingType>()), kernel);
      });
    } else {
      field.queue().submit([&] (sycl::handler& cgh)
      {
	using KernelType = SelectionFieldModifierKernel<FieldModifierType,Operation,FieldFunctor>;
	KernelType kernel(cgh, modifier, op, func, time, field);
	cgh.parallel_for(sycl::range<1>(modifier.selection().size()), kernel);
      });
    }
  }
}

template<typename FieldType>
void set_field_nan(MeshSelection<typename FieldType::MeshType,
		   FieldType::FieldMappingType>& selection,
		   FieldType& field)
{
  field.queue().submit([&] (sycl::handler& cgh)
  {
    SetNaNFieldModifierKernel<FieldType> kern(cgh, selection, field);
    cgh.parallel_for(sycl::range<1>(selection.size()), kern);
  });
}

#endif
