/***********************************************************************
 * geom_test.cpp
 *
 * Program for terrain analysis
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "../GlobalConfig.cpp"
#include "../Config.cpp"
#include "../Meshes/Cartesian2DMesh.cpp"
#include "../Geometry.cpp"
#include "../RasterField.cpp"

int main(int argc, char* argv[])
{
  std::locale loc;
  GlobalConfig::init(argc, argv);

  const Config& conf = GlobalConfig::instance().configuration();

  GeometryCollection gc(conf);
  
  std::shared_ptr<Cartesian2DMesh> mesh =
    std::make_shared<Cartesian2DMesh>(conf.get_child("mesh"));

  if (gc.size() == 1 and gc.at(0)->type() == Geometry::Type::polygon) {
    const Polygon& poly = *std::dynamic_pointer_cast<Polygon>(gc.at(0));
    // select_polygon(poly, inverted);
    mesh->template for_each_object_within<FieldMapping::Cell>
      (poly, [&](const size_t& id) { std::cout << id << std::endl; }, true);
  } else if (gc.size() == 1 and
	     gc.at(0)->type() == Geometry::Type::multipolygon) {
    const MultiPolygon& mpoly =
      *std::dynamic_pointer_cast<MultiPolygon>(gc.at(0));
    if (mpoly.size() == 1) {
      // select_polygon(mp.at(0), inverted);
      mesh->template for_each_object_within<FieldMapping::Cell>
	(mpoly.at(0),
	 [&](const size_t& id) { std::cout << id << std::endl; }, true);
    } else {
      std::cerr << "Cannot invert with multipolygon geometry "
		<< "containing more than one polygon." << std::endl;
      throw std::runtime_error("Cannot invert with multipolygon geometry "
			       "containing more than one polygon.");
    }
  } else {
    std::cerr << "Cannot invert non-polygon geometry." << std::endl;
    throw std::runtime_error("Cannot invert non-polygon geometry.");
  }

  return 0;
}
