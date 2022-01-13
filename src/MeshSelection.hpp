/***********************************************************************
 * MeshSelection.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef MeshSelection_hpp
#define MeshSelection_hpp

#include "Field.hpp"
#include "Geometry.hpp"

template<typename MeshDefn, FieldMapping FM>
class MeshSelection
{
public:

  using MeshType = MeshDefn;
  static const FieldMapping FieldMappingType = FM;
  
private:

  std::shared_ptr<sycl::queue> queue_;
  
  std::shared_ptr<MeshDefn> meshdefn_p_;

  std::shared_ptr<DataArray<size_t>> list_;

  size_t id_at_location(const std::array<double,2>& loc)
  {
    size_t idmax = meshdefn_p_->template object_count<FieldMappingType>();
    size_t id = meshdefn_p_->template get_nearest_object<FieldMappingType>(loc);
    if (id < idmax) {
      return id;
    } else {
      std::cerr << "Cannot select ID outside mesh ("
		<< id << ") at (" << loc[0] << ", " << loc[1] << ")"
		<< std::endl;
      throw std::runtime_error("Cannot select ID outside mesh");
    }
  }
  
  void allocate_list(std::vector<size_t>& id_list)
  {
    std::sort(id_list.begin(), id_list.end());
    auto last = std::unique(id_list.begin(), id_list.end());
    id_list.erase(last, id_list.end());
    list_ = std::make_shared<DataArray<size_t>>(queue_, id_list);
    list_->move_to_device();
  }
  
public:

  MeshSelection(const std::shared_ptr<sycl::queue>& queue,
		const std::shared_ptr<MeshDefn>& meshdefn_p,
		const Config& conf = Config())
    : queue_(queue),
      meshdefn_p_(meshdefn_p),
      list_()
  {
    // Parse the configuration to get a selection. Empty config must
    // equal global selection
    std::string sel_type_str = conf.get_value<std::string>("global");
    if (sel_type_str == "global" or sel_type_str == "") {
      // We need do nothing further. A global selection is indicated
      // by list_ being an uninitialised shared pointer.
      return;
    }

    size_t idmax = meshdefn_p_->template object_count<FieldMappingType>();
    std::vector<size_t> id_list;

    if (sel_type_str == "id list") {
      // The user supplies a raw list of IDs
      auto erange = conf.equal_range("id");
      for (auto it = erange.first; it != erange.second; ++it) {
	std::vector<size_t> local_id_list =
	  split_string<size_t>(it->second.get_value<std::string>());
	for (auto&& id : local_id_list) {
	  if (id < idmax) {
	    id_list.push_back(id);
	  } else {
	    std::cerr << "Cannot select ID outside mesh ("
		      << id << ")" << std::endl;
	    throw std::runtime_error("Cannot select ID outside mesh");
	  }
	}
      }
    } else if (sel_type_str == "location list") {
      // The user supplies a list of coordinates
      auto erange = conf.equal_range("at");
      for (auto it = erange.first; it != erange.second; ++it) {
	typename MeshType::CoordType loc = split_string<double,2>(it->second.get_value<std::string>());
	id_list.push_back(id_at_location(loc));
      }
    } else if (sel_type_str == "gis") {
      GeometryCollection gc(conf);
      bool inverted = conf.get<bool>("inverted", false);
      if (inverted) {
	// The user wants the effect inverted. This can only work if
	// we have exactly one object of type polygon...
	if (gc.size() == 1 and gc.at(0)->type() == Geometry::Type::polygon) {
	  const Polygon& poly = *std::dynamic_pointer_cast<Polygon>(gc.at(0));
	  // select_polygon(poly, inverted);
	  meshdefn_p_->template for_each_object_within<FieldMappingType>
	    (poly, [&](const size_t& id) { id_list.push_back(id); }, true);

	  // ... but that polygon might be inside a multipolygon:
	} else if (gc.size() == 1 and
		   gc.at(0)->type() == Geometry::Type::multipolygon) {
	  const MultiPolygon& mpoly =
	    *std::dynamic_pointer_cast<MultiPolygon>(gc.at(0));
	  if (mpoly.size() == 1) {
	    // select_polygon(mp.at(0), inverted);
	    meshdefn_p_->template for_each_object_within<FieldMappingType>
	      (mpoly.at(0),
	       [&](const size_t& id) { id_list.push_back(id); }, true);
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
      } else {
	// The user wants to select cells/faces/vertices intersected
	// by the objects (not inverted)
	for (auto&& geom_ptr : gc) {
	  switch (geom_ptr->type()) {
	  case Geometry::Type::point: {
	    const Point& pt = *std::dynamic_pointer_cast<Point>(geom_ptr);
	    id_list.push_back(id_at_location(pt.as_2d_array()));
	    break;
	  }
	  case Geometry::Type::multipoint: {
	    auto mpt = std::dynamic_pointer_cast<MultiPoint>(geom_ptr);
	    for (auto&& pt : *mpt) {
	      id_list.push_back(id_at_location(pt.as_2d_array()));
	    }
	    break;
	  }
	  case Geometry::Type::polygon: {
	    const Polygon& poly = *std::dynamic_pointer_cast<Polygon>(geom_ptr);
	    meshdefn_p_->template for_each_object_within<FieldMappingType>
	      (poly,
	       [&](const size_t& id) { id_list.push_back(id); }, false);
	    break;
	  }
	  case Geometry::Type::multipolygon: {
	    for (auto&& poly :
		   *std::dynamic_pointer_cast<MultiPolygon>(geom_ptr)) {
	      meshdefn_p_->template for_each_object_within<FieldMappingType>
		(poly,
		 [&](const size_t& id) { id_list.push_back(id); }, false);
	    }
	    break;
	  }
	  default:
	    std::cerr << "Geometry of type " << geom_ptr->type_str()
		      << " is not supported." << std::endl;
	    throw std::runtime_error("Geometry type not supported.");
	  };
	}
      }
    } else {
      std::cerr << "Unknown selection method: " << sel_type_str
		<< std::endl;
      throw std::runtime_error("Unknown selection method.");
    }
    
    allocate_list(id_list);
  }

  ~MeshSelection(void)
  {}

  bool is_global(void) const
  {
    return (not bool(list_));
  }
  
  size_t size(void) const
  {
    if (list_) {
      return list_->size();
    } else {
      return meshdefn_p_->template object_count<FM>();
    }
  }

  std::shared_ptr<DataArray<size_t>> list_ptr(void) const { return list_; }
  
  using AccessMode = sycl::access::mode;
  using AccessTarget = sycl::access::target;
  using AccessPlaceholder = sycl::access::placeholder;

  template<AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer,
	   AccessPlaceholder IsPlaceholder = AccessPlaceholder::false_t>
  using Accessor = typename DataArray<size_t>::template Accessor<Mode, Target, IsPlaceholder>;

  Accessor<sycl::access::mode::read>
  get_read_accessor(sycl::handler& cgh) const
  {
    assert(list_);
    return list_->get_read_accessor(cgh);
  }

};

using MeshSelectionAccessor =
  typename DataArray<size_t>::
  template Accessor<sycl::access::mode::read>;


#endif
