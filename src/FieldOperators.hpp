/***********************************************************************
 * FieldOperators.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldOperators_hpp
#define FieldOperators_hpp

#define MAKE_UNARY_OPERATOR(NAME,OP)					\
  template<typename SField,						\
	   typename DField>						\
  struct UnaryField##NAME##Operator					\
  {									\
  using SourceFieldType = SField;					\
  using DestinationFieldType = DField;					\
  using SourceMeshType = typename SField::MeshType;			\
  using DestinationMeshType = typename DField::MeshType;		\
  using S1 = typename SourceFieldType::ValueType;			\
  using D1 = typename DestinationFieldType::ValueType;			\
									\
  D1 operator()(const S1& a) const					\
  {									\
    if constexpr (not std::is_same<SourceMeshType,			\
		  DestinationMeshType>::value) {			\
      throw std::logic_error("Mismatched mesh types in unary field operator"); \
    } else if constexpr (SourceFieldType::FieldMappingType !=		\
			 DestinationFieldType::FieldMappingType) {	\
      throw std::logic_error("Mismatched mesh mappings in unary field operator"); \
    } else {								\
      return D1(OP(a));							\
    }									\
  }									\
  }

#define MAKE_BINARY_OPERATOR(NAME,OP)					\
  template<typename S1Field,						\
	   typename S2Field,						\
	   typename DField>						\
  struct BinaryField##NAME##Operator					\
  {									\
  using Source1FieldType = S1Field;					\
  using Source2FieldType = S2Field;					\
  using DestinationFieldType = DField;					\
  using Source1MeshType = typename S1Field::MeshType;			\
  using Source2MeshType = typename S2Field::MeshType;			\
  using DestinationMeshType = typename DField::MeshType;		\
  using S1 = typename Source1FieldType::ValueType;			\
  using S2 = typename Source2FieldType::ValueType;			\
  using D1 = typename DestinationFieldType::ValueType;			\
  									\
  D1 operator()(const S1& a, const S2& b) const				\
  {									\
    if constexpr ((not std::is_same<Source1MeshType,			\
		   DestinationMeshType>::value) or			\
		  (not std::is_same<Source1MeshType,			\
		   DestinationMeshType>::value)) {			\
      throw std::logic_error("Mismatched mesh types in binary field operator"); \
    } else if constexpr ((Source1FieldType::FieldMappingType !=		\
			  DestinationFieldType::FieldMappingType) or	\
			 (Source2FieldType::FieldMappingType !=		\
			  DestinationFieldType::FieldMappingType)) {	\
      throw std::logic_error("Mismatched mesh mappings in binary field operator"); \
    } else {								\
      return D1((a) OP (b));						\
    }									\
  }									\
  }

// #define MAKE_UNARY_OPERATOR(NAME,OP)			\
//   template<typename S1,					\
// 	   typename MeshDefn,				\
// 	   FieldMapping FM,				\
// 	   typename D1>					\
//   struct UnaryField##NAME##Operator			\
//   {							\
//     using SourceFieldType = Field<S1,MeshDefn,FM>;	\
//     using DestinationFieldType = Field<D1,MeshDefn,FM>;	\
//     							\
//     D1 operator()(const S1& a) const			\
//     {							\
//       return D1(OP(a));					\
//     }							\
//   };

// #define MAKE_BINARY_OPERATOR(NAME,OP)				\
//   template<typename S1,						\
// 	   typename S2,						\
// 	   typename MeshDefn,					\
// 	   FieldMapping FM,					\
// 	   typename D1>						\
//   struct BinaryField##NAME##Operator				\
//   {								\
//     using Source1FieldType = Field<S1,MeshDefn,FM>;		\
//     using Source2FieldType = Field<S2,MeshDefn,FM>;		\
//     using DestinationFieldType = Field<D1,MeshDefn,FM>;		\
//     								\
//     D1 operator()(const S1& a, const S2& b) const		\
//     {								\
//       return D1((a) OP (b));					\
//     }								\
//   }

MAKE_UNARY_OPERATOR(IsNaN,std::isnan);
MAKE_UNARY_OPERATOR(Cast,);

MAKE_BINARY_OPERATOR(Sum,+);
MAKE_BINARY_OPERATOR(Difference,-);
MAKE_BINARY_OPERATOR(Multiplication,*);
MAKE_BINARY_OPERATOR(Division,/);

#undef MAKE_UNARY_OPERATOR
#undef MAKE_BINARY_OPERATOR

#endif
