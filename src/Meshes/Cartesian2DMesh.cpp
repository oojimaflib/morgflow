/***********************************************************************
 * Meshes/Cartesian2DMesh.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#include "Cartesian2DMesh.hpp"

Cartesian2DMesh::Cartesian2DMesh(const Config& conf)
{
  ncells_ = split_string<size_t, 2>(conf.get<std::string>("cell count"));
  origin_ = split_string<double, 2>(conf.get<std::string>("origin"));
  cell_size_ = split_string<double, 2>(conf.get<std::string>("cell size"));
}

template<>
size_t Cartesian2DMesh::get_nearest_object<FieldMapping::Cell>(const CoordType& loc) const
{
  double idx_d0 = (loc[0] - origin_[0]) / cell_size_[0];
  double idx_d1 = (loc[1] - origin_[1]) / cell_size_[1];
  IndexType idx {
    (idx_d0 < 0.0 ? 0 : idx_d0 < ncells_[0] * cell_size_[0] ? size_t(idx_d0) : ncells_[0] - 1),
    (idx_d1 < 0.0 ? 0 : idx_d1 < ncells_[1] * cell_size_[1] ? size_t(idx_d1) : ncells_[1] - 1),
  };
  if (idx[0] >= ncells_[0] or idx[1] >= ncells_[1]) {
    return object_count<FieldMapping::Cell>();
  }
  return get_cell_linear_id(idx);
}

template<>
void Cartesian2DMesh::for_each_object_within<FieldMapping::Cell>
(const Polygon& poly, std::function<void(const size_t&)> fn, bool inverted) const
{
  // Reject polygons with multiple rings
  if (poly.size() != 1) {
    throw std::runtime_error("Polygons with holes are not supported.");
  }
  
  const LineString& ls = poly.at(0);

  // Reject sliver polygons
  if (ls.size() < 3) {
    std::cerr << "Warning. Sliver polygon selects no cells" << std::endl;
    return;
  }

  for (size_t yi = 0; yi < ncells_[1]; ++yi) {
    std::vector<size_t> nodes_xi;
    size_t j = ls.size() - 1;
    for (size_t i = 0; i < ls.size(); ++i) {
      const Point& pti = ls.at(i);
      const Point& ptj = ls.at(j);
      double viy = (pti[1] - origin_[1]) / cell_size_[1];
      double vjy = (ptj[1] - origin_[1]) / cell_size_[1];

      if ((viy < (double) yi && vjy >= (double) yi) ||
	  (vjy < (double) yi && viy >= (double) yi)) {
	double vix = (pti[0] - origin_[0]) / cell_size_[0];
	double vjx = (ptj[0] - origin_[0]) / cell_size_[0];
	nodes_xi.push_back((size_t) (vix + (yi - viy) / (vjy - viy) * (vjx - vix)));
      }
      
      j = i;
    }

    if (nodes_xi.size() == 0) {
      if (inverted) {
	for (size_t xi = 0; xi < ncells_[0]; ++xi) {
	  fn(yi * ncells_[0] + xi);	  
	}
      }
      continue;
    }
    
    size_t i = 0;
    while (i < nodes_xi.size() - 1) {
      if (nodes_xi.at(i) > nodes_xi.at(i+1)) {
	std::swap(nodes_xi.at(i), nodes_xi.at(i+1));
      } else {
	++i;
      }
    }

    if (inverted) {
      for (size_t xi = 0; xi < ncells_[0]; ++xi) {
	bool within = false;
	for (size_t i = 0; i < nodes_xi.size(); i += 2) {
	  if (xi >= nodes_xi.at(i) && xi < nodes_xi.at(i+1)) {
	    within = true;
	    break;
	  }
	}
	if (within) {
	  continue;
	} else {
	  fn(yi * ncells_[0] + xi);
	}
      }
    } else {
      for (size_t i = 0; i < nodes_xi.size(); i += 2) {
	if (nodes_xi.at(i) >= ncells_[0]) break;
	if (nodes_xi.at(i+1) < ncells_[0]) {
	  if (nodes_xi.at(i) > ncells_[0]) {
	    nodes_xi.at(i) = 0;
	  }
	  if (nodes_xi.at(i+1) >= ncells_[0]) {
	    nodes_xi.at(i+1) = ncells_[0];
	  }
	  for (size_t xi = nodes_xi.at(i); xi < nodes_xi.at(i+1); ++xi) {
	    fn(yi * ncells_[0] + xi);
	  }
	}
      }
    }
  }
}

/*
template<>
void Cartesian2DMesh::for_each_object_within<FieldMapping::Cell>
(const Polygon& poly, std::function<void(const size_t&)> fn) const
{
  struct Edge
  {
    size_t xi;
    size_t yi0;
    long int wp_xmov;
    long int xdir;
    long int err;
    long int err_adj_up;
    long int err_adj_dn;
    size_t count;
  };

  // Reject polygons with multiple rings
  if (poly.size() != 1) {
    throw std::runtime_error("Polygons with holes are not supported.");
  }

  const LineString& ls = poly.at(0);

  // Reject sliver polygons
  if (ls.size() < 3) {
    std::cerr << "Warning. Sliver polygon selects no cells" << std::endl;
    return;
  }

  // Build the global edge table
  std::list<Edge> global_edge_table;
  // Scan through the vertex list and put each edge that has
  // non-zero height into the table
  for (size_t vi = 0; vi < ls.size(); ++vi) {
    CoordType loc0 = ls.at(vi).as_2d_array();
    CoordType loc1 = (vi == 0 ?
		      ls.back().as_2d_array() :
		      ls.at(vi - 1).as_2d_array());
	
    size_t i0 = get_nearest_object<FieldMapping::Cell>(loc0);
    size_t i1 = get_nearest_object<FieldMapping::Cell>(loc1);

    IndexType idx0 = get_cell_index(i0);
    IndexType idx1 = get_cell_index(i1);

    // Orient the edge so idx0[1] <= idx1[1]
    if (idx0[1] > idx1[1]) {
      std::swap(idx0, idx1);
      // std::swap(i0, i1);
      // std::swap(loc0, loc1);
    }

    long int dy = idx1[1] - idx0[1];
    if (dy != 0) { // skip this edge if it has no vertical height
      long int dx = idx1[0] - idx0[0];
      long int width = std::abs(dx);
      long int xdir = (dx > 0 ? 1 : -1);
      Edge e {
	.xi = idx0[0],
	.yi0 = idx0[1],
	.wp_xmov = ((dy >= width) ? 0 : xdir * width / dy),
	.xdir = xdir,
	.err = ((dx >= 0) ? 0 : 1 - dy),
	.err_adj_up = ((dy >= width) ? width : width % dy),
	.err_adj_dn = dy,
	.count = (size_t) dy
      };
      global_edge_table.push_front(e);
    }
  }

  // Sort the global edge table by yi0, xi
  global_edge_table.sort([] (const Edge& e0, const Edge& e1) -> bool
  {
    if (e0.yi0 == e1.yi0) return (e0.xi < e1.xi);
    return (e0.yi0 < e1.yi0);
  });

  std::list<Edge> active_edge_table;
  size_t yi = global_edge_table.front().yi0;
  while ((not global_edge_table.empty()) ||
	 (not active_edge_table.empty())) {
    // Stop only when no edges remain in either edge table

    // Edges that start at the current row should move from the
    // global to the active edge table.
    while ((not global_edge_table.empty()) and
	   global_edge_table.front().yi0 == yi) {
      active_edge_table.push_front(global_edge_table.front());
      global_edge_table.pop_front();
    }

    // Sort the active edge table by the current x-coordinate of
    // each edge
    active_edge_table.sort([] (const Edge& e0, const Edge& e1) -> bool
    {
      return (e0.xi < e1.xi);
    });

    size_t xi0 = 0;
    size_t xi1 = 0;
    auto ei = active_edge_table.begin();
    while (ei != active_edge_table.end()) {
      xi1 = ei->xi;
      if (++ei == active_edge_table.end()) break;

      xi0 = xi1;
      xi1 = ei->xi;

      for (size_t xi = xi0; xi < xi1; ++xi) {
	fn(get_cell_linear_id({xi, yi}));
      }
      //ei++ ??
    }

    // Remove edges from the active edge table that end at the
    // current row
    active_edge_table.remove_if([] (const Edge& e) -> bool {
      return e.count == 1;
    });

    std::for_each(active_edge_table.begin(),
		  active_edge_table.end(),
		  [] (Edge& e) {
		    e.count -= 1;
		    e.xi += e.wp_xmov;
		    if ((e.err += e.err_adj_up) > 0) {
		      e.xi += e.xdir;
		      e.err -= e.err_adj_dn;
		    }
		  });

    yi++;
  }
}
*/
/*
template<>
void Cartesian2DMesh::append_object_ids_within<FieldMapping::Cell>(const Polygon& poly,
								   std::vector<size_t>& id_list) const
{
  for_each_object_within<FieldMapping::Cell>(poly,
					     [&](const size_t& id) {
					       id_list.push_back(id);
					     });
}
*/

void Cartesian2DMesh::write_check_file(const stdfs::path& check_path,
				       const Config& config) const
{
  // Create sub-directory for mesh checks:
  stdfs::path mesh_path = check_path / "mesh";
  if (not stdfs::exists(mesh_path)) {
    try {
      stdfs::create_directory(mesh_path);
    } catch (stdfs::filesystem_error& err) {
      std::cerr << "Could not create check file directory: "
		<< mesh_path << std::endl;
      throw err;
    }
  } else if (not stdfs::is_directory(mesh_path)) {
    std::cerr << "Could not create check file directory over file: "
	      << mesh_path << std::endl;
    throw std::runtime_error("File exists.");
  }

  // Create mesh log file
  std::ofstream log(mesh_path / "log.txt");
  
  log << "Writing Cartesian 2D Mesh." << std::endl
      << "  Cells: " << ncells_[0] << " × " << ncells_[1]
      << " = " << object_count<FieldMapping::Cell>() << std::endl
      << "  Faces: " << object_count<FieldMapping::Face>() << std::endl
      << "  Vertices: " << object_count<FieldMapping::Vertex>() << std::endl;

  {  // Write cell, face and vertex locations
    stdfs::path cc_file = mesh_path / "cell_centres.csv";
    std::ofstream cc(cc_file);
    log << "Writing cell centres to " << cc_file << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Cell>(); ++i) {
      auto loc = get_object_coordinate<FieldMapping::Cell>(i);
      cc << loc[0] << "," << loc[1] << std::endl;
    }
    cc.close();
    stdfs::path fc_file = mesh_path / "face_centres.csv";
    std::ofstream fc(fc_file);
    log << "Writing face centres to " << fc_file << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Face>(); ++i) {
      auto loc = get_object_coordinate<FieldMapping::Face>(i);
      fc << loc[0] << "," << loc[1] << std::endl;
    }
    fc.close();
    stdfs::path vx_file = mesh_path / "vertices.csv";
    std::ofstream vx(vx_file);
    log << "Writing vertex locations to " << vx_file << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Vertex>(); ++i) {
      auto loc = get_object_coordinate<FieldMapping::Vertex>(i);
      vx << loc[0] << "," << loc[1] << std::endl;
    }
    vx.close();
  }
  
  {  // Write files with face and cell connectivity
    stdfs::path cc_file = mesh_path / "cell_connectivity.csv";
    std::ofstream cc(cc_file);
    log << "Writing cell connectivity to " << cc_file << std::endl;
    cc << "f_w,f_e,f_s,f_n,v_sw,v_se,v_nw,v_ne" << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Cell>(); ++i) {
      auto f_arr = get_faces_around_cell(get_cell_index(i));
      auto v_arr = get_vertices_around_cell(get_cell_index(i));
      cc << f_arr[0] << "," << f_arr[1] << ","
	 << f_arr[2] << "," << f_arr[3] << ","
	 << v_arr[0] << "," << v_arr[1] << ","
	 << v_arr[2] << "," << v_arr[3] << std::endl;
    }
    cc.close();
    stdfs::path fc_file = mesh_path / "face_connectivity.csv";
    std::ofstream fc(fc_file);
    log << "Writing face connectivity to " << fc_file << std::endl;
    fc << "c_us,c_ds,v_l,v_r" << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Face>(); ++i) {
      auto c_arr = get_cells_around_face(i);
      auto v_arr = get_vertices_around_face(i);
      fc << c_arr[0] << "," << c_arr[1] << ","
	 << v_arr[0] << "," << v_arr[1] << std::endl;
    }
    fc.close();
  }

  {  // Write files with vertex, face and cell geometry
    stdfs::path cg_file = mesh_path / "cell_geometry.csv";
    std::ofstream cg(cg_file);
    log << "Writing cell geometry to " << cg_file << std::endl;
    cg << "wkt,id" << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Cell>(); ++i) {
      cg << "\"" << get_cell_geometry(i).wkt() << "\"," << i << std::endl;
    }
    cg.close();
    stdfs::path fg_file = mesh_path / "face_geometry.csv";
    std::ofstream fg(fg_file);
    log << "Writing face geometry to " << fg_file << std::endl;
    fg << "wkt,id" << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Face>(); ++i) {
      fg << "\"" << get_face_geometry(i).wkt() << "\"," << i << std::endl;
    }
    fg.close();
    stdfs::path vg_file = mesh_path / "vertex_geometry.csv";
    std::ofstream vg(vg_file);
    log << "Writing vertex geometry to " << vg_file << std::endl;
    vg << "wkt,id" << std::endl;
    for (size_t i = 0; i < object_count<FieldMapping::Vertex>(); ++i) {
      vg << "\"" << get_vertex_geometry(i).wkt() << "\"," << i << std::endl;
    }
    vg.close();
  }
  
  // Close the log file
  log.close();
}
