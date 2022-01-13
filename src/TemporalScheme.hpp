/***********************************************************************
 * TemporalScheme_hpp
 *
 * Copyright (C) Gerald C J Morgan 2020
 ***********************************************************************/

#ifndef TemporalScheme_hpp
#define TemporalScheme_hpp

#include <memory>

#include "sycl.hpp"
#include "Config.hpp"
#include "OutputDriver.hpp"
#include "BoundaryCondition.hpp"

template<typename Solver>
class TemporalScheme
{
public:

  using SolverType = Solver;
  using ValueType = typename Solver::ValueType;
  using MeshType = typename Solver::MeshType;
  
private:

  std::shared_ptr<sycl::queue> initialise_queue(void) const
  {
    std::cout << "Initialising compute device..." << std::endl;
    sycl::device compute_device =
      GlobalConfig::instance().get_device_parameters().device;
    return std::make_shared<sycl::queue>(compute_device);
  }
  
protected:

  std::shared_ptr<sycl::queue> queue_;
  
  std::shared_ptr<Solver> solver_;

  typename Solver::SolutionState U_;

  std::vector<OutputDriver<TemporalScheme<Solver>>> output_drivers_;
  std::vector<std::shared_ptr<BoundaryCondition<Solver>>> boundary_conditions_;
  
public:

  TemporalScheme()
    : queue_(initialise_queue()),
      solver_(std::make_shared<Solver>(queue_)),
      U_(solver_->initial_state()),
      output_drivers_(create_output_drivers<TemporalScheme<Solver>>()),
      boundary_conditions_(create_boundary_conditions<Solver>(solver_))
  {
    
  }

  virtual ~TemporalScheme(void) {}

  std::shared_ptr<sycl::queue>& queue_ptr(void)
  {
    return queue_;
  }
  
  sycl::queue& queue(void)
  {
    return *queue_;
  }
  
  Solver& solver(void) const
  {
    return *solver_;
  }
  
  //virtual void create_boundary(const Config& conf) = 0;
  //virtual void create_measure(const Config& conf) = 0;

  virtual void write_check_files(void) const
  {
    solver_->write_check_files();
  }
  
  virtual void step(const double& time_now,
		    const double& timestep,
		    const double& bdy_t0,
		    const double& bdy_t1) = 0;

  virtual void accept_step(void) = 0;

  virtual void end_of_step(void) = 0;

  virtual void update_boundaries(const double& bdy_t0,
				 const double& bdy_t1) = 0;

  virtual void update_measures(const double& time_now) = 0;

  std::shared_ptr<OutputFunction<ValueType,MeshType>> get_output_function(const std::string& name)
  {
    return solver_->get_output_function(name, U_);
  }

  void outer_loop(const double& start_time,
		  const double& end_time,
		  const double& step_size,
		  const size_t& display_every)
  {
    // Get user-specified timestepping parameters
    const auto& ts_params = GlobalConfig::instance().get_timestep_parameters();
    double dt = ts_params.time_step;
    double max_dt = ts_params.max_time_step;
    double courant_target = ts_params.courant_target;

    // Number of iterations of outer loop
    size_t nsteps = ((size_t) (0.001 + end_time - start_time) / step_size);

    // Do the initial output
    for (auto&& od : output_drivers_) {
      if (start_time >= od.next_output_time()) {
	od.output(*this);
      }
    }

    // Table output to the console
    DisplayTable<double,double,double,double> screen_output_table
      ({ {10, "t (hours)", "%|.3f|"},
	 {9, "Δt", "%|.4f|"},
	 {9, "tₗ", "%|.3f|"},
	 {9, "Co", "%|.4f|"} }  );
    /*
    screen_output_table.write_top_rule();
    screen_output_table.write_header_row();
    screen_output_table.write_mid_rule();
    */
    
    // Loop over the steps
    for (size_t i = 0; i < nsteps; ++i) {
      // Calculate start and end time of step
      double t_step_start = start_time + i * step_size;
      double t_step_end = t_step_start + step_size;

      inner_loop(dt, max_dt, courant_target,
		 t_step_start, t_step_end,
		 screen_output_table, display_every);
    }
  }

  void inner_loop(double& dt,
		  const double& max_dt,
		  const double& courant_target,
		  const double& t_start,
		  const double& t_end,
		  DisplayTable<double,double,double,double>& so_table,
		  const size_t& display_every)
  {
    solver_->clear_boundary_conditions();
    for (auto&& bdy_ptr : boundary_conditions_) {
      bdy_ptr->update(*this, t_start, t_end);
    }
    this->update_measures(t_start);
    
    size_t repeated_step_count = 0;
    size_t local_repeat_count = 0;
    size_t inner_steps = 0;
    double t_local = 0.0;
    double t_local_end = t_end - t_start;

    bool any_output = true;
    
    while (true) {
      if (any_output) {
	so_table.write_top_rule();
	so_table.write_header_row();
	any_output = false;
      }
    
      // Do the time step
      double t_now = t_start + t_local;
      this->step(t_now, dt, t_start, t_end);

      // Get the solution maximum control number
      double comax = solver_->get_control_number(U_, dt);

      // Variable to hold our new target timestep
      double target_dt = dt;

      if (comax > courant_target) {
	// Simulation is unstable (simulation control number is above
	// our target)
	so_table.write_data_row(t_now / 3600., dt, t_local, comax);

	// Track the number of repeated steps. Give up if we're
	// repeating too many
	local_repeat_count++;
	repeated_step_count++;
	if (repeated_step_count >= 1000) {
	  throw std::runtime_error("Too many repeated steps");
	}

	// Set target timestep for the repeat of this step to be a
	// minimum of 10% of dt, a maximum of 90% of dt
	target_dt = dt * std::fmax(0.1, std::fmin(0.9, comax / courant_target));
      } else {
	// Simulation is stable (simulation control number is below
	// our target)
	local_repeat_count = 0;
	
	// Accept this step.
	this->accept_step();

	// Increment the local time and step counts
	t_local += dt;
	inner_steps++;

	// We might want to consider increasing the timestep.
	if (comax < 0.9 * courant_target) {
	  target_dt = std::fmin(max_dt, dt * 1.1);
	}

	// Output to the console
	if (inner_steps % display_every == 0) {
	  so_table.write_data_row(t_now / 3600., dt, t_local, comax);
	}

	if (t_local >= t_local_end) {
	  // That timestep took us to the end of the inner loop
	  if (inner_steps % display_every != 0) {
	    so_table.write_data_row(t_now / 3600., dt,
					       t_local, comax);
	  }

	  queue_->wait_and_throw();

	  for (auto&& od : output_drivers_) {
	    if (t_start + t_local >= od.next_output_time()) {
	      any_output = true;
	      so_table.write_bot_rule();
	      od.output(*this);
	    }
	  }

	  if (repeated_step_count > 0) {
	    if (not any_output) {
	      so_table.write_bot_rule();
	    } else {
	      so_table.write_mid_rule();
	    }
	    std::cout << "WARNING: repeated " << repeated_step_count
		      << " steps." << std::endl;
	    any_output = true;
	  }

	  /*
	  if (i < nsteps - 1) {
	  }
	  */
	  if (not any_output) {
	    so_table.write_bot_rule();
	  }
	  
	  return;
	} else if (t_local + target_dt > t_local_end) {
	  // The next timestep will take us to or past the end of the
	  // inner loop. Lower it so that it hits exactly.
	  target_dt = t_local_end - t_local;
	  // We need to finish the loop on an even number of steps, so
	  // if we currently have an even number of steps we need to
	  // lower the timestep so that we do two more.
	  if (inner_steps % 2 == 0) {
	    // To do two more steps we need to go 60% of the way
	    target_dt *= 0.6;
	  }
	} else if (t_local + 1.5 * target_dt >= t_local_end) {
	  // This timestep will take us close to the end of the
	  // loop. Lower it so we don't get really close but not
	  // quite there
	  if (inner_steps % 2 == 0) {
	    // If we're on an even step we want to finish in two
	    // steps, so we go 60% of the way there
	    target_dt = 0.6 * (t_local_end - t_local);
	  } else {
	    // If we're on an odd step we want to finish in one or
	    // three steps. Because we can't guarantee that we can
	    // use a high enough timestep to finish in one, we go
	    // 35% of the way there and aim for three.
	    target_dt = 0.35 * (t_local_end - t_local);
	  }
	}	
      }

      dt = target_dt;
    }
  }
  
  void run(void)
  {
    const auto& run_params = GlobalConfig::instance().get_run_parameters();
    const auto& ts_params = GlobalConfig::instance().get_timestep_parameters();
    double start_time = run_params.start_time;
    double end_time = run_params.end_time;
    double sync_step = run_params.sync_step;
    size_t display_every = run_params.display_every;

    //   std::vector<std::shared_ptr<OutputDriver>> output_drivers;
    // TODO: populate list of output drivers
    
    if (ts_params.dt_type == GlobalConfig::TimestepParameters::DtType::fixed) {
      std::cerr << "Fixed timestep mode not currently supported." << std::endl;
      throw std::runtime_error("Fixed timestep mode not supported.");
    } else if (ts_params.dt_type == GlobalConfig::TimestepParameters::DtType::adaptive) {
      outer_loop(start_time, end_time, sync_step, display_every);
    }
  }
    /*
      size_t nsteps = ((size_t) (0.001 + end_time - start_time) / sync_step);
      double dt = ts_params.time_step;
      double max_dt = ts_params.max_time_step;
      double courant_target = ts_params.courant_target;

      DisplayTable<double,double,double,double> screen_output_table
	({ {10, "t (hours)", "%|.3f|"},
	   {9, "Δt", "%|.4f|"},
	   {9, "tₗ", "%|.3f|"},
	   {9, "Co", "%|.4f|"} }  );
      
      // double t_eps = 0.0001;
      for (auto&& od : output_drivers_) {
	if (start_time >= od.next_output_time()) {
	  od.output(*this);
	}
      }

      screen_output_table.write_top_rule();
      screen_output_table.write_header_row();
      screen_output_table.write_mid_rule();

	  
      for (size_t i = 0; i < nsteps; ++i) {
	double t_o = start_time + i * sync_step;
	double t_n = t_o + sync_step;
	// std::cout << "Output at " << t_o << std::endl;

	solver_->clear_boundary_conditions();
	for (auto&& bdy_ptr : boundary_conditions_) {
	  bdy_ptr->update(*this, t_o, t_o + sync_step);
	}
	this->update_measures(t_o);
	
	size_t repeated_step_count = 0;
	size_t local_repeat_count = 0;
	size_t inner_steps = 0;
	double t_local = 0.0;

	while (true) {
	  double t = t_o + t_local;
	  this->step(t, dt, t_o, t_n);
	  double comax = solver_->get_control_number(U_, dt);
	  double dt_fac = courant_target / comax;
	  double target_dt = dt;

	  if (comax > courant_target) {
	    screen_output_table.write_data_row(t / 3600., dt, t_local, comax);
	    // Simulation is unstable
	    target_dt = dt * std::fmax(0.1, std::fmin(0.9, dt_fac));
	    // std::cout << "Repeating step..." << std::endl;
	    local_repeat_count++;
	    repeated_step_count++;
	    if (repeated_step_count >= 1000) {
	      throw std::runtime_error("Too many repeated steps");
	    }
	    continue;
	  }

	  local_repeat_count = 0;

	  // If we got here, simulation was stable, accept this step.
	  this->accept_step();
	  t_local += dt;
	  inner_steps++;

	  if (target_dt != dt) {
	    // If we haven't lowered the timestep, we might want to
	    // consider increasing it.
	    target_dt = std::fmin(max_dt,
				  dt * (dt_fac > 1.1 ? 1.1 : dt_fac));
	  }

	  if (inner_steps % display_every == 0) {
	    screen_output_table.write_data_row(t / 3600., dt, t_local, comax);
	  }

	  if (t_local >= sync_step) {
	    // That timestep took us to the end of the inner loop
	    if (inner_steps % display_every != 0) {
	      screen_output_table.write_data_row(t / 3600., dt, t_local, comax);
	    }

	    queue_->wait_and_throw();
	    
	    bool any_output = false;
	    for (auto&& od : output_drivers_) {
	      if (t_o + t_local >= od.next_output_time()) {
		any_output = true;
		screen_output_table.write_bot_rule();
		od.output(*this);
	      }
	    }

	    if (repeated_step_count > 0) {
	      if (not any_output) {
		screen_output_table.write_bot_rule();
	      }
	      std::cout << "WARNING: repeated " << repeated_step_count
			<< " steps." << std::endl;
	      any_output = true;
	    }

	    if (i < nsteps - 1) {
	      if (any_output) {
		screen_output_table.write_top_rule();
		screen_output_table.write_header_row();
	      }
	      screen_output_table.write_mid_rule();
	    }

	    break;
	  } else if (t_local + target_dt > sync_step) {
	    // This timestep will take us to the end of the inner loop
	    if (inner_steps % 2 == 0) {
	      dt = 0.6 * (sync_step - t_local);
	    } else {
	      dt = sync_step - t_local;
	    }
	    // std::cout << "new dt: " << dt << std::endl;
	  } else if (t_local + 1.5 * target_dt >= sync_step) {
	    // This timestep will take us close to the end of the loop
	    if (inner_steps % 2 == 0) {
	      dt = 0.6 * (sync_step - t_local);
	    } else {
	      dt = 0.35 * (sync_step - t_local);
	    }
	    // std::cout << "new dt (1): " << dt << std::endl;
	  } else {
	    dt = target_dt;
	  }
	  
	}
	//	this->end_of_step();
      }
    } else {
      throw std::logic_error("Timestepping type not set");
    }
  }
    */
};

#endif
