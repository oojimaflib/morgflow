/***********************************************************************
 * FieldVector.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldVector_hpp
#define FieldVector_hpp

#include "Field.hpp"

template<typename T, typename MeshDefn, FieldMapping FM, size_t N>
class FieldVector : public std::vector< Field<T, MeshDefn, FM> >
{
public:

  using ValueType = T;
  using MeshType = MeshDefn;
  using FieldType = Field<T,MeshDefn,FM>;
  static const FieldMapping FieldMappingType = FieldType::FieldMappingType;
  
private:

  FieldVector(void)
    : std::vector<FieldType>()
  {}

public:

  FieldVector(const std::shared_ptr<sycl::queue>& queue,
	      const std::array<std::string,N>& names,
	      const std::shared_ptr<MeshDefn>& meshdefn_p,
	      bool on_device,
	      const T& init_value = T())
    : std::vector<FieldType>()
  {
    for (size_t i = 0; i < N; ++i) {
      this->template emplace_back(queue, names.at(i),
				  meshdefn_p, on_device, init_value);
    }
  }
  
  FieldVector(const std::initializer_list<FieldType>& il)
    : std::vector<FieldType>(il)
  {
    //    assert(std::vector<CellField<T,MeshDefn>>::size() == N);
  }

  FieldVector(const std::string& prefix,
	      const FieldVector<T,MeshDefn,FM,N>& cfv,
	      const std::string& suffix)
    : std::vector<FieldType>()
  {
    for (size_t i = 0; i < N; ++i) {
      this->template emplace_back(prefix, cfv.at(i), suffix);
    }
  }

  /*
  FieldVector<T,MeshDefn,FM,N> copy(const std::string& prefix = "",
				    const std::string& suffix = "")
  {
    FieldVector<T,MeshDefn,FM,N> new_cfv;
    for (size_t i = 0; i < N; ++i) {
      new_cfv.push_back(this->at(i).copy(prefix, suffix));
    }
    return new_cfv;
  }
  */

  void move_to_device(void)
  {
    for (auto&& cf : *this) {
      cf.move_to_device();
    }
  }
  
  void move_to_host(void)
  {
    for (auto&& cf : *this) {
      cf.move_to_host();
    }
  }

  std::shared_ptr<MeshType> mesh_definition(void) const
  {
    return this->at(0).mesh_definition();
  }

  using AccessMode = sycl::access::mode;
  using AccessTarget = sycl::access::target;
  using AccessPlaceholder = sycl::access::placeholder;

  template<AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer,
	   AccessPlaceholder IsPlaceholder = AccessPlaceholder::false_t>
  using FieldAccessorType = typename FieldType::template Accessor<Mode, Target, IsPlaceholder>;

  template<AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer,
	   AccessPlaceholder IsPlaceholder = AccessPlaceholder::false_t>
  using Accessor = std::array<FieldAccessorType<Mode, Target, IsPlaceholder>, N>;

  template<AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer>
  Accessor<Mode, Target> get_accessor(sycl::handler& cgh) const
  {
    Accessor<Mode, Target> acc;
    for (size_t i = 0; i < N; ++i) {
      acc[i] = this->at(i).template get_accessor<Mode,Target>(cgh);
    }
    return acc;
  }

  Accessor<sycl::access::mode::read>
  get_read_accessor(sycl::handler& cgh) const
  {
    return get_accessor<sycl::access::mode::read>(cgh);
  }
  
  Accessor<sycl::access::mode::write>
  get_write_accessor(sycl::handler& cgh) const
  {
    return get_accessor<sycl::access::mode::write>(cgh);
  }
  
  Accessor<sycl::access::mode::read_write>
  get_read_write_accessor(sycl::handler& cgh) const
  {
    return get_accessor<sycl::access::mode::read_write>(cgh);
  }
  
  template<size_t Count,
	   AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer,
	   AccessPlaceholder IsPlaceholder = AccessPlaceholder::false_t>
  using SliceAccessor = std::array<FieldAccessorType<Mode, Target, IsPlaceholder>, Count>;

  template<size_t From, size_t Count,
	   AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer>
  SliceAccessor<Count, Mode, Target> get_slice_accessor(sycl::handler& cgh) const
  {
    SliceAccessor<Count, Mode, Target> sacc;
    for (size_t i = 0; i < Count; ++i) {
      FieldType& cf = this->at(From + i);
      // FieldAccessorType<Mode, Target>& cfa = 
      sacc[i] = cf.template get_accessor<Mode, Target>(cgh);
    }
    return sacc;
  }

  sycl::range<1> get_range(void) const
  {
    return sycl::range<1>(this->at(0).size());
  }
  
};

template<typename T,
	 typename MeshDefn,
	 size_t N>
using CellFieldVector = FieldVector<T, MeshDefn, FieldMapping::Cell, N>;

template<typename T,
	 typename MeshDefn,
	 size_t N>
using FaceFieldVector = FieldVector<T, MeshDefn, FieldMapping::Face, N>;

template<typename T,
	 typename MeshDefn,
	 size_t N>
using VertexFieldVector = FieldVector<T, MeshDefn, FieldMapping::Vertex, N>;


#endif
