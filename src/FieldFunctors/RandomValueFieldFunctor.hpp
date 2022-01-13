/***********************************************************************
 * RandomValueFieldFunctor.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef FieldFunctors_RandomValueFieldFunctor_hpp
#define FieldFunctors_RandomValueFieldFunctor_hpp

#include "../FieldFunctor.hpp"

#include <random>

template<typename T>
class RandomNumberGenerator
{
public:

  RandomNumberGenerator(const Config& config) {}

  virtual ~RandomNumberGenerator(void) {}

  virtual T get_value(void) const = 0;
  
};

template<typename T,
	 typename Generator>
class RandomGenerator : public RandomNumberGenerator<T>
{
protected:

  mutable Generator gen_;
  
public:

  RandomGenerator(const Config& config)
    : RandomNumberGenerator<T>(config), gen_()
  {
    std::vector<uint32_t> seed_vec(split_string<uint32_t>(config.get<std::string>("seed")));
    std::seed_seq ss(seed_vec.begin(), seed_vec.end());
    gen_.seed(ss);
  }

  virtual ~RandomGenerator(void) {}

  virtual T get_value(void) const = 0;
  
};

template<typename T,
	 typename Generator,
	 typename Distribution>
class RandomDistribution;

#define DECLARE_RANDOM_DISTRIBUTION(DIST_TYPE_NAME, CONSTRUCTOR_ARGS)	\
  template<typename T,							\
	   typename Generator>						\
  class RandomDistribution<T,Generator,DIST_TYPE_NAME>			\
    : public RandomGenerator<T,Generator>				\
  {									\
  private:								\
									\
    mutable DIST_TYPE_NAME dist_;					\
									\
  public:								\
									\
    RandomDistribution<T,Generator,DIST_TYPE_NAME>(const Config& config) \
      : RandomGenerator<T,Generator>(config),				\
      dist_(DIST_TYPE_NAME CONSTRUCTOR_ARGS)				\
      {}								\
									\
    virtual ~RandomDistribution<T,Generator,DIST_TYPE_NAME>(void) {}	\
									\
    virtual T get_value(void) const { return dist_(this->gen_); }	\
									\
  }

DECLARE_RANDOM_DISTRIBUTION(std::uniform_real_distribution<T>,
			    (config.get<T>("min"), config.get<T>("max")));

DECLARE_RANDOM_DISTRIBUTION(std::exponential_distribution<T>,
			    (config.get<T>("lambda")));
DECLARE_RANDOM_DISTRIBUTION(std::gamma_distribution<T>,
			    (config.get<T>("alpha"), config.get<T>("beta")));
DECLARE_RANDOM_DISTRIBUTION(std::weibull_distribution<T>,
			    (config.get<T>("a"), config.get<T>("b")));
DECLARE_RANDOM_DISTRIBUTION(std::extreme_value_distribution<T>,
			    (config.get<T>("a"), config.get<T>("b")));

DECLARE_RANDOM_DISTRIBUTION(std::normal_distribution<T>,
			    (config.get<T>("mean"), config.get<T>("std dev")));
DECLARE_RANDOM_DISTRIBUTION(std::lognormal_distribution<T>,
			    (config.get<T>("m"), config.get<T>("s")));
DECLARE_RANDOM_DISTRIBUTION(std::chi_squared_distribution<T>,
			    (config.get<T>("n")));
DECLARE_RANDOM_DISTRIBUTION(std::cauchy_distribution<T>,
			    (config.get<T>("a"), config.get<T>("b")));
DECLARE_RANDOM_DISTRIBUTION(std::fisher_f_distribution<T>,
			    (config.get<T>("m"), config.get<T>("n")));
DECLARE_RANDOM_DISTRIBUTION(std::student_t_distribution<T>,
			    (config.get<T>("n")));

/*
template<typename T,
	 typename Generator>
class RandomDistribution<T,Generator,std::normal_distribution<T>>
  : public RandomGenerator<T,Generator>
{
private:

  mutable std::normal_distribution<T> dist_;

public:

  RandomDistribution<T,Generator,std::normal_distribution<T>>(const Config& config)
    : RandomGenerator<T,Generator>(config),
      dist_(std::normal_distribution<T>(config.get<T>("mean"),
					config.get<T>("std dev")))
  {}

  virtual ~RandomDistribution<T,Generator,std::normal_distribution<T>>(void) {}

  virtual T get_value(void) const { return dist_(this->gen_); }
  
};
*/



template<typename T,
	 typename CoordType,
	 typename FFOp = FieldFunctorMeanOperation<T>>
class RandomValueFieldFunctor : public FieldFunctor
{
public:
  
  static const bool host_only = true;
  
private:

  std::shared_ptr<RandomNumberGenerator<T>> rng_;

  template<typename Engine>
  void construct_rng(const Config& config)
  {
    std::string dist_type_str = config.get<std::string>("distribution");
    if (dist_type_str == "uniform") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::uniform_real_distribution<T>>>(config);
    } else if (dist_type_str == "exponential") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::exponential_distribution<T>>>(config);
    } else if (dist_type_str == "gamma") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::gamma_distribution<T>>>(config);
    } else if (dist_type_str == "weibull") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::weibull_distribution<T>>>(config);
    } else if (dist_type_str == "extreme value") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::extreme_value_distribution<T>>>(config);
    } else if (dist_type_str == "normal") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::normal_distribution<T>>>(config);
    } else if (dist_type_str == "log normal") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::lognormal_distribution<T>>>(config);
    } else if (dist_type_str == "chi squared") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::chi_squared_distribution<T>>>(config);
    } else if (dist_type_str == "cauchy") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::cauchy_distribution<T>>>(config);
    } else if (dist_type_str == "fisher f") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::fisher_f_distribution<T>>>(config);
    } else if (dist_type_str == "student t") {
      rng_ = std::make_shared<RandomDistribution<T, Engine, std::student_t_distribution<T>>>(config);
    } else {
      std::cerr << "Distribution type: " << dist_type_str << " is not supported." << std::endl;
      throw std::runtime_error("Distribution type not supported.");
    }
  }
  
  void construct_rng(const Config& config)
  {
    std::string engine_type_str = config.get<std::string>("engine",
							  "mersenne twister 1998");
    if (engine_type_str == "mersenne twister 1998") {
      construct_rng<std::mt19937>(config);
    } else if (engine_type_str == "mersenne twister 2000") {
      construct_rng<std::mt19937_64>(config);
    } else if (engine_type_str == "minimal standard 1988") {
      construct_rng<std::minstd_rand0>(config);
    } else if (engine_type_str == "minimal standard 1993") {
      construct_rng<std::minstd_rand>(config);
    } else if (engine_type_str == "ranlux 24") {
      construct_rng<std::ranlux24>(config);
    } else if (engine_type_str == "ranlux 48") {
      construct_rng<std::ranlux48>(config);
    } else if (engine_type_str == "ranlux 24 base") {
      construct_rng<std::ranlux24_base>(config);
    } else if (engine_type_str == "ranlux 48 base") {
      construct_rng<std::ranlux48_base>(config);
    } else if (engine_type_str == "knuth b") {
      construct_rng<std::knuth_b>(config);
    } else {
      std::cerr << "Random number engine type: " << engine_type_str << " is not supported." << std::endl;
      throw std::runtime_error("Random number engine type not supported.");
    }
  }

public:

  RandomValueFieldFunctor(const std::shared_ptr<sycl::queue>& queue,
			 const Config& config)
    : FieldFunctor()
  {
    construct_rng(config);
  }
  
  virtual ~RandomValueFieldFunctor(void) {}

  virtual std::string name(void) const
  {
    return "Random Value";
  }
  
  void bind(sycl::handler& cgh) {}

  T operator()(const double& time,
	       const CoordType& coord,
	       const T& nodata) const
  {
    return rng_->get_value();
    (void) time;
    (void) coord;
    (void) nodata;
  }
  
  T operator()(const double& time,
	       const CoordType& coord,
	       const std::array<double,2>& box_size,
	       const T& nodata) const
  {
    return rng_->get_value();
    (void) time;
    (void) nodata;
  }
  
};

#endif
