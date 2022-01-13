/***********************************************************************
 * Field.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef Field_hpp
#define Field_hpp

#include "DataArray.hpp"
#include "Mesh.hpp"

template<typename T,
	 typename MeshDefn,
	 FieldMapping FM>
class Field : public DataArray<T>
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
private:

  std::string name_;

  std::shared_ptr<MeshDefn> meshdefn_p_;

public:

  Field(const std::shared_ptr<sycl::queue>& queue,
	const std::string& name,
	const std::shared_ptr<MeshDefn>& meshdefn_p,
	const T& init_value = T())
    : DataArray<T>(queue,
		   meshdefn_p->template object_count<FM>(),
		   init_value),
      name_(name),
      meshdefn_p_(meshdefn_p)
  {
    std::cout << "Created field on host \"" << name_ << "\"" << std::endl;
  }

  Field(const std::shared_ptr<sycl::queue>& queue,
	const std::string& name,
	const std::shared_ptr<MeshDefn>& meshdefn_p,
	bool on_device,
	const T& init_value = T())
    : DataArray<T>(queue,
		   meshdefn_p->template object_count<FM>(),
		   on_device, init_value),
      name_(name),
      meshdefn_p_(meshdefn_p)
  {
    std::cout << "Created field on " << (on_device ? "device" : "host")
	      << " \"" << name_ << "\"" << std::endl;
  }
  
  Field(const Field<T, MeshDefn, FM>& f)
    : DataArray<T>(f),
      name_(f.name_),
      meshdefn_p_(f.meshdefn_p_)
  {
    std::cout << "Duplicating field \"" << name_ << "\"" << std::endl;
  }

  Field(Field<T, MeshDefn, FM>&& f) = default;

  Field(const std::string& prefix,
	const Field<T, MeshDefn, FM>& f,
	const std::string& suffix)
    : DataArray<T>(f),
      name_(prefix + f.name_ + suffix),
      meshdefn_p_(f.meshdefn_p_)
  {
    std::cout << "Duplicated field \"" << f.name_ << "\" as \""
	      << name_ << "\"" << std::endl;
  }

  const std::string& name(void) const
  {
    return name_;
  }

  const std::shared_ptr<MeshDefn>& mesh_definition(void) const
  {
    return meshdefn_p_;
  }

  Field<T, MeshDefn, FM>& on_device(void)
  {
    this->move_to_device();
    return *this;
  }
  
};

#include "FieldOperators.hpp"

template<typename UnaryFieldOp>
class UnaryFieldOperationKernel
{
private:

  using SourceFieldType = typename UnaryFieldOp::SourceFieldType;
  using DestinationFieldType = typename UnaryFieldOp::DestinationFieldType;

  using SourceAccessor =
    typename SourceFieldType::template Accessor<sycl::access::mode::read>;

  using DestinationAccessor =
    typename DestinationFieldType::
    template Accessor<sycl::access::mode::discard_write>;

  SourceAccessor s_ro;
  DestinationAccessor d_wo;

public:

  UnaryFieldOperationKernel(sycl::handler& cgh,
			    const SourceFieldType& s,
			    const DestinationFieldType& d)
    : s_ro(s.get_read_accessor(cgh)),
      d_wo(d.get_discard_write_accessor(cgh))
  {}

  void operator()(sycl::item<1> item) const
  {
    d_wo[item] = UnaryFieldOp()(s_ro[item]);
  }

};

template<typename BinaryFieldOp>
class BinaryFieldOperationKernel
{
private:

  using Source1FieldType = typename BinaryFieldOp::Source1FieldType;
  using Source2FieldType = typename BinaryFieldOp::Source2FieldType;
  using DestinationFieldType = typename BinaryFieldOp::DestinationFieldType;

  using Field1Accessor =
    typename Source1FieldType::template Accessor<sycl::access::mode::read>;
  using Field2Accessor =
    typename Source2FieldType::template Accessor<sycl::access::mode::read>;
  using DestinationAccessor =
    typename DestinationFieldType::
    template Accessor<sycl::access::mode::discard_write>;

  Field1Accessor f1_ro;
  Field2Accessor f2_ro;
  DestinationAccessor d_wo;
  
public:

  BinaryFieldOperationKernel(sycl::handler& cgh,
			     const Source1FieldType& f1,
			     const Source2FieldType& f2,
			     const DestinationFieldType& dest)
    : f1_ro(f1.get_read_accessor(cgh)),
      f2_ro(f2.get_read_accessor(cgh)),
      d_wo(dest.get_discard_write_accessor(cgh))
  {}
  
  void operator()(sycl::item<1> item) const {
    d_wo[item] = BinaryFieldOp()(f1_ro[item],f2_ro[item]);
  }
  
};

template<typename UnaryFieldOp>
void unary_field_operation(const typename UnaryFieldOp::SourceFieldType& s,
			   typename UnaryFieldOp::DestinationFieldType& d)
{
  size_t op_size = s.size();
  
  // Sanity check: field sizes must match.
  if (op_size != d.size()) {
    throw std::logic_error("Output field size must match input field "
			   "size in unary operator");
  }
  
  if (s.is_on_device()) {
    // Sanity check: Everything is on the compute device
    if (not d.is_on_device()) {
      throw std::logic_error("Input and output fields must be on the same "
			     "device in unary operator");
    }

    // Sanity check: SYCL queue objects must match
    auto& op_queue = s.queue();
    if (op_queue != d.queue()) {
      throw std::logic_error("Input and output fields must be in the same "
			     "context in unary operator");
    }
  
    op_queue.submit([&](sycl::handler& cgh)
    {
      auto op_kernel = UnaryFieldOperationKernel<UnaryFieldOp>(cgh, s, d);
      cgh.parallel_for(sycl::range<1>(op_size), op_kernel);
    });
    
  } else {
    // Sanity check: Everything is on the host
    if (d.is_on_device()) {
      throw std::logic_error("Input and output fields must be on the same "
			     "device in unary operator");
    }

    for (size_t i = 0; i < op_size; ++i) {
      d.host_vector().at(i) = UnaryFieldOp()(s.host_vector().at(i));
    }
  }
}

template<typename BinaryFieldOp>
void binary_field_operation(const typename BinaryFieldOp::Source1FieldType& f1,
			    const typename BinaryFieldOp::Source2FieldType& f2,
			    typename BinaryFieldOp::DestinationFieldType& dest)
{
  size_t op_size = f1.size();
  
  // Sanity check: field sizes must match.
  if (op_size != f2.size()) {
    throw std::logic_error("Input field sizes must match in binary operator");
  }
  if (op_size != dest.size()) {
    throw std::logic_error("Output field size must match input field "
			   "size in binary operator");
  }
  
  if (f1.is_on_device()) {
    // Sanity check: Everything is on the compute device
    if (not f2.is_on_device()) {
      throw std::logic_error("Input fields must be on the same "
			     "device in binary operator");
    }
    if (not dest.is_on_device()) {
      throw std::logic_error("Input and output fields must be on the same "
			     "device in binary operator");
    }

    // Sanity check: SYCL queue objects must match
    auto& op_queue = f1.queue();
    if (op_queue != f2.queue()) {
      throw std::logic_error("Input fields must be in the same "
			     "context in binary operator");
    }
    if (op_queue != dest.queue()) {
      throw std::logic_error("Input and output fields must be in the same "
			     "context in binary operator");
    }
  
    op_queue.submit([&](sycl::handler& cgh)
    {
      auto op_kernel = BinaryFieldOperationKernel<BinaryFieldOp>(cgh, f1, f2, dest);
      cgh.parallel_for(sycl::range<1>(op_size), op_kernel);
    });
    
  } else {
    // Sanity check: Everything is on the host
    if (f2.is_on_device()) {
      throw std::logic_error("Input fields must be on the same "
			     "device in binary operator");
    }
    if (dest.is_on_device()) {
      throw std::logic_error("Input and output fields must be on the same "
			     "device in binary operator");
    }

    for (size_t i = 0; i < op_size; ++i) {
      dest.host_vector().at(i) = BinaryFieldOp()(f1.host_vector().at(i),
						 f2.host_vector().at(i));
    }
  }
}

template<typename UnaryFieldOp>
typename UnaryFieldOp::DestinationFieldType
unary_field_operation(const std::string& name,
		      const typename UnaryFieldOp::SourceFieldType& s)
{
  typename UnaryFieldOp::DestinationFieldType dest(s.queue_ptr(), name,
						   s.mesh_definition(),
						   s.is_on_device());
  unary_field_operation<UnaryFieldOp>(s, dest);
  return dest;
}

template<typename BinaryFieldOp>
typename BinaryFieldOp::DestinationFieldType
binary_field_operation(const std::string& name,
		       const typename BinaryFieldOp::Source1FieldType& f1,
		       const typename BinaryFieldOp::Source2FieldType& f2)
{
  typename BinaryFieldOp::DestinationFieldType dest(f1.queue_ptr(), name,
						    f1.mesh_definition(),
						    f1.is_on_device());
  binary_field_operation<BinaryFieldOp>(f1, f2, dest);
  return dest;
}

template<typename S1Field,
	 typename D1Field>
D1Field field_cast(const std::string& name,
		   const S1Field& a)
{
  return unary_field_operation<UnaryFieldCastOperator<S1Field,D1Field>>(name, a);
}

template<typename S1Field,
	 typename D1Field>
void field_cast_to(const S1Field& a,
		   D1Field& c)
{
  unary_field_operation<UnaryFieldCastOperator<S1Field,D1Field>>(a, c);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
D1Field field_sum(const std::string& name,
		  const S1Field& a,
		  const S2Field& b)
{
  return binary_field_operation<BinaryFieldSumOperator<S1Field,S2Field,D1Field>>(name, a, b);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
void field_sum_to(const S1Field& a,
		  const S2Field& b,
		  D1Field& c)
{
  binary_field_operation<BinaryFieldSumOperator<S1Field,S2Field,D1Field>>(a, b, c);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
D1Field field_difference(const std::string& name,
			 const S1Field& a,
			 const S2Field& b)
{
  return binary_field_operation<BinaryFieldDifferenceOperator<S1Field,S2Field,D1Field>>(name, a, b);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
void field_difference_to(const S1Field& a,
			 const S2Field& b,
			 D1Field& c)
{
  binary_field_operation<BinaryFieldDifferenceOperator<S1Field,S2Field,D1Field>>(a, b, c);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
D1Field field_multiplication(const std::string& name,
			     const S1Field& a,
			     const S2Field& b)
{
  return binary_field_operation<BinaryFieldMultiplicationOperator<S1Field,S2Field,D1Field>>(name, a, b);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
void field_multiplication_to(const S1Field& a,
			     const S2Field& b,
			     D1Field& c)
{
  binary_field_operation<BinaryFieldMultiplicationOperator<S1Field,S2Field,D1Field>>(a, b, c);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
D1Field field_division(const std::string& name,
		       const S1Field& a,
		       const S2Field& b)
{
  return binary_field_operation<BinaryFieldDivisionOperator<S1Field,S2Field,D1Field>>(name, a, b);
}

template<typename S1Field,
	 typename S2Field,
	 typename D1Field>
void field_division_to(const S1Field& a,
		       const S2Field& b,
		       D1Field& c)
{
  binary_field_operation<BinaryFieldDivisionOperator<S1Field,S2Field,D1Field>>(a, b, c);
}

template<typename T,
	 typename MeshDefn>
using CellField = Field<T, MeshDefn, FieldMapping::Cell>;

template<typename T,
	 typename MeshDefn>
using FaceField = Field<T, MeshDefn, FieldMapping::Face>;

template<typename T,
	 typename MeshDefn>
using VertexField = Field<T, MeshDefn, FieldMapping::Vertex>;

#endif

