/***********************************************************************
 * TemporalDerivatives/SV/Kernels/Cartesian2DMeshCellKernel.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef TemporalDerivatives_SV_Kernels_Cartesian2DMeshCellKernel_hpp
#define TemporalDerivatives_SV_Kernels_Cartesian2DMeshCellKernel_hpp

template<typename T>
class SVCartesian2DMeshCellTemporalDerivativeKernel
{
protected:

  using ValueType = T;
  using MeshType = Cartesian2DMesh;

  template<size_t N>
  using CellFieldVector = FieldVector<T,MeshType,FieldMapping::Cell,N>;

  template<size_t N>
  using FaceFieldVector = FieldVector<T,MeshType,FieldMapping::Face,N>;

  MeshType mesh_;

  template<size_t N>
  using ReadAccessor =
    typename CellFieldVector<N>::template Accessor<sycl::access::mode::read>;

  template<size_t N>
  using ReadFluxAccessor =
    typename FaceFieldVector<N>::template Accessor<sycl::access::mode::read>;

  template<size_t N>
  using WriteAccessor =
    typename CellFieldVector<N>::template Accessor<sycl::access::mode::write>;

  ReadAccessor<3> U_ro_;
  ReadAccessor<3> zb_ro_;
  ReadAccessor<4> n_ro_;
  ReadAccessor<2> Q_in_ro_;
  ReadAccessor<2> h_in_ro_;
  ReadFluxAccessor<4> F_ro_;
  WriteAccessor<3> dUdt_wo_;

  float time_now_;
  float timestep_;
  float bdy_t0_;
  float bdy_t1_;

public:

  SVCartesian2DMeshCellTemporalDerivativeKernel(sycl::handler& cgh,
						const CellFieldVector<3>& U,
						const CellFieldVector<3>& zb,
						const CellFieldVector<4>& n,
						const CellFieldVector<2>& Q_in,
						const CellFieldVector<2>& h_in,
						const FaceFieldVector<4>& flux,
						CellFieldVector<3>& dUdt,
						const double& time_now,
						const double& timestep,
						const double& bdy_t0,
						const double& bdy_t1)
    : mesh_(*(U.mesh_definition())),
      U_ro_(U.get_read_accessor(cgh)),
      zb_ro_(zb.get_read_accessor(cgh)),
      n_ro_(n.get_read_accessor(cgh)),
      Q_in_ro_(Q_in.get_read_accessor(cgh)),
      h_in_ro_(h_in.get_read_accessor(cgh)),
      F_ro_(flux.get_read_accessor(cgh)),
      dUdt_wo_(dUdt.get_write_accessor(cgh)),
      time_now_(time_now),
      timestep_(timestep),
      bdy_t0_(bdy_t0), bdy_t1_(bdy_t1)
  {}

  void operator()(sycl::item<1> item) const
  {
    size_t cell_c = item.get_linear_id();

    // Get the IDs of the surrounding faces
    auto cell_index = mesh_.get_cell_index(cell_c);
    auto face_list = mesh_.get_faces_around_cell(cell_index);
    size_t fid_W = face_list[0];
    size_t fid_E = face_list[1];
    size_t fid_S = face_list[2];
    size_t fid_N = face_list[3];

    // Get cell size
    auto cell_size = mesh_.cell_size();
    float dx = cell_size[0];
    float dy = cell_size[1];

    // Calculate the raw changes in variable from the cell face fluxes
    float dhdt = (F_ro_[0][fid_W] - F_ro_[0][fid_E]) / dx
      + (F_ro_[0][fid_S] - F_ro_[0][fid_N]) / dy;
    float dudt = (F_ro_[1][fid_W] - F_ro_[1][fid_E]) / dx
      + (F_ro_[1][fid_S] - F_ro_[1][fid_N]) / dy;
    float dvdt = (F_ro_[2][fid_W] - F_ro_[2][fid_E]) / dx
      + (F_ro_[2][fid_S] - F_ro_[2][fid_N]) / dy;

    // Get the cell bed-slopes (pre-calculated) and apply gravity
    // forces as a source term. The magnitude of the horizontal force
    // due to the bed slope is limited to gh.
    float dzdx = zb_ro_[1][cell_c];
    if (sycl::fabs(dzdx) > U_ro_[0][cell_c] / dx) {
      dzdx = sycl::sign(dzdx) * U_ro_[0][cell_c] / dx;
    }
    float dzdy = zb_ro_[2][cell_c];
    if (sycl::fabs(dzdy) > U_ro_[0][cell_c] / dy) {
      dzdy = sycl::sign(dzdy) * U_ro_[0][cell_c] / dy;
    }
    float dudt_bed = -9.81f * dzdx;
    float dvdt_bed = -9.81f * dzdy;

    // Calculate the forces on the water in the cell due to vertical
    // walls at the cell faces and apply as a source term. The
    // magnitude of the force is limited by the cell water depth (so
    // only the portion of the wall that is wet affects the water)
    if (F_ro_[3][fid_W] < 0.0f)
      dudt_bed += -9.81f * sycl::fmax(F_ro_[3][fid_W], -U_ro_[0][cell_c]) / dx;
    if (F_ro_[3][fid_E] > 0.0f)
      dudt_bed += -9.81f * sycl::fmin(F_ro_[3][fid_E], U_ro_[0][cell_c]) / dx;
    if (F_ro_[3][fid_S] < 0.0f)
      dvdt_bed += -9.81f * sycl::fmax(F_ro_[3][fid_S], -U_ro_[0][cell_c]) / dy;
    if (F_ro_[3][fid_N] > 0.0f)
      dvdt_bed += -9.81f * sycl::fmin(F_ro_[3][fid_N], U_ro_[0][cell_c]) / dy;
    dudt += dudt_bed;
    dvdt += dvdt_bed;

    // Calculate volume inflows from flow boundaries...
    float dhdt_source = 0.0;
    {
      float Q_0 = Q_in_ro_[0][cell_c];
      float Q_1 = Q_in_ro_[1][cell_c];
      float dQ_dt = (Q_1 - Q_0) / (bdy_t1_ - bdy_t0_);
      float Q_now = Q_0 + (time_now_ - bdy_t0_) * dQ_dt;
      float Q_next = Q_now + timestep_ * dQ_dt;
      dhdt_source = 0.5f * (Q_now + Q_next) / (dx * dy);
      //      std::cout << timestep_ << "\t" << Q_0 << "\t"
      //	<< Q_1 << "\t" << dhdt_source << std::endl;
    }
    // ...and from water level boundaries
    float h_boundary = 0.0;
    {
      float h_0 = h_in_ro_[0][cell_c];
      if (h_0 < 0.0f) {
	h_boundary = -1.0f;
      } else {
	float h_1 = h_in_ro_[1][cell_c];
	float dh_dt = (h_1 - h_0) / (bdy_t1_ - bdy_t0_);
	float h_now = h_0 + (time_now_ - bdy_t0_) * dh_dt;
	float h_next = h_now + timestep_ * dh_dt;
	h_boundary = 0.5f * (h_now + h_next);
      }
    }

    // If there is a water level boundary here, over-ride the
    // calculated change in water level with one that will satisfy
    // that boundary
    if (h_boundary >= 0.0f) {
      dhdt = (h_boundary - U_ro_[0][cell_c]);
    } else {
      // Otherwise, add in any contributions from flow boundaries
      dhdt += dhdt_source;
      //std::cout << "dhdt = " << dhdt << std::endl;
    }

    // Calculate the Manning's n value for the cell...
    float manning_n = sycl::mix(n_ro_[0][cell_c], n_ro_[2][cell_c],
				sycl::smoothstep(n_ro_[1][cell_c],
						 n_ro_[3][cell_c],
						 U_ro_[0][cell_c]));
    // ...and hence the friction slope terms:
    float sf = 0.0f;
    if (U_ro_[0][cell_c] > 1e-6) {
      float inv_h = U_ro_[0][cell_c]
	/ (U_ro_[0][cell_c] * U_ro_[0][cell_c] + 1e-3);
      sf = manning_n * manning_n
	* sycl::sqrt(U_ro_[1][cell_c] * U_ro_[1][cell_c] +
		     U_ro_[2][cell_c] * U_ro_[2][cell_c])
	* sycl::pow(inv_h, 1.333333f);
    }

    // Apply the friction slopes to the du/dt and dv/dt terms, but
    // prevent the friction force being so strong as to push the water
    // backwards
    float u_estimate = U_ro_[1][cell_c] + dudt * 0.5f * timestep_;
    float dudt_f = 9.81f * sf * U_ro_[1][cell_c];
    if ((sycl::sign(dudt_f) == sycl::sign(u_estimate)) and
	(sycl::fabs(dudt_f) > sycl::fabs(u_estimate))) {
      dudt_f = u_estimate;
    } else if (sycl::sign(dudt_f) == sycl::sign(u_estimate)) {
      dudt_f = 0.0f;
    }
    dudt -= dudt_f;

    float v_estimate = U_ro_[2][cell_c] + dvdt * 0.5f * timestep_;
    float dvdt_f = 9.81f * sf * U_ro_[2][cell_c];
    if ((sycl::sign(dvdt_f) == sycl::sign(v_estimate)) and
	(sycl::fabs(dvdt_f) > sycl::fabs(v_estimate))) {
      dvdt_f = v_estimate;
    } else if (sycl::sign(dvdt_f) == sycl::sign(v_estimate)) {
      dvdt_f = 0.0f;
    }
    dvdt -= dvdt_f;

    dUdt_wo_[0][cell_c] = dhdt;
    dUdt_wo_[1][cell_c] = dudt;
    dUdt_wo_[2][cell_c] = dvdt;
  }
  
};

#endif
