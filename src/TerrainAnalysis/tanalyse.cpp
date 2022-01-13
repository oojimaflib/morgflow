/***********************************************************************
 * tanalyse.cpp
 *
 * Program for terrain analysis
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "../RasterField.hpp"
#include "../FieldGenerator.hpp"
#include "../OutputFormats/CSVOutputFormat.hpp"

#include "../RasterField.cpp"
#include "../GlobalConfig.cpp"
#include "../Config.cpp"
#include "../Meshes/Cartesian2DMesh.cpp"
#include "../Geometry.cpp"

int main(int argc, char* argv[])
{
  std::locale loc;
  GlobalConfig::init(argc, argv);

  const Config& conf = GlobalConfig::instance().configuration();

  sycl::device compute_device =
    GlobalConfig::instance().get_device_parameters().device;
  std::shared_ptr<sycl::queue> queue =
    std::make_shared<sycl::queue>(compute_device);

  /*
  std::shared_ptr<RasterField<float>> rf =
    RasterField<float>::load_gdal(queue, conf.get_child("raster field"));

  std::vector<std::array<double,2>> test_poly =
    {
      { 389763., 437259. },
      { 389763., 439286. },
      { 391149., 439286. },
      { 391149., 437259. },
      { 389763., 437259. },
    };
  std::vector<size_t> pixel_list = rf->get_pixels_in_polygon(test_poly);

  for (auto&& px : pixel_list) {
    std::cout << px << std::endl;
  }

  return 0;
  */
  
  std::shared_ptr<Cartesian2DMesh> mesh =
    std::make_shared<Cartesian2DMesh>(conf.get_child("mesh"));

  CellField<float, Cartesian2DMesh> zb = CellField<float, Cartesian2DMesh>(queue, "zb", mesh, 0.0f);
  generate_field<float, Cartesian2DMesh, FieldMapping::Cell>(zb);
  
  stdfs::path check_file_path = GlobalConfig::instance().get_check_file_path();
  auto format = std::make_shared<CSVOutputFormat<float,Cartesian2DMesh>>(Config(), "xyz", ", ", check_file_path);
  
  std::shared_ptr<OutputFunction<float,Cartesian2DMesh>> ac_func =
    std::make_shared<SingleFieldOutputFunction<float,Cartesian2DMesh,FieldMapping::Cell>>(zb);
  format->output(ac_func, "zb");
  
  return 0;
}

