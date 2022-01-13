/***********************************************************************
 * Mesh.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef Mesh_hpp
#define Mesh_hpp

#include <string>

enum class FieldMapping
  {
    Cell,
    Face,
    Vertex,
  };

template<typename IndexType,
	 typename CoordType>
class Mesh {
public:

  Mesh(void)
  {
  }

  size_t cell_count(void) const
  {
    return 0;
  }

  size_t face_count(void) const
  {
    return 0;
  }

  size_t vertex_count(void) const
  {
    return 0;
  }

  CoordType cell_centre(const IndexType& i) const
  {
    return CoordType();
  }
  
  CoordType face_centre(const IndexType& i) const
  {
    return CoordType();
  }
  
  CoordType vertex(const IndexType& i) const
  {
    return CoordType();
  }
  
};

#endif

