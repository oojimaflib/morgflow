/***********************************************************************
 * mflow.cpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "Config.hpp"
#include "Mesh.hpp"
#include "FieldVector.hpp"

#include "SVSolver.hpp"
#include "SpatialDerivative.hpp"
#include "TemporalSchemes/RungeKutta.hpp"

#include "GlobalConfig.cpp"
#include "Config.cpp"
#include "Meshes/Cartesian2DMesh.cpp"
#include "Geometry.cpp"
#include "OutputFormat.cpp"
#include "BoundaryConditions/SVBoundaryCondition.cpp"

int main(int argc, char* argv[])
{
  std::locale loc;
  GlobalConfig::init(argc, argv);

  std::cout << "Initialised global configuration" << std::endl;
  using Solver = SVSolver;
  
  std::shared_ptr<TemporalScheme<Solver>> scheme =
    RungeKuttaTemporalScheme<Solver,1>::create();

  scheme->write_check_files();
  scheme->run();

  return 0;
};

  
