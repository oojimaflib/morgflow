/***********************************************************************
 * SpatialDerivatives/Minmod/Kernels/Cartesian2DMeshCell2CellKernel.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef SpatialDerivatives_Minmod_Cartesian2DMeshCell2CellKernel_hpp
#define SpatialDerivatives_Minmod_Cartesian2DMeshCell2CellKernel_hpp

template<typename T,
	 size_t N>
class MinmodCartesian2DMeshCell2CellSpatialDerivativeKernel
{
protected:

  using ValueType = T;
  using MeshType = Cartesian2DMesh;
  using FV = FieldVector<T,MeshType,FieldMapping::Cell,N>;
  
  MeshType mesh_;
  
  using ReadAccessor =
    typename FV::template Accessor<sycl::access::mode::read>;
  
  using WriteAccessor =
    typename FV::template Accessor<sycl::access::mode::write>;
  
  ReadAccessor U_ro_;
  
  WriteAccessor dUdx_wo_;
  WriteAccessor dUdy_wo_;

  ValueType theta_;
  
  ValueType minmod3(ValueType a,
		    ValueType b,
		    ValueType c) const
  {
    ValueType mm =
      0.5 * (sycl::sign(a) + sycl::sign(b)) *
      sycl::min(sycl::fabs(a), sycl::fabs(b));
    return 0.5 * (sycl::sign(mm) + sycl::sign(c)) *
      sycl::min(sycl::fabs(mm), sycl::fabs(c));
  }
  
public:
  
  MinmodCartesian2DMeshCell2CellSpatialDerivativeKernel(sycl::handler& cgh,
							const FV& U,
							FV& dUdx,
							FV& dUdy,
							const ValueType& theta)
    : mesh_(*(U.mesh_definition())),
      U_ro_(U.get_read_accessor(cgh)),
      dUdx_wo_(dUdx.get_write_accessor(cgh)),
      dUdy_wo_(dUdy.get_write_accessor(cgh)),
      theta_(theta)
  {
  }
  
  void operator()(sycl::item<1> item) const {
    size_t cid_c = item.get_linear_id();
    typename MeshType::IndexType cidx_c = mesh_.get_cell_index(cid_c);

    uint8_t cell_edge = 0;
    size_t cid_w;
    if (cidx_c[0] > 0) {
      cid_w = mesh_.get_cell_linear_id({cidx_c[0] - 1, cidx_c[1]});
    } else {
      cid_w = cid_c;
      cell_edge += 8;
    }
    size_t cid_e;
    if (cidx_c[0] < mesh_.get_cell_index_size()[0] - 1) {
      cid_e = mesh_.get_cell_linear_id({cidx_c[0] + 1, cidx_c[1]});
    } else {
      cid_e = cid_c;
      cell_edge += 4;
    }
    size_t cid_s;
    if (cidx_c[1] > 0) {
      cid_s = mesh_.get_cell_linear_id({cidx_c[0], cidx_c[1] - 1});
    } else {
      cid_s = cid_c;
      cell_edge += 2;
    }
    size_t cid_n;
    if (cidx_c[1] < mesh_.get_cell_index_size()[1] - 1) {
      cid_n = mesh_.get_cell_linear_id({cidx_c[0], cidx_c[1] + 1});
    } else {
      cid_n = cid_c;
      cell_edge += 1;
    }

    for (size_t i = 0; i < U_ro_.size(); ++i) {
      auto& U = U_ro_[i];

      ValueType Uc = U[cid_c];
      ValueType Uw = U[cid_w];
      ValueType Ue = U[cid_e];
      ValueType Us = U[cid_s];
      ValueType Un = U[cid_n];

      if (Uw != Uw) Uw = Uc;
      if (Ue != Ue) Ue = Uc;
      if (Us != Us) Us = Uc;
      if (Un != Un) Un = Uc;

      typename MeshType::CoordType cell_size = mesh_.cell_size();
      dUdx_wo_[i][cid_c] = minmod3(theta_ * (Uc - Uw) / cell_size[0],
				   theta_ * (Ue - Uc) / cell_size[0],
				   0.5f * (Ue - Uw) / cell_size[0]);
      dUdy_wo_[i][cid_c] = minmod3(theta_ * (Uc - Us) / cell_size[1],
				   theta_ * (Un - Uc) / cell_size[1],
				   0.5f * (Un - Us) / cell_size[1]);
    }
  }

};

#endif
