/***********************************************************************
 * Meshes/Cartesian2DMesh.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef Meshes_Cartesian2DMesh_hpp
#define Meshes_Cartesian2DMesh_hpp

#include "../Mesh.hpp"

#include "../Config.hpp"
// #include "../Field.hpp"
#include "../Geometry.hpp"

class Cartesian2DMesh : public Mesh<std::array<size_t, 2>,
				    std::array<double, 2>>
{
public:

  typedef std::array<size_t,2> IndexType;
  typedef std::array<double,2> CoordType;
  
private:

  IndexType ncells_;
  CoordType origin_;
  CoordType cell_size_;

public:
  
  Cartesian2DMesh(const Config& conf);

  CoordType cell_size(void) const
  {
    return cell_size_;
  }
  
  size_t dimensionality(void) const
  {
    return 2;
  }
  
  // Return the total number of objects of a given type within the
  // mesh
  template<FieldMapping FM>
  inline size_t object_count(void) const;

  template<>
  inline size_t object_count<FieldMapping::Cell>(void) const
  {
    return ncells_[0] * ncells_[1];
  }
  
  template<>
  inline size_t object_count<FieldMapping::Face>(void) const
  {
    return (ncells_[0] + 1) * ncells_[1]
      + ncells_[0] * (ncells_[1] + 1);
  }

  template<>
  inline size_t object_count<FieldMapping::Vertex>(void) const
  {
    return (ncells_[0] + 1) * (ncells_[1] + 1);
  }

  // Get the ID of the object nearest to a given location
  template<FieldMapping FM>
  size_t get_nearest_object(const CoordType& loc) const;

  /*
  template<>
  size_t get_nearest_object<FieldMapping::Cell>(const CoordType& loc) const;
  */

  // Get the location of a given object
  template<FieldMapping FM>
  CoordType get_object_coordinate(const size_t& i) const;

  template<>
  CoordType
  get_object_coordinate<FieldMapping::Cell>(const size_t& i) const
  {
    return this->cell_centre(i);
  }
  
  template<>
  CoordType
  get_object_coordinate<FieldMapping::Face>(const size_t& i) const
  {
    return this->face_centre(i);
  }
  
  template<>
  CoordType
  get_object_coordinate<FieldMapping::Vertex>(const size_t& i) const
  {
    return this->vertex(i);
  }

  // Loop over a subset of the mesh defined by a polygon
  template<FieldMapping FM>
  void for_each_object_within(const Polygon& poly,
			      std::function<void(const size_t&)> fn,
			      bool inverted = false) const;

  /*
  template<FieldMapping FM>
  void append_object_ids_within(const Polygon& poly,
				std::vector<size_t>& id_list) const;
  */
  
  inline size_t cell_count(void) const
  {
    return this->object_count<FieldMapping::Cell>();
  }
  
  inline size_t face_count(void) const
  {
    return this->object_count<FieldMapping::Face>();
  }

  inline size_t vertex_count(void) const
  {
    return this->object_count<FieldMapping::Vertex>();
  }
  
  CoordType cell_centre(const IndexType& i) const
  {
    return { origin_[0] + ((i[0] + 0.5) * cell_size_[0]),
      origin_[1] + ((i[1] + 0.5) * cell_size_[1]) };
  }

  CoordType cell_centre(const size_t& cell_id) const
  {
    return this->cell_centre(this->get_cell_index(cell_id));
  }

  CoordType face_centre(const size_t& face_id) const
  {
    CoordType result;
    
    if (face_id < (ncells_[0] + 1) * ncells_[1]) {
      // Face is vertical and has cells to the left and right
      size_t fyid = face_id / (ncells_[0] + 1);
      size_t fxid = face_id % (ncells_[0] + 1);
      
      result = { origin_[0] + fxid * cell_size_[0],
	origin_[1] + (fyid + 0.5) * cell_size_[1] };
    } else {
      // Face is horizontal and has cells to the bottom and top
      size_t local_id = face_id - (ncells_[0] + 1) * ncells_[1];
      size_t fyid = local_id / ncells_[0];
      size_t fxid = local_id % ncells_[0];
      
      result = { origin_[0] + (fxid + 0.5) * cell_size_[0],
	origin_[1] + fyid * cell_size_[1] };
    }

    return result;
  }

  CoordType vertex(const IndexType& vi) const
  {
    return { origin_[0] + vi[0] * cell_size_[0],
      origin_[1] + vi[1] * cell_size_[1] };
  }

  CoordType vertex(const size_t& i) const
  {
    return this->vertex(this->get_vertex_index(i));
  }
  
  IndexType get_cell_index(const size_t& linear_id) const
  {
    return { linear_id % ncells_[0],
      linear_id / ncells_[0] };
  }

  size_t get_cell_linear_id(const IndexType& index) const
  {
    return index[1] * ncells_[0] + index[0];    
  }

  IndexType get_cell_index_size(void) const
  {
    return ncells_;
  }

  IndexType get_vertex_index(const size_t& linear_id) const
  {
    return { linear_id % (ncells_[0] + 1),
      linear_id / (ncells_[0] + 1) };
  }

  std::array<size_t, 2> get_cells_around_face(const size_t& face_id) const
  {
    std::array<size_t, 2> result;
    
    if (face_id < (ncells_[0] + 1) * ncells_[1]) {
      // Face is vertical and has cells to the left and right
      size_t fyid = face_id / (ncells_[0] + 1);
      size_t fxid = face_id % (ncells_[0] + 1);

      if (fxid < ncells_[0]) {
	result[1] = get_cell_linear_id({fxid, fyid});
	if (fxid > 0) {
	  // Somewhere in the middle of the row
	  result[0] = get_cell_linear_id({fxid - 1, fyid});
	} else{
	  // Left hand edge of mesh. No cell to our left.
	  result[0] = face_count();
	}
      } else {
	// Right hand edge of mesh. No cell to our right.
	result[0] = get_cell_linear_id({fxid - 1, fyid});
	result[1] = face_count();
      }
    } else {
      // Face is horizontal and has cells to the bottom and top
      size_t local_id = face_id - (ncells_[0] + 1) * ncells_[1];
      size_t fyid = local_id / ncells_[0];
      size_t fxid = local_id % ncells_[0];

      if (fyid < ncells_[1]) {
	result[1] = get_cell_linear_id({fxid, fyid});
	if (fyid > 0) {
	  // Somewhere in the middle of the column
	  result[0] = get_cell_linear_id({fxid, fyid - 1});
	} else {
	  // Bottom edge of mesh. No cell to our left
	  result[0] = face_count();
	}
      } else {
	result[0] = get_cell_linear_id({fxid, fyid - 1});
	result[1] = face_count();
      }
    }
    
    return result;
  }

  std::array<size_t, 2> get_vertices_around_face(const size_t& face_id) const
  {
    if (face_id < (ncells_[0] + 1) * ncells_[1]) {
      // Face is vertical and has vertices to the south and north
      return { face_id + (ncells_[0] + 1), face_id };
    } else {
      // Face is horizontal and has vertices to the west and east
      size_t local_id = face_id - (ncells_[0] + 1) * ncells_[1];
      size_t fyid = local_id / ncells_[0];
      size_t fxid = local_id % ncells_[0];
      return { fyid * (ncells_[0] + 1) + fxid,
	fyid * (ncells_[0] + 1) + fxid + 1 };
    }
  }
  
  std::array<size_t,4> get_faces_around_cell(const IndexType& cell_index) const
  {
    size_t w = cell_index[1] * (ncells_[0] + 1) + cell_index[0];
    size_t e = w + 1;
    size_t s = (ncells_[0] + 1) * ncells_[1] +
      cell_index[1] * ncells_[0] + cell_index[0];
    size_t n = s + ncells_[0];
      
    return { w, e, s, n };
  }

  std::array<size_t,4>
  get_vertices_around_cell(const IndexType& cell_index) const
  {
    size_t sw = cell_index[1] * (ncells_[0] + 1) + cell_index[0];
    size_t se = sw + 1;
    size_t nw = sw + (ncells_[0] + 1);
    size_t ne = nw + 1;
    return { sw, se, nw, ne };
  }
  
  Point get_vertex_geometry(const size_t& i) const
  {
    auto loc = get_object_coordinate<FieldMapping::Vertex>(i);
    return Point(loc[0], loc[1]);
  }

  LineString get_face_geometry(const size_t& i) const
  {
    auto v_arr = get_vertices_around_face(i);
    return LineString({
	get_vertex_geometry(v_arr[0]),
	get_vertex_geometry(v_arr[1])
      });
  }

  Polygon get_cell_geometry(const size_t& i) const
  {
    auto v_arr = get_vertices_around_cell(get_cell_index(i));
    return Polygon({ LineString({
	    get_vertex_geometry(v_arr[0]),
	    get_vertex_geometry(v_arr[2]),
	    get_vertex_geometry(v_arr[3]),
	    get_vertex_geometry(v_arr[1]),
	    get_vertex_geometry(v_arr[0])
	  }) });
  }

  template<FieldMapping FM>
  std::string get_object_wkt(const size_t& i) const;

  template<>
  std::string
  get_object_wkt<FieldMapping::Cell>(const size_t& i) const
  {
    return this->get_cell_geometry(i).wkt();
  }

  template<>
  std::string
  get_object_wkt<FieldMapping::Face>(const size_t& i) const
  {
    return this->get_face_geometry(i).wkt();
  }

  template<>
  std::string
  get_object_wkt<FieldMapping::Vertex>(const size_t& i) const
  {
    return this->get_vertex_geometry(i).wkt();
  }
  
  /*
  template<FieldMapping FM>
  CoordType get_object_vertex_list(const size_t& i,
				   const size_t& j) const;

  template<>
  CoordType get_object_vertex_list<FieldMapping::Cell>(const size_t& i,
						       const size_t& j) const
  {
    auto v_arr = get_vertices_around_cell(get_cell_index(i));
    switch (j) {
    case 1:
      return this->vertex(v_arr[2]);
    case 2:
      return this->vertex(v_arr[3]);
    case 3:
      return this->vertex(v_arr[1]);
    default:
      return this->vertex(v_arr[0]);
    };
  }
  
  template<>
  CoordType get_object_vertex_list<FieldMapping::Face>(const size_t& i,
						       const size_t& j) const
  {
    auto v_arr = get_vertices_around_face(i);
    switch (j) {
    case 1:
      return this->vertex(v_arr[1]);
    default:
      return this->vertex(v_arr[0]);
    };
  }

  template<>
  CoordType get_object_vertex_list<FieldMapping::Vertex>(const size_t& i,
							 const size_t& j) const
  {
    return this->vertex(i);
    (void) j;
  }
  */
  
  void write_check_file(const stdfs::path& check_path,
			const Config& config) const;
  
};

#endif
