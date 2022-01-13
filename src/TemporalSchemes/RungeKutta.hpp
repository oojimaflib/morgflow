/***********************************************************************
 * TemporalSchemes/RungeKutta.hpp
 *
 * Copyright (C) Gerald C J Morgan 2020
 ***********************************************************************/

#ifndef TemporalSchemes_RungeKutta_hpp
#define TemporalSchemes_RungeKutta_hpp

#include "../TemporalScheme.hpp"
//#include "../BoundaryCondition.hpp"
//#include "../Measure.hpp"

template<int S>
class RungeKuttaCoefficientSet
{
private:

  std::array<std::array<float, S>, S+1> a_;
  std::array<float, S> c_;

public:

  RungeKuttaCoefficientSet(const std::array<std::array<float, S>, S+1>& a,
			   const std::array<float, S>& c)
    : a_(a), c_(c)
  {
    std::cout << "Butcher tableau for Runge Kutta scheme is:" << std::endl;
    for (int i = 0; i < S; ++i) {
      std::cout << std::to_string(c_[i]) << " │ ";
      for (int j = 0; j < i; ++j) {
	std::cout << std::to_string(a_[i][j]) << "   ";
      }
      std::cout << std::endl;
    }
    std::cout << "────────" << "─┼─";
    for (int j = 0; j < S+1; ++j) {
      std::cout << "────────";
    }
    std::cout << std::endl;
    std::cout << std::string(8, ' ') << " │ ";
    for (int i = 0; i < S; ++i) {
      std::cout << std::to_string(a_[S][i]) << "   ";
    }
    std::cout << std::endl;
  }

  const float& a(const size_t& i, const size_t& j) const
  {
    return a_[i][j];
  }

  const float& c(const size_t& i) const
  {
    return c_[i];
  }
  
};

template<typename Solver, int S>
class RungeKuttaTemporalScheme : public TemporalScheme<Solver>
{
private:

  std::shared_ptr<RungeKuttaCoefficientSet<S>> coeffs_;

  typename Solver::SolutionState Ustar_;
  std::array<typename Solver::SolutionState, S> dUdt_;

  //  double courant_target_;

  using SSAccessorRO = typename Solver::SolutionState::template Accessor<sycl::access::mode::read>;
  using SSAccessorRW = typename Solver::SolutionState::template Accessor<sycl::access::mode::read_write>;

  template<size_t S2>
  std::array<typename Solver::SolutionState, S2> construct_dUdt(void);

  template<>
  std::array<typename Solver::SolutionState, 1> construct_dUdt(void)
  {
    return {
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_0")
    };
  }
  template<>
  std::array<typename Solver::SolutionState, 2> construct_dUdt(void)
  {
    return {
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_0"),
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_1")
    };
  }
  template<>
  std::array<typename Solver::SolutionState, 3> construct_dUdt(void)
  {
    return {
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_0"),
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_1"),
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_2")
    };
  }
  template<>
  std::array<typename Solver::SolutionState, 4> construct_dUdt(void)
  {
    return {
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_0"),
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_1"),
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_2"),
      typename Solver::SolutionState("(d", this->U_, "⁄dt)_3")
    };
  }

  class RungeKuttaStep
  {
  private:

    size_t step_;
    RungeKuttaCoefficientSet<S> coeffs_;

    double time_now_;
    double timestep_;

    SSAccessorRW Ustar_rw_;
    SSAccessorRO U_ro_;
    std::array<SSAccessorRO, S> dUdt_ro_;

  public:

    RungeKuttaStep(const size_t& step,
		   const RungeKuttaCoefficientSet<S>& coeffs,
		   const double& time_now,
		   const double& timestep,
		   const SSAccessorRW& Ustar_rw,
		   const SSAccessorRO& U_ro,
		   const std::array<SSAccessorRO, S>& dUdt_ro)
      : step_(step),
	coeffs_(coeffs),
	time_now_(time_now),
	timestep_(timestep),
	Ustar_rw_(Ustar_rw),
	U_ro_(U_ro),
	dUdt_ro_(dUdt_ro)
    {}

    void operator()(sycl::item<1> item) const {
      for (size_t vec_id = 0; vec_id < Ustar_rw_.size(); ++vec_id) {
	Ustar_rw_[vec_id][item] = U_ro_[vec_id][item];

	if (step_ > 0) {
	  for (size_t i = 0; i < step_; ++i) {
	    Ustar_rw_[vec_id][item] += timestep_ * coeffs_.a(step_, i) * dUdt_ro_[i][vec_id][item];
	  }
	}

      }
      
      if (Ustar_rw_[0][item] < 0.0) {
	Ustar_rw_[0][item] = 0.0;
	Ustar_rw_[1][item] = 0.0;
	Ustar_rw_[2][item] = 0.0;
      } else if (Ustar_rw_[0][item] < 1e-4) {
	Ustar_rw_[1][item] = 0.0;
	Ustar_rw_[2][item] = 0.0;
      }
    }

  };

  void update_Ustar(size_t step,
		    const double& time_now, const double& timestep,
		    const double& bdy_t0, const double& bdy_t1)
  {
    this->queue_->submit([&] (sycl::handler& cgh) {
      SSAccessorRW Ustar_rw =
	Ustar_.template get_accessor<sycl::access::mode::read_write>(cgh);

      SSAccessorRO U_ro =
	this->U_.template get_accessor<sycl::access::mode::read>(cgh);
      
      std::array<SSAccessorRO, S> dUdt_ro;
      for (size_t i = 0; i < S; ++i) {
	dUdt_ro[i] =
	  dUdt_[i].template get_accessor<sycl::access::mode::read>(cgh);
      }

      auto kernel = RungeKuttaStep(step, *coeffs_, time_now, timestep,
				   Ustar_rw, U_ro, dUdt_ro);
      
      cgh.parallel_for(this->U_.get_range(), kernel);
    });

    if (step < S) {
      this->solver_->update_ddt(Ustar_,
				dUdt_[step],
				time_now + coeffs_->c(step) * timestep,
				timestep, bdy_t0, bdy_t1);
    }
  }

public:

  RungeKuttaTemporalScheme(const std::shared_ptr<RungeKuttaCoefficientSet<S>>& coeffs)
    : TemporalScheme<Solver>(),
      coeffs_(coeffs),
      Ustar_("", this->U_, "*"),
      dUdt_(construct_dUdt<S>())
  {
  }

  virtual ~RungeKuttaTemporalScheme(void) {}

  virtual void step(const double& time_now, const double& timestep,
		    const double& bdy_t0, const double& bdy_t1)
  {
    for (size_t st = 0; st <= S; ++st) {
      //std::cout << "sub-step " << st << std::endl;
      update_Ustar(st, time_now, timestep, bdy_t0, bdy_t1);
    }
  }

  virtual void accept_step(void)
  {
    std::swap(this->U_, Ustar_);
  }

  virtual void end_of_step(void)
  {
    // U_.end_of_step(this->queue_);
  }

  virtual void update_boundaries(const double& bdy_t0,
				 const double& bdy_t1)
  {
  }

  virtual void update_measures(const double& t)
  {
  }

  static std::shared_ptr<TemporalScheme<Solver>>
  create(void)
  {
    Config empty;
    const Config& config = GlobalConfig::instance().configuration().get_child("temporal scheme", empty);
    if (config.count("method") > 0) {
      std::string method = config.get<std::string>("method");

      std::cout << "Using a Runge-Kutta temporal scheme: '"
		<< method << "':" << std::endl;
      
      if (method == "Euler") {
	std::shared_ptr<RungeKuttaCoefficientSet<1>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<1>>
	  (std::array<std::array<float, 1>,2>({{
		{0.0,},
		{1.0,},
	      }}),
	    //std::array<float, 1>({{1.0,}}),
	    std::array<float, 1>({{0.0,}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,1>>(coeffs);
      } else if (method == "midpoint") {
	std::shared_ptr<RungeKuttaCoefficientSet<2>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<2>>
	  (std::array<std::array<float, 2>,3>({{
		{0.0, 0.0},
		{0.5, 0.0},
		{0.0, 1.0},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 2>({{0.0, 0.5}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,2>>(coeffs);
      } else if (method == "Heun") {
	std::shared_ptr<RungeKuttaCoefficientSet<2>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<2>>
	  (std::array<std::array<float, 2>,3>({{
		{0.0, 0.0},
		{1.0, 0.0},
		{0.5, 0.5},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 2>({{0.0, 1.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,2>>(coeffs);
      } else if (method == "Ralston") {
	std::shared_ptr<RungeKuttaCoefficientSet<2>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<2>>
	  (std::array<std::array<float, 2>,3>({{
		{0.0, 0.0},
		{2.0/3.0, 0.0},
		{0.25, 0.75},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 2>({{0.0, 2.0/3.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,2>>(coeffs);
      } else if (method == "generic2") {
	float alpha = config.get<float>("alpha");
	std::shared_ptr<RungeKuttaCoefficientSet<2>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<2>>
	  (std::array<std::array<float, 2>,3>({{
		{0.0, 0.0},
		{alpha, 0.0},
		{1.0f - 1.0f / (2.0f * alpha), 1.0f / (2.0f * alpha)},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 2>({{0.0, alpha}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,2>>(coeffs);
      } else if (method == "Kutta3") {
	std::shared_ptr<RungeKuttaCoefficientSet<3>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<3>>
	  (std::array<std::array<float, 3>,4>({{
		{0.0, 0.0, 0.0},
		{0.5, 0.0, 0.0},
		{-1.0, 2.0, 0.0},
		{1.0/6.0, 2.0/3.0, 1.0/6.0},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 3>({{0.0, 0.5, 1.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,3>>(coeffs);
      } else if (method == "Heun3") {
	std::shared_ptr<RungeKuttaCoefficientSet<3>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<3>>
	  (std::array<std::array<float, 3>,4>({{
		{0.0, 0.0, 0.0},
		{1.0/3.0, 0.0, 0.0},
		{0.0, 2.0/3.0, 0.0},
		{0.25, 0.0, 0.75},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 3>({{0.0, 1.0/3.0, 2.0/3.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,3>>(coeffs);
      } else if (method == "Ralston3") {
	std::shared_ptr<RungeKuttaCoefficientSet<3>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<3>>
	  (std::array<std::array<float, 3>,4>({{
		{0.0, 0.0, 0.0},
		{0.5, 0.0, 0.0},
		{0.0, 0.75, 0.0},
		{2.0/9.0, 1.0/3.0, 4.0/9.0},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 3>({{0.0, 0.5, 0.75}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,3>>(coeffs);
      } else if (method == "SSPRK3") {
	std::shared_ptr<RungeKuttaCoefficientSet<3>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<3>>
	  (std::array<std::array<float, 3>,4>({{
		{0.0, 0.0, 0.0},
		{1.0, 0.0, 0.0},
		{0.25, 0.25, 0.0},
		{1.0/6.0, 1.0/6.0, 2.0/3.0},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 3>({{0.0, 1.0, 0.5}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,3>>(coeffs);
      } else if (method == "generic3") {
	float alpha = config.get<float>("alpha");
	std::shared_ptr<RungeKuttaCoefficientSet<3>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<3>>
	  (std::array<std::array<float, 3>,4>({{
		{0.0, 0.0, 0.0},
		{alpha, 0.0, 0.0},
		{1.0f + (1.0f - alpha) / (alpha * (3.0f * alpha - 2.0f)),
		 -(1.0f - alpha) / (alpha * (3.0f * alpha - 2.0f)),
		 0.0},
		{0.5f - 1.0f / (6.0f * alpha),
		 1.0f / (6.0f * alpha * (1.0f - alpha)),
		 (2.0f - 3.0f * alpha) / (6.0f * (1.0f - alpha))},
	      }}),
	    // std::array<float, 2>({{0.0, 1.0}}),
	    std::array<float, 3>({{0.0, alpha, 1.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,3>>(coeffs);
      } else if (method == "classic") {
	std::shared_ptr<RungeKuttaCoefficientSet<4>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<4>>
	  (std::array<std::array<float, 4>,5>({{
		{0.0, 0.0, 0.0, 0.0},
		{0.5, 0.0, 0.0, 0.0},
		{0.0, 0.5, 0.0, 0.0},
		{0.0, 0.0, 1.0, 0.0},
		{1.0/6.0, 1.0/3.0, 1.0/3.0, 1.0/6.0},
	      }}),
	    // std::array<float, 4>({{1.0/6.0, 1.0/3.0, 1.0/3.0, 1.0/6.0}}),
	    std::array<float, 4>({{0.0, 0.5, 0.5, 1.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,4>>(coeffs);
      } else if (method == "Ralston4") {
	std::shared_ptr<RungeKuttaCoefficientSet<4>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<4>>
	  (std::array<std::array<float, 4>,5>({{
		{       0.0,         0.0,        0.0, 0.0},
		{       0.4,         0.0,        0.0, 0.0},
		{0.29697761,  0.15875964,        0.0, 0.0},
		{0.21810040, -3.05096516, 3.83286476, 0.0},
		{0.17476028, -0.55148066, 1.20553560, 0.17118478},
	      }}),
	    // std::array<float, 4>({{1.0/6.0, 1.0/3.0, 1.0/3.0, 1.0/6.0}}),
	    std::array<float, 4>({{0.0, 0.4, 0.45573725, 1.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,4>>(coeffs);
      } else if (method == "3/8") {
	std::shared_ptr<RungeKuttaCoefficientSet<4>> coeffs
	  = std::make_shared<RungeKuttaCoefficientSet<4>>
	  (std::array<std::array<float, 4>,5>({{
		{ 0.0, 0.0, 0.0, 0.0},
		{ 1.0/3.0, 0.0, 0.0, 0.0},
		{ -1.0/3.0, 1.0, 0.0, 0.0},
		{ 1.0, -1.0, 1.0, 0.0},
		{ 1.0/8.0, 3.0/8.0, 3.0/8.0, 1.0/8.0},
	      }}),
	    // std::array<float, 4>({{1.0/6.0, 1.0/3.0, 1.0/3.0, 1.0/6.0}}),
	    std::array<float, 4>({{0.0, 1.0/3.0, 2.0/3.0, 1.0}}));
	return std::make_shared<RungeKuttaTemporalScheme<Solver,4>>(coeffs);
      } else {
	std::cerr << "Temporal Scheme \"" << method << "\" not known." << std::endl;
	throw std::runtime_error("Temporal scheme not known");
      }
    } else {
      std::cerr << "No temporal scheme specified." << std::endl;
      throw std::runtime_error("No temporal scheme specified");
    }
  }
  
};

#endif
