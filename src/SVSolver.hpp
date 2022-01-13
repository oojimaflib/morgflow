/***********************************************************************
 * SVSolver.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef SVSolver_hpp
#define SVSolver_hpp

#include "FieldVector.hpp"
#include "FieldGenerator.hpp"

#include "OutputFormat.hpp"
#include "OutputFormats/CSVOutputFormat.hpp"

#include "Meshes/Cartesian2DMesh.hpp"

#include "SpatialDerivatives/MinmodSpatialDerivative.hpp"
#include "FluxFunctions/SVFluxFunction.hpp"
#include "TemporalDerivatives/SVTemporalDerivative.hpp"
#include "ControlNumbers/SVControlNumber.hpp"

class SVSolver
{
public:

  using ValueType = float;
  using MeshType = Cartesian2DMesh;
  using SolutionState = CellFieldVector<ValueType, MeshType, 3>;
  using SpatialDerivativeType = SpatialDerivative<ValueType,
						  MeshType,
						  FieldMapping::Cell,
						  FieldMapping::Cell,3>;
  using FluxFunctionType = FluxFunction<ValueType,
					MeshType,
					FieldMapping::Cell,
					FieldMapping::Face,
					3, 4>;
  using TemporalDerivativeType = TemporalDerivative<ValueType,
						    MeshType,
						    FieldMapping::Cell,3>;

  static const FieldMapping BCFieldMappingType = FieldMapping::Cell;

  using ValueField = Field<ValueType,MeshType,FieldMapping::Cell>;
  
private:

  std::shared_ptr<sycl::queue> queue_;
  std::shared_ptr<MeshType> mesh_;
  std::shared_ptr<SpatialDerivativeType> spatial_derivative_;
  std::shared_ptr<FluxFunctionType> flux_function_;
  std::shared_ptr<TemporalDerivativeType> temporal_derivative_;

  // Constants
  CellFieldVector<ValueType, MeshType, 3> zbed_;
  CellFieldVector<ValueType, MeshType, 4> manning_n_;

  // Temporaries
  CellFieldVector<ValueType, MeshType, 3> dUdx_;
  CellFieldVector<ValueType, MeshType, 3> dUdy_;
  FaceFieldVector<ValueType, MeshType, 4> flux_;
  
  // Boundary Conditions
  FieldVector<ValueType, MeshType, BCFieldMappingType, 2> Q_in_;
  FieldVector<ValueType, MeshType, BCFieldMappingType, 2> h_in_;

public:

  SVSolver(std::shared_ptr<sycl::queue>& queue)
    : queue_(queue),
      mesh_(std::make_shared<MeshType>(GlobalConfig::instance().configuration().get_child("mesh"))),
      spatial_derivative_(std::make_shared<MinmodSpatialDerivative<ValueType,MeshType,FieldMapping::Cell,FieldMapping::Cell,3>>()),
      flux_function_(std::make_shared<SVFluxFunction<ValueType,MeshType,FieldMapping::Cell,FieldMapping::Face,3,4>>()),
      temporal_derivative_(std::make_shared<SVTemporalDerivative<ValueType,MeshType,FieldMapping::Cell,3>>()),
      zbed_(queue, { "zb", "dzb⁄dx", "dzb⁄dy" }, mesh_, true, 0.0f),
      manning_n_(queue, {"manning_n0", "manning_h0",
			 "manning_n1", "manning_h1"}, mesh_, true, 0.0f),
      /*
      zbed_({
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue, "zb",
							      mesh_, 0.0f),
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue, "dzb⁄dx",
							      mesh_, 0.0f),
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue, "dzb⁄dy",
							      mesh_, 0.0f)
      }),
      manning_n_({
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue,
							      "manning_n0",
							      mesh_, 0.03f),
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue,
							      "manning_h0",
							      mesh_, 0.05f),
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue,
							      "manning_n1",
							      mesh_, 0.03f),
	generate_field<ValueType,MeshType,FieldMapping::Cell>(queue,
							      "manning_h1",
							      mesh_, 0.1f)
      }),
      */
      dUdx_(queue, { "dh⁄dx", "du⁄dx", "dv⁄dx" }, mesh_, true, 0.0f),
      dUdy_(queue, { "dh⁄dy", "du⁄dy", "dv⁄dy" }, mesh_, true, 0.0f),
      flux_(queue, { "mass", "xmom", "ymom", "wall" }, mesh_, true, 0.0f),
      Q_in_(queue, { "Q_in_0", "Q_in_1" }, mesh_, true, 0.0f),
      h_in_(queue, { "h_in_0", "h_in_1" }, mesh_, true, -1.0f)
      /*
      dUdx_({
	CellField<ValueType, MeshType>(queue, "dh⁄dx", mesh_, true, 0.0f),
	CellField<ValueType, MeshType>(queue, "du⁄dx", mesh_, true, 0.0f),
	CellField<ValueType, MeshType>(queue, "dv⁄dx", mesh_, true, 0.0f)
      }),
      dUdy_({
	CellField<ValueType, MeshType>(queue, "dh⁄dy", mesh_, true, 0.0f),
	CellField<ValueType, MeshType>(queue, "du⁄dy", mesh_, true, 0.0f),
	CellField<ValueType, MeshType>(queue, "dv⁄dy", mesh_, true, 0.0f)
      }),
      flux_({
	FaceField<ValueType, MeshType>(queue, "mass", mesh_, true, 0.0f),
	FaceField<ValueType, MeshType>(queue, "xmom", mesh_, true, 0.0f),
	FaceField<ValueType, MeshType>(queue, "ymom", mesh_, true, 0.0f),
	FaceField<ValueType, MeshType>(queue, "wall", mesh_, true, 0.0f)
      }),
      Q_in_({
	Field<ValueType, MeshType, BCFieldMappingType>(queue, "Q_in_0",
						       mesh_, true, 0.0f),
	Field<ValueType, MeshType, BCFieldMappingType>(queue, "Q_in_1",
						       mesh_, true, 0.0f)
      }),
      h_in_({
	Field<ValueType, MeshType, BCFieldMappingType>(queue, "h_in_0",
						       mesh_, true, -1.0f),
	Field<ValueType, MeshType, BCFieldMappingType>(queue, "h_in_1",
						       mesh_, true, -1.0f)
      })
      */
  {
    // Read user-specified values for zb, n, etc.
    generate_field<ValueType, MeshType, FieldMapping::Cell>(zbed_.at(0));
    generate_field<ValueType, MeshType, FieldMapping::Cell>(manning_n_.at(0));
    generate_field<ValueType, MeshType, FieldMapping::Cell>(manning_n_.at(1));
    generate_field<ValueType, MeshType, FieldMapping::Cell>(manning_n_.at(2));
    generate_field<ValueType, MeshType, FieldMapping::Cell>(manning_n_.at(3));

    // Deactivate user-specified areas of the mesh.
    const Config& gconf = GlobalConfig::instance().configuration();
    auto deact_range = gconf.equal_range("deactivate");
    for (auto it = deact_range.first; it != deact_range.second; ++it) {
      MeshSelection<MeshType,FieldMapping::Cell> sel(queue, mesh_, it->second);
      std::cout << "Deactivating " << sel.size() << " cells." << std::endl;
      set_field_nan<ValueField>(sel, zbed_.at(0));
    }

    std::cout << "Initialised solver." << std::endl;
  }

  const std::shared_ptr<sycl::queue>& queue_ptr(void) const
  {
    return queue_;
  }
  
  std::shared_ptr<MeshType>& mesh(void)
  {
    return mesh_;
  }

  Field<ValueType,MeshType,FieldMapping::Cell> initial_depth(void)
  {
    const Config& gconf = GlobalConfig::instance().configuration();
    bool depth_specified = (gconf.count("h") > 0);
    bool stage_specified = (gconf.count("stage") > 0);
    if (depth_specified and stage_specified) {
      std::cerr << "ERROR: both depth and stage initial conditions were specified." << std::endl;
      throw std::runtime_error("User specified both depth and stage initial conditions.");
    } else if (depth_specified) {
      return generate_field<ValueType,MeshType,FieldMapping::Cell>(queue_, "h",
								   mesh_, 0.0f);
    } else if (stage_specified) {
      // Get bed levels and stage (as doubles)
      using DoubleField = Field<double,MeshType,FieldMapping::Cell>;
      DoubleField zb2 = field_cast<ValueField,DoubleField>("zb2", zbed_.at(0));
      DoubleField st2
	= generate_field<double,MeshType,FieldMapping::Cell>(queue_, "stage",
							     mesh_, 0.0f);
      // Calculate depths
      return field_difference<DoubleField, DoubleField,
			      ValueField>("h", st2, zb2);
    } else {
      // No depth/water level initial conditions specified.
      return Field<ValueType,
		   MeshType,
		   FieldMapping::Cell>(queue_, "h", mesh_, true, 0.0f);
    }
  }

  FieldVector<ValueType,MeshType,FieldMapping::Cell,2>
  initial_velocity(const Field<ValueType,MeshType,FieldMapping::Cell>& h)
  {
    const Config& gconf = GlobalConfig::instance().configuration();
    
    bool uv_specified = (gconf.count("u") > 0);
    if (uv_specified and gconf.count("v") == 0) {
      std::cerr << "ERROR: u velocity specified without v velocity." << std::endl;
      throw std::runtime_error("u specified without v");
    }
    if (gconf.count("v") > 0 and not uv_specified) {
      std::cerr << "ERROR: v velocity specified without u velocity." << std::endl;
      throw std::runtime_error("v specified without u");
    }
    
    bool qxy_specified = (gconf.count("qx") > 0);
    if (qxy_specified and gconf.count("qy") == 0) {
      std::cerr << "ERROR: qx flow specified without qy flow." << std::endl;
      throw std::runtime_error("qx specified without qy");
    }
    if (gconf.count("qy") > 0 and not qxy_specified) {
      std::cerr << "ERROR: qy velocity specified without qx velocity." << std::endl;
      throw std::runtime_error("qy specified without qx");
    }
    
    bool qth_specified = (gconf.count("q") > 0);
    if (qth_specified and gconf.count("theta") == 0) {
      std::cerr << "ERROR: q flow specified without theta direction." << std::endl;
      throw std::runtime_error("q specified without theta");
    }
    if (gconf.count("theta") > 0 and not qth_specified) {
      std::cerr << "ERROR: theta flow direction specified without q flow." << std::endl;
      throw std::runtime_error("theta specified without q");
    }

    if (uv_specified) {
      if (qxy_specified) {
	std::cerr << "ERROR: Cannot specify both initial velocity and initial unit flow." << std::endl;
	throw std::runtime_error("Cannot specify both (u,v) and (qx,qy).");
      }
      if (qth_specified) {
	std::cerr << "ERROR: Cannot specify both initial velocity and initial unit flow." << std::endl;
	throw std::runtime_error("Cannot specify both (u,v) and (q,theta).");
      }
      return FieldVector<ValueType,MeshType,FieldMapping::Cell,2>
	({
	  generate_field<ValueType,MeshType,FieldMapping::Cell>(queue_, "u",
								mesh_, 0.0f),
	  generate_field<ValueType,MeshType,FieldMapping::Cell>(queue_, "v",
								mesh_, 0.0f)
	  
	});
    } else if (qxy_specified) {
      if (qth_specified) {
	std::cerr << "ERROR: Cannot specify initial unit flow both as components and magnitude/direction." << std::endl;
	throw std::runtime_error("Cannot specify both (qx,qy) and (q,theta).");
      }
      using ValueField = Field<ValueType,MeshType,FieldMapping::Cell>;
      ValueField qx
	= generate_field<ValueType,MeshType,FieldMapping::Cell>(queue_, "qx",
								mesh_, 0.0f);
      ValueField qy
	= generate_field<ValueType,MeshType,FieldMapping::Cell>(queue_, "qy",
								mesh_, 0.0f);

      Field<ValueType,MeshType,FieldMapping::Cell> u =
	field_division<ValueField,ValueField,ValueField>("u", qx, h);
      
      Field<ValueType,MeshType,FieldMapping::Cell> v =
	field_division<ValueField,ValueField,ValueField>("v", qy, h);
      
      return FieldVector<ValueType,MeshType,FieldMapping::Cell,2>({u,v});
    } else if (qth_specified) {
      throw std::runtime_error("q,theta specification not yet supported.");
    } else {
      return FieldVector<ValueType,MeshType,FieldMapping::Cell,2>
	(queue_, { "u", "v" }, mesh_, true, 0.0f);
    }
  }
  
  SolutionState initial_state(void)
  {
    SolutionState init(queue_, { "h", "u", "v" }, mesh_, true, 0.0f);
    
    const Config& gconf = GlobalConfig::instance().configuration();
    bool depth_specified = (gconf.count("h") > 0);
    bool stage_specified = (gconf.count("stage") > 0);
    
    if (depth_specified and stage_specified) {
      std::cerr << "ERROR: both depth and stage initial conditions were specified." << std::endl;
      throw std::runtime_error("User specified both depth and stage initial conditions.");
    } else if (depth_specified) {
      generate_field<ValueType,MeshType,FieldMapping::Cell>(init.at(0));
    } else if (stage_specified) {
      // Get bed levels and stage (as doubles)
      using DoubleField = Field<double,MeshType,FieldMapping::Cell>;
      DoubleField zb2
	= field_cast<ValueField, DoubleField>("zb2", zbed_.at(0));
      DoubleField st2(queue_, "stage", mesh_, true, 0.0f);
      generate_field<double,MeshType,FieldMapping::Cell>(st2);
      
      // Calculate depths
      field_difference_to<DoubleField,
			  DoubleField,
			  ValueField>(st2, zb2, init.at(0));
    }
    
    // Deactivate user-specified areas of the mesh.
    auto deact_range = gconf.equal_range("deactivate");
    for (auto it = deact_range.first; it != deact_range.second; ++it) {
      MeshSelection<MeshType,FieldMapping::Cell> sel(queue_, mesh_, it->second);
      std::cout << "Deactivating " << sel.size() << " cells." << std::endl;
      set_field_nan<ValueField>(sel, init.at(0));
      set_field_nan<ValueField>(sel, init.at(1));
      set_field_nan<ValueField>(sel, init.at(2));
    }
    /*
    auto uv = initial_velocity(h);
    std::cout << "c" << std::endl;
    SolutionState init = { h, uv.at(0), uv.at(1) };
    std::cout << "d" << std::endl;
    */
    return init;
  }

  void write_check_files(void)
  {
    const GlobalConfig& gc = GlobalConfig::instance();
    stdfs::path check_file_path = gc.get_check_file_path();
    if (not stdfs::exists(check_file_path)) {
      try {
	stdfs::create_directory(check_file_path);
      } catch (stdfs::filesystem_error& err) {
	std::cerr << "Could not create check file directory: "
		  << check_file_path << std::endl;
	throw err;
      }
    } else if (not stdfs::is_directory(check_file_path)) {
      std::cerr << "Could not create check file directory over file: "
		<< check_file_path << std::endl;
      throw std::runtime_error("File exists.");
    }

    std::optional<Config> mesh_conf = gc.write_check_file("mesh");
    if (mesh_conf) {
      mesh_->write_check_file(check_file_path, mesh_conf.value());
    }
    std::optional<Config> ac_conf = gc.write_check_file("active");
    if (ac_conf) {
      auto format = std::make_shared<CSVOutputFormat<ValueType,MeshType>>(Config(), "wkt", ", ", check_file_path);
      std::shared_ptr<OutputFunction<ValueType,MeshType>> ac_func = std::make_shared<IsNaNOutputFunction<ValueType,MeshType,FieldMapping::Cell>>("active cells", &(zbed_.at(0)));
      format->output(ac_func, "init");
    }
    std::optional<Config> zbn_conf = gc.write_check_file("cell constants");
    if (zbn_conf) {
      auto format = std::make_shared<CSVOutputFormat<ValueType,MeshType>>(Config(), "wkt", ", ", check_file_path);
      std::shared_ptr<OutputFunction<ValueType,MeshType>> zbn_func =
	std::make_shared<MultiFieldOutputFunction<ValueType,
						  MeshType,
						  FieldMapping::Cell,
						  ValueType, ValueType,
						  ValueType,
						  ValueType, ValueType,
						  ValueType, ValueType>>
	("cell constants",
	 zbed_.at(0), zbed_.at(1), zbed_.at(2),
	 manning_n_.at(0), manning_n_.at(1),
	 manning_n_.at(2), manning_n_.at(3));
      format->output(zbn_func, "const");
    }
  }

  void clear_boundary_conditions()
  {
    using FieldModifierType = FieldModifier<CellField<ValueType,MeshType>>;
    using CoordType = typename MeshType::CoordType;
    FieldModifierType fm("clear boundaries",
			 MeshSelection<MeshType,BCFieldMappingType>(queue_,
								    mesh_),
			 0.0f, 1.0f, -2.0f, 2.0f, 1.0f);
    FixedValueFieldFunctor<ValueType, CoordType> qfunc(0.0f);
    FixedValueFieldFunctor<ValueType, CoordType> hfunc(-1.0f);
    
    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FixedValueFieldFunctor<ValueType, CoordType>>
      (fm, qfunc, 0.0, Q_in_.at(0));
    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FixedValueFieldFunctor<ValueType, CoordType>>
      (fm, qfunc, 0.0, Q_in_.at(1));
    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FixedValueFieldFunctor<ValueType, CoordType>>
      (fm, hfunc, 0.0, h_in_.at(0));
    modify_field<FieldModifierType,
		 SetOperation<ValueType>,
		 FixedValueFieldFunctor<ValueType, CoordType>>
      (fm, hfunc, 0.0, h_in_.at(1));
  }
  
  FieldVector<ValueType, MeshType, BCFieldMappingType, 2>& Q_in(void)
  {
    return Q_in_;
  }

  FieldVector<ValueType, MeshType, BCFieldMappingType, 2>& h_in(void)
  {
    return h_in_;
  }

  std::shared_ptr<OutputFunction<ValueType,MeshType>>
  get_output_function(const std::string& name,
		      SolutionState& U)
  {
    if (name == "depth") {
      return std::make_shared<DepthOutputFunction<ValueType, MeshType, FieldMapping::Cell>>(&(U.at(0)));
    } else if (name == "stage") {
      return std::make_shared<MultiFieldOutputFunction<ValueType, MeshType, FieldMapping::Cell,ValueType,ValueType,ValueType>>("stage", field_sum<ValueField,ValueField,ValueField>("stage", zbed_.at(0), U.at(0)), zbed_.at(0), U.at(0));
    } else if (name == "component velocity") {
      return std::make_shared<MultiFieldOutputFunction<ValueType, MeshType, FieldMapping::Cell,ValueType,ValueType>>("component velocity", U.at(1), U.at(2));
      // return std::make_shared<ComponentVelocityOutputFunction<ValueType, MeshType, FieldMapping::Cell>>(&(U.at(1)), &(U.at(2)));
    } else if (name == "huv") {
      return std::make_shared<MultiFieldOutputFunction<ValueType, MeshType, FieldMapping::Cell,ValueType,ValueType,ValueType>>("huv", U.at(0), U.at(1), U.at(2));
    } else if (name == "active cells") {
      return std::make_shared<IsNaNOutputFunction<ValueType,MeshType,FieldMapping::Cell>>("active cells", &(zbed_.at(0)));
    } else if (name == "debug boundaries") {
      return std::make_shared<DebugBoundaryOutputFunction<ValueType,MeshType,FieldMapping::Cell>>(&Q_in_, &h_in_);
    } else if (name == "debug slopes") {
      return std::make_shared<DebugSlopeOutputFunction<ValueType,MeshType,FieldMapping::Cell>>(&dUdx_, &dUdy_);
    } else if (name == "debug fluxes") {
      return std::make_shared<DebugFluxOutputFunction<ValueType,MeshType,FieldMapping::Face>>(&flux_);
    } else {
      std::cerr << "Unknown output function type: " << name << std::endl;
      throw std::runtime_error("Unknown output function type");
    }
  }

  void update_ddt(const SolutionState& U,
		  SolutionState& dUdt,
		  const double& time_now, const double& timestep,
		  const double& bdy_t0, const double& bdy_t1)
  {
    spatial_derivative_->calculate(U, dUdx_, dUdy_);
    flux_function_->calculate(U, zbed_, manning_n_, dUdx_, dUdy_, flux_);
    temporal_derivative_->calculate(U, zbed_, manning_n_, Q_in_, h_in_,
				    flux_, dUdt, time_now, timestep, bdy_t0, bdy_t1);
  }
  
  ValueType get_control_number(const SolutionState& U,
			       const double& timestep)
  {
    return SVControlNumber<ValueType,
			   MeshType,
			   FieldMapping::Cell,
			   3>().calculate(U, timestep);
  }
  
};

// #include "BoundaryConditions/SVBoundaryCondition.hpp"

#endif
