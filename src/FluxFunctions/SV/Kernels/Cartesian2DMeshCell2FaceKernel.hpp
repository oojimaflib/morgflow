/***********************************************************************
 * FluxFunctions/SV/Kernels/Cartesian2DMeshCell2FaceKernel.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FluxFunctions_SV_Kernels_Cartesian2DMeshCell2FaceKernel_hpp
#define FluxFunctions_SV_Kernels_Cartesian2DMeshCell2FaceKernel_hpp

template<typename T>
class SVCartesian2DMeshCell2FaceFluxFunctionKernel
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
  using WriteAccessor =
    typename FaceFieldVector<N>::template Accessor<sycl::access::mode::write>;

  ReadAccessor<3> U_ro_;
  ReadAccessor<3> zb_ro_;
  ReadAccessor<4> n_ro_;
  ReadAccessor<3> dUdx_ro_;
  ReadAccessor<3> dUdy_ro_;
  WriteAccessor<4> F_wo_;

public:

  SVCartesian2DMeshCell2FaceFluxFunctionKernel(sycl::handler& cgh,
					       const CellFieldVector<3>& U,
					       const CellFieldVector<3>& zb,
					       const CellFieldVector<4>& n,
					       const CellFieldVector<3>& dUdx,
					       const CellFieldVector<3>& dUdy,
					       FaceFieldVector<4>& F)
    : mesh_(*(U.mesh_definition())),
      U_ro_(U.get_read_accessor(cgh)),
      zb_ro_(zb.get_read_accessor(cgh)),
      n_ro_(n.get_read_accessor(cgh)),
      dUdx_ro_(dUdx.get_read_accessor(cgh)),
      dUdy_ro_(dUdy.get_read_accessor(cgh)),
      F_wo_(F.get_write_accessor(cgh))
  {}

  void operator()(sycl::item<1> item) const
  {
    size_t fid = item.get_linear_id();

    // Get basic mesh data
    auto ncells = mesh_.get_cell_index_size();
    size_t nxcells = ncells[0];
    size_t nycells = ncells[1];
    auto cell_size = mesh_.cell_size();
    ValueType dx = cell_size[0];
    ValueType dy = cell_size[1];
    
    // Get the IDs of the adjacent cells and store if we are on a mesh
    // edge.
    auto adjacent_cells = mesh_.get_cells_around_face(fid);
    size_t lhs_id, rhs_id;
    int edge = 0; // Non-zero if this face is on the edge of the
		  // mesh. -1 if the LHS cell is "fake", 1 if the RHS
		  // cell is fake
    
    if (adjacent_cells[0] < mesh_.cell_count()) {
      lhs_id = adjacent_cells[0];
      if (adjacent_cells[1] < mesh_.cell_count()) {
	rhs_id = adjacent_cells[1];
      } else {
	rhs_id = lhs_id;
	edge = 1;
      }
    } else {
      rhs_id = adjacent_cells[1];
      lhs_id = rhs_id;
      edge = -1;
    }

    // Is this face carrying flow in the x- or y-direction?  These are
    // stored as ints so they can be effectively used as factors in
    // the equations below
    int xdir = (fid < (nxcells + 1) * nycells ? 1 : 0);
    int ydir = 1 - xdir;

    
    // Get the cell bed levels either side of the face
    ValueType zb_L = zb_ro_[0][lhs_id];// + (edge < 0 ? h_R + 10.0f : 0.0f);
    ValueType zb_R = zb_ro_[0][rhs_id];// + (edge > 0 ? h_L + 10.0f : 0.0f);

    // Check cell bed levels for NaN--meaning the cell is excluded
    // from calculation. If we find one, use the same set-up as a mesh
    // edge. If we find two, just return zero flux.
    if (zb_L != zb_L) {
      lhs_id = rhs_id;
      // zb_L = zb_R;
      edge = -1;

      if (zb_R != zb_R) {
	F_wo_[0][fid] = 0.0f;
	F_wo_[1][fid] = 0.0f;
	F_wo_[2][fid] = 0.0f;
	F_wo_[3][fid] = 0.0f;
	return;
      }
    } else if (zb_R != zb_R) {
      rhs_id = lhs_id;
      // zb_R = zb_L;
      edge = 1;
    }

    // Get the data for each cell:

    // Water depth: zero if the cell is fake
    ValueType h_L = U_ro_[0][lhs_id] * (edge < 0 ? 0 : 1);
    ValueType h_R = U_ro_[0][rhs_id] * (edge > 0 ? 0 : 1);

    // x-velocities: zero if the face flows horizontally and the
    // cell is fake
    ValueType u_L = U_ro_[1][lhs_id] * (edge < 0 && xdir == 1 ? 0 : 1);
    ValueType u_R = U_ro_[1][rhs_id] * (edge > 0 && xdir == 1 ? 0 : 1);

    // y-velocities: zero if the face flows vertically and the cell
    // is fake
    ValueType v_L = U_ro_[2][lhs_id] * (edge < 0 && ydir == 1 ? 0 : 1);
    ValueType v_R = U_ro_[2][rhs_id] * (edge > 0 && ydir == 1 ? 0 : 1);

    // Slopes of water depth: zero if the cell is fake.
    ValueType dhdx_L = dUdx_ro_[0][lhs_id] * (edge < 0 ? 0 : 1);
    ValueType dhdx_R = dUdx_ro_[0][rhs_id] * (edge > 0 ? 0 : 1);
    ValueType dhdy_L = dUdy_ro_[0][lhs_id] * (edge < 0 ? 0 : 1);
    ValueType dhdy_R = dUdy_ro_[0][rhs_id] * (edge > 0 ? 0 : 1);

    // Slopes of x-velocity: zero if the face is flowing horizontally
    // and either cell is fake
    ValueType dudx_L = dUdx_ro_[1][lhs_id] * (edge < 0 && xdir == 1 ? 0 : 1);
    ValueType dudx_R = dUdx_ro_[1][rhs_id] * (edge > 0 && xdir == 1 ? 0 : 1);
    ValueType dudy_L = dUdy_ro_[1][lhs_id] * (edge < 0 && xdir == 1 ? 0 : 1);
    ValueType dudy_R = dUdy_ro_[1][rhs_id] * (edge > 0 && xdir == 1 ? 0 : 1);

    // Slopes of y-velocity: zero if the face is flowing vertically
    // and either cell is fake
    ValueType dvdx_L = dUdx_ro_[2][lhs_id] * (edge < 0 && ydir == 1 ? 0 : 1);
    ValueType dvdx_R = dUdx_ro_[2][rhs_id] * (edge > 0 && ydir == 1 ? 0 : 1);
    ValueType dvdy_L = dUdy_ro_[2][lhs_id] * (edge < 0 && ydir == 1 ? 0 : 1);
    ValueType dvdy_R = dUdy_ro_[2][rhs_id] * (edge > 0 && ydir == 1 ? 0 : 1);

    // Slopes of bed level: zero if the cell is fake
    ValueType dzdx_L = zb_ro_[1][lhs_id] * (edge < 0 ? 0 : 1);
    ValueType dzdx_R = zb_ro_[1][rhs_id] * (edge > 0 ? 0 : 1);
    ValueType dzdy_L = zb_ro_[2][lhs_id] * (edge < 0 ? 0 : 1);
    ValueType dzdy_R = zb_ro_[2][rhs_id] * (edge > 0 ? 0 : 1);

    // If one of the cells is fake, set its bed level above the
    // water level of the other cell
    if (edge < 0) {
      zb_L = zb_R + h_R * 1.1f;
      /*
      if (xdir == 1) {
	dzdx_L = 0.0f;
      } else {
	dzdy_L = 0.0f;
      }
      */
    }
    if (edge > 0) {
      zb_R = zb_L + h_L * 1.1f;
      /*
      if (xdir == 1) {
	dzdx_R = 0.0f;
      } else {
	dzdy_R = 0.0f;
      }
      */
    }

    /*
    if (fid == 48) {
      std::cout << xdir << "/" << ydir << "\tLR\t"
		<< h_L << "/" << h_R << "\t"
		<< dhdx_L << "/" << dhdx_R << "\t"
		<< u_L << "/" << u_R << "\t"
		<< v_L << "/" << v_R << "\t"
		<< zb_L << "/" << zb_R << std::endl;
    }
    */
    
    // Project central estimates of bed level from each cell to the
    // upstream (m,-) and downstream (p,+) sides of each face
    ValueType z_m = zb_L +
      0.5f * dx * dzdx_L * xdir +
      0.5f * dy * dzdy_L * ydir;
    ValueType z_p = zb_R +
      -0.5f * dx * dzdx_R * xdir +
      -0.5f * dy * dzdy_R * ydir;

    ValueType h_m = h_L +
      0.5f * dx * dhdx_L * xdir +
      0.5f * dy * dhdy_L * ydir;
    ValueType h_p = h_R +
      -0.5f * dx * dhdx_R * xdir +
      -0.5f * dy * dhdy_R * ydir;

    ValueType u_m = u_L +
      0.5f * dx * dudx_L * xdir +
      0.5f * dy * dudy_L * ydir;
    ValueType u_p = u_R +
      -0.5f * dx * dudx_R * xdir +
      -0.5f * dy * dudy_R * ydir;
    
    ValueType v_m = v_L +
      0.5f * dx * dvdx_L * xdir +
      0.5f * dy * dvdy_L * ydir;
    ValueType v_p = v_R +
      -0.5f * dx * dvdx_R * xdir +
      -0.5f * dy * dvdy_R * ydir;

    ValueType z_f = sycl::fmax(z_m, z_p);// + 1e-4f;

    // Limit the depths at the face.
    h_m = sycl::fmax(h_m, 0.0f);
    h_p = sycl::fmax(h_p, 0.0f);

    // Calculate water levels at the face
    ValueType y_m = z_m + h_m;
    ValueType y_p = z_p + h_p;

    // Calculate the wave speed, c = √(gh)
    ValueType c_m = sycl::sqrt(9.81f * h_m);
    ValueType c_p = sycl::sqrt(9.81f * h_p);

    // Calculate the face fluxes
    ValueType Hh, Hu, Hv;

    /*
    if (fid == 48) {
      std::cout << xdir << "/" << ydir << "\t"
		<< h_m << "/" << h_p << "\t"
		<< u_m << "/" << u_p << "\t"
		<< v_m << "/" << v_p << "\t"
		<< z_m << "/" << z_p << std::endl;
    }
    */
    
    size_t b = 0;
    if (y_m > z_f or y_p > z_f) {
      b=1;
      // Fully submerged case
      ValueType spd_p = u_p * xdir + v_p * ydir;
      ValueType spd_m = u_m * xdir + v_m * ydir;

      ValueType Fh_m = h_m * spd_m;
      ValueType Fh_p = h_p * spd_p;
      ValueType Fu_m = u_m * ((1.0f - 0.5f * xdir) * spd_m) + 9.81f * h_m * xdir;
      ValueType Fu_p = u_p * ((1.0f - 0.5f * xdir) * spd_p) + 9.81f * h_p * xdir;
      ValueType Fv_m = v_m * ((1.0f - 0.5f * ydir) * spd_m) + 9.81f * h_m * ydir;
      ValueType Fv_p = v_p * ((1.0f - 0.5f * ydir) * spd_p) + 9.81f * h_p * ydir;

      ValueType a = sycl::fmax(sycl::fabs(spd_p + sycl::sign(spd_p) * c_p),
			       sycl::fabs(spd_m + sycl::sign(spd_m) * c_m));
      
      Hh = 0.5f * (Fh_p + Fh_m) - 0.5f * a * (h_p - h_m);
      Hu = 0.5f * (Fu_p + Fu_m) - 0.5f * a * (u_p - u_m);
      Hv = 0.5f * (Fv_p + Fv_m) - 0.5f * a * (v_p - v_m);
    } else if (h_m <= 0.0f and h_p <= 0.0f) {
      b=2;
      // Fully dry case
      Hh = Hu = Hv = 0.0f;
    } else {
      // Partially submerged step
      if (z_m > z_p) {
	b=3;
	ValueType spd = u_m * xdir + v_m * ydir;

	ValueType Fh = h_m * spd;
	ValueType Fu = u_m * (0.5f * spd) + 9.81f * h_m * xdir;
	ValueType Fv = v_m * (0.5f * spd) + 9.81f * h_m * ydir;

	ValueType a = sycl::fabs(spd + sycl::sign(spd) * c_m);
	
	Hh = Fh - 0.5f * a * -h_m;
	Hu = Fu - 0.5f * a * -u_m;
	Hv = Fv - 0.5f * a * -v_m;
      } else {
	b=4;
	ValueType spd = u_p * xdir + v_p * ydir;

	ValueType Fh = h_p * spd;
	ValueType Fu = u_p * (0.5f * spd) + 9.81f * h_p * xdir;
	ValueType Fv = v_p * (0.5f * spd) + 9.81f * h_p * ydir;

	ValueType a = sycl::fabs(spd + sycl::sign(spd) * c_p);
	
	Hh = Fh - 0.5f * a * h_p;
	Hu = Fu - 0.5f * a * u_p;
	Hv = Fv - 0.5f * a * v_p;
      }
    }

    ValueType dz_f = z_p - z_m;

    /*
    if (fid == 48) {
      std::cout << edge << ": "
		<< Hh << " "
		<< Hu << " "
		<< Hv << " "
		<< dz_f << std::endl;
    }
    */
    
    F_wo_[0][fid] = Hh;
    F_wo_[1][fid] = Hu;
    F_wo_[2][fid] = Hv;
    F_wo_[3][fid] = dz_f;
  }
};

#endif
