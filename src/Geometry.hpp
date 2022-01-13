/***********************************************************************
 * Geometry.hpp
 *
 * Vector Geometry classes
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef Geometry_hpp
#define Geometry_hpp

#include <string>
#include <iostream>
#include <utility>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/phoenix.hpp>
#include <boost/phoenix/object/dynamic_cast.hpp>
#include <boost/phoenix/operator/self.hpp>
#include <boost/optional.hpp>

#include "Config.hpp"

class Geometry
{
private:

  bool has_z_;
  bool has_m_;

protected:

  virtual std::string inner_text(void) const = 0;
  
public:

  enum class Type {
    point,
    multipoint,
    linestring,
    multilinestring,
    polygon,
    multipolygon,
    collection
  };
  
  Geometry(bool has_z = false, bool has_m = false)
    : has_z_(has_z), has_m_(has_m)
  {}
  virtual ~Geometry(void) {}
  
  virtual std::string type_str(void) const = 0;
  virtual Type type(void) const = 0;

  std::string wkt(void) const
  {
    std::string s = type_str() + " ( ";
    s += this->inner_text();
    s += " )";
    return s;
  }

  bool has_z(void) const { return has_z_; }
  bool has_m(void) const { return has_m_; }

  virtual void set_has_z(bool has_z)
  {
    has_z_ = has_z;
  }
  virtual void set_has_m(bool has_m)
  {
    has_m_ = has_m;
  }
};

class Point : public Geometry,
	      public std::vector<double>
{
public:

  Point(void)
    : Geometry(false, false),
      std::vector<double>({0.0,0.0})
  {
    // std::cout << "Calling default" << std::endl;
  }

  Point(const double& x,
	const double& y)
    : Geometry(false, false),
      std::vector<double>({x, y})
  {}

  Point(const double& x,
	const double& y,
	const double& z_or_m,
	bool xyz = true)
    : Geometry(xyz, !xyz),
      std::vector<double>({x, y, z_or_m})
  {}

  Point(const double& x,
	const double& y,
	const double& z,
	const double& m)
    : Geometry(true, true),
      std::vector<double>({x, y, z, m})
  {}

  Point(const std::vector<double>& vec)
    : Geometry(false, false),
      std::vector<double>(vec)
  {
    assert(std::vector<double>::size() >= 2);
  }

  Point(const std::vector<double>& vec,
	boost::optional<char> zflag,
	boost::optional<char> mflag)
    : Geometry(bool(zflag), bool(mflag)), std::vector<double>(vec)
  {
    std::cout << "Calling optional" << std::endl;
    if (Geometry::has_z()) std::cout << "Has Z" << std::endl;
    if (Geometry::has_m()) std::cout << "Has M" << std::endl;
      
    //     assert_correct_size();
  }

  /*
  Point(const double& x, const double& y)
    : Geometry(), std::vector<double>({x,y})
  {}
  */

  virtual ~Point(void) {}

  virtual std::string type_str(void) const
  {
    if (has_z() && has_m()) {
      return "Point ZM";
    } else if (has_z()) {
      return "Point Z";
    } else if (has_m()) {
      return "Point M";
    } else {
      return "Point";
    }
  }
  virtual Geometry::Type type(void) const { return Geometry::Type::point; }
  
  double x(void) const { return this->operator[](0); }
  double y(void) const { return this->operator[](1); }
  double z(void) const
  {
    if (has_z()) return this->operator[](2);
    return std::numeric_limits<double>::quiet_NaN();
  }
  double m(void) const
  {
    if (has_m()) {
      if (has_z()) return this->operator[](3);
      return this->operator[](2);
    }
    return std::numeric_limits<double>::quiet_NaN();
  }
  
  void assert_correct_size(void) const
  {
    assert(std::vector<double>::size() == (2 + (has_z()?1:0) + (has_m()?1:0)));
  }
  
  virtual std::string inner_text(void) const
  {
    std::string s = std::to_string(this->x())
      + " " + std::to_string(this->y());
    if (has_z()) s += " " + std::to_string(this->z());
    if (has_m()) s += " " + std::to_string(this->m());
    return s;
  }

  std::array<double, 2> as_2d_array(void) const
  {
    return std::array<double, 2>({ x(), y() });
  }
  
};

class MultiPoint : public Geometry,
		   public std::vector<Point>
{
public:
  
  MultiPoint(void)
    : Geometry(), std::vector<Point>()
  {}
  MultiPoint(const std::vector<Point>& vec)
    : Geometry(), std::vector<Point>(vec)
  {}
  MultiPoint(const std::vector<Point>& vec,
	     boost::optional<char> zflag,
	     boost::optional<char> mflag)
    : Geometry(bool(zflag), bool(mflag)), std::vector<Point>(vec)
  {
    std::cout << "Calling MP optional" << std::endl;
    if (Geometry::has_z()) std::cout << "Has Z" << std::endl;
    if (Geometry::has_m()) std::cout << "Has M" << std::endl;
    
    for (auto&& p : *this) {
      p.set_has_z(Geometry::has_z());
      p.set_has_m(Geometry::has_m());
      // p.assert_correct_size();
    }
  }


  virtual ~MultiPoint(void) {}

  virtual std::string type_str(void) const { return "MultiPoint"; }
  virtual Geometry::Type type(void) const { return Geometry::Type::multipoint; }

  virtual std::string inner_text(void) const
  {
    std::string s = "";
    bool first = true;
    for (auto&& p : *this) {
      if (first) {
	first = false;
      } else {
	s += ", ";
      }
      s += "( " + p.inner_text() + " )";
    }
    return s;
  }
  
};

class LineString : public MultiPoint
{
public:

  LineString(void)
    : MultiPoint()
  {}
  LineString(const MultiPoint& mp)
    : MultiPoint(mp)
  {}
  LineString(const std::vector<Point>& vec)
    : MultiPoint(vec)
  {}
  LineString(const std::vector<Point>& vec,
	     boost::optional<char> zflag,
	     boost::optional<char> mflag)
    : MultiPoint(vec, zflag, mflag)
  {}

  virtual ~LineString(void) {}

  virtual std::string type_str(void) const { return "LineString"; }
  virtual Geometry::Type type(void) const { return Geometry::Type::linestring; }

  virtual std::string inner_text(void) const
  {
    std::string s = "";
    bool first = true;
    for (auto&& p : *this) {
      if (first) {
	first = false;
      } else {
	s += ", ";
      }
      s += p.inner_text();
    }
    return s;
  }  

};

class MultiLineString : public Geometry,
			public std::vector<LineString>
{
public:
  
  MultiLineString(void)
    : Geometry(), std::vector<LineString>()
  {}
  MultiLineString(const std::vector<LineString>& vec)
    : Geometry(), std::vector<LineString>(vec)
  {}
  MultiLineString(const std::vector<LineString>& vec,
		  boost::optional<char> zflag,
		  boost::optional<char> mflag)
    : Geometry(bool(zflag), bool(mflag)), std::vector<LineString>(vec)
  {
    for (auto&& ls : *this) {
      ls.set_has_z(Geometry::has_z());
      ls.set_has_m(Geometry::has_m());
      //ls.assert_correct_size();
    }
  }

  virtual ~MultiLineString(void) {}

  virtual std::string type_str(void) const { return "MultiLineString"; }
  virtual Geometry::Type type(void) const { return Geometry::Type::multilinestring; }

  virtual std::string inner_text(void) const
  {
    std::string s = "";
    bool first = true;
    for (auto&& ls : *this) {
      if (first) {
	first = false;
      } else {
	s += ", ";
      }
      s += "( " + ls.inner_text() + " )";
    }
    return s;
  }
  
};

class Polygon : public MultiLineString
{
public:

  Polygon(void)
    : MultiLineString()
  {}
  Polygon(const MultiLineString& mls)
    : MultiLineString(mls)
  {}
  Polygon(const std::vector<LineString>& vec)
    : MultiLineString(vec)
  {}
  Polygon(const std::vector<LineString>& vec,
	  boost::optional<char> zflag,
	  boost::optional<char> mflag)
    : MultiLineString(vec, bool(zflag), bool(mflag))
  {
    for (auto&& ls : *this) {
      ls.set_has_z(Geometry::has_z());
      ls.set_has_m(Geometry::has_m());
      //ls.assert_correct_size();
    }
  }

  virtual ~Polygon(void) {}

  virtual std::string type_str(void) const { return "Polygon"; }
  virtual Geometry::Type type(void) const { return Geometry::Type::polygon; }
  
};
  
class MultiPolygon : public Geometry,
		     public std::vector<Polygon>
{
public:
  
  MultiPolygon(void)
    : Geometry(), std::vector<Polygon>()
  {}
  MultiPolygon(const std::vector<Polygon>& vec)
    : Geometry(), std::vector<Polygon>(vec)
  {}
  MultiPolygon(const std::vector<Polygon>& vec,
	       boost::optional<char> zflag,
	       boost::optional<char> mflag)
    : Geometry(bool(zflag), bool(mflag)), std::vector<Polygon>(vec)
  {
    for (auto&& pg : *this) {
      pg.set_has_z(Geometry::has_z());
      pg.set_has_m(Geometry::has_m());
    }
  }

  virtual ~MultiPolygon(void) {}

  virtual std::string type_str(void) const { return "MultiPolygon"; }
  virtual Geometry::Type type(void) const { return Geometry::Type::multipolygon; }
   
  virtual std::string inner_text(void) const { return "Not implemented"; }  

};

class GeometryCollection;

void read_wkt_geometry(const std::string& str, GeometryCollection& gc);
void read_gdal_geometry(const Config& config, GeometryCollection& gc);

class GeometryCollection : public Geometry,
			   public std::vector<std::shared_ptr<Geometry>>
{
public:

  GeometryCollection(void)
    : Geometry(), std::vector<std::shared_ptr<Geometry>>()
  {}
  GeometryCollection(const std::vector<std::shared_ptr<Geometry>>& vec)
    : Geometry(), std::vector<std::shared_ptr<Geometry>>(vec)
  {}

  GeometryCollection(const Config& config)
    : Geometry(), std::vector<std::shared_ptr<Geometry>>()
  {
    if (config.count("wkt") == 1) {
      read_wkt_geometry(config.get<std::string>("wkt"), *this);
    } else if (config.count("source") == 1) {
      read_gdal_geometry(config, *this);
    }
  }
  
  virtual ~GeometryCollection(void) {}
  
  virtual std::string type_str(void) const { return "GeometryCollection"; }
  virtual Geometry::Type type(void) const { return Geometry::Type::collection; }

  virtual std::string inner_text(void) const { return "Not implemented"; }  

};

namespace {
  template <typename T>
  struct make_shared_f
  {
    template <typename... A> struct result 
    { typedef std::shared_ptr<T> type; };
    
    template <typename... A>
    typename result<A...>::type operator()(A&&... a) const {
      return std::make_shared<T>(std::forward<A>(a)...);
    }
  };
  
  template <typename T>
  using make_shared_ = boost::phoenix::function<make_shared_f<T> >;
}

typedef std::shared_ptr<Point> PointPtr;

namespace wkt_parser {
  namespace qi = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;
  namespace phx = boost::phoenix;

  template<typename It>
  struct WKT : qi::grammar<It, GeometryCollection()>
  {
    qi::rule<It, Point()> point;
    qi::rule<It, Point()> point_text;
    qi::rule<It, Point()> point_tagged_text;
    qi::rule<It, std::shared_ptr<Point>()> point_ptr;

    qi::rule<It, MultiPoint()> multipoint;
    qi::rule<It, MultiPoint()> multipoint_text;
    qi::rule<It, MultiPoint()> multipoint_tagged_text;
    qi::rule<It, std::shared_ptr<MultiPoint>()> multipoint_ptr;
    
    qi::rule<It, LineString()> linestring;
    qi::rule<It, LineString()> linestring_text;
    qi::rule<It, LineString()> linestring_tagged_text;
    qi::rule<It, std::shared_ptr<LineString>()> linestring_ptr;
    
    qi::rule<It, MultiLineString()> multilinestring;
    qi::rule<It, MultiLineString()> multilinestring_text;
    qi::rule<It, MultiLineString()> multilinestring_tagged_text;
    qi::rule<It, std::shared_ptr<MultiLineString>()> multilinestring_ptr;

    qi::rule<It, Polygon()> polygon;
    qi::rule<It, Polygon()> polygon_text;
    qi::rule<It, Polygon()> polygon_tagged_text;
    qi::rule<It, std::shared_ptr<Polygon>()> polygon_ptr;
    
    qi::rule<It, MultiPolygon()> multipolygon;
    qi::rule<It, MultiPolygon()> multipolygon_text;
    qi::rule<It, MultiPolygon()> multipolygon_tagged_text;
    qi::rule<It, std::shared_ptr<MultiPolygon>()> multipolygon_ptr;

    qi::rule<It, std::shared_ptr<Geometry>> geometry_tagged_text;
    qi::rule<It, GeometryCollection()> start;

    WKT(void)
      : WKT::base_type(start)
    {
      point = (qi::double_ % qi::omit[+(qi::space)])[qi::_val = qi::_1];
      point_text %= (qi::lit('(') >>
		    qi::omit[*qi::space] >>
		    point >>
		    qi::omit[*qi::space] >>
		    qi::lit(')'));
      point_tagged_text = (ascii::no_case[qi::lit("point")] >>
			   -(qi::omit[*qi::space] >>
			     ascii::no_case[ascii::char_("z")]) >>
			   -(qi::omit[*qi::space] >>
			     ascii::no_case[ascii::char_("m")]) >>
			   qi::omit[*qi::space] >>
			   point_text)[qi::_val = phx::construct<Point>(qi::_3, qi::_1, qi::_2)];
      point_ptr = (point_tagged_text)[qi::_val = make_shared_<Point>()(qi::_1)];

      multipoint = (point_text % (*qi::space >>
				  ',' >>
				  *qi::space))[qi::_val = qi::_1];
      multipoint_text %= (qi::lit('(') >>
			  qi::omit[*qi::space] >>
			  multipoint >>
			  qi::omit[*qi::space] >>
			  qi::lit(')'));
      multipoint_tagged_text = (ascii::no_case[qi::lit("multipoint")] >>
				-(qi::omit[*qi::space] >>
				  ascii::no_case[ascii::char_("z")]) >>
				-(qi::omit[*qi::space] >>
				  ascii::no_case[ascii::char_("m")]) >>
				qi::omit[*qi::space] >>
				multipoint_text)
	[qi::_val = phx::construct<MultiPoint>(qi::_3, qi::_1, qi::_2)];
      multipoint_ptr = (multipoint_tagged_text)
	[qi::_val = make_shared_<MultiPoint>()(qi::_1)];
      
      linestring = (point % (*qi::space >>
			     ',' >>
			     *qi::space))[qi::_val = qi::_1];
      linestring_text %= (qi::lit('(') >>
			  qi::omit[*qi::space] >>
			  linestring >>
			  qi::omit[*qi::space] >>
			  qi::lit(')'));
      // linestring_tagged_text = (ascii::no_case[qi::lit("linestring")] >>
      // 				qi::omit[*qi::space] >>
      // 				linestring_text);
      linestring_tagged_text = (ascii::no_case[qi::lit("linestring")] >>
				-(qi::omit[*qi::space] >>
				  ascii::no_case[ascii::char_("z")]) >>
				-(qi::omit[*qi::space] >>
				  ascii::no_case[ascii::char_("m")]) >>
				qi::omit[*qi::space] >>
				linestring_text)
	[qi::_val = phx::construct<LineString>(qi::_3, qi::_1, qi::_2)];
      linestring_ptr = (linestring_tagged_text)
	[qi::_val = make_shared_<LineString>()(qi::_1)];

      multilinestring = (linestring_text % (*qi::space >>
					    ',' >>
					    *qi::space))[qi::_val = qi::_1];
      multilinestring_text %= (qi::lit('(') >>
			       qi::omit[*qi::space] >>
			       multilinestring >>
			       qi::omit[*qi::space] >>
			       qi::lit(')'));
      // multilinestring_tagged_text =
      // 	(ascii::no_case[qi::lit("multilinestring")] >>
      // 	 qi::omit[*qi::space] >>
      // 	 multilinestring_text);
      multilinestring_tagged_text =
	(ascii::no_case[qi::lit("multilinestring")] >>
	 -(qi::omit[*qi::space] >>
	   ascii::no_case[ascii::char_("z")]) >>
	 -(qi::omit[*qi::space] >>
	   ascii::no_case[ascii::char_("m")]) >>
	 qi::omit[*qi::space] >>
	 multilinestring_text)
	[qi::_val = phx::construct<MultiLineString>(qi::_3, qi::_1, qi::_2)];
      multilinestring_ptr = (multilinestring_tagged_text)
	[qi::_val = make_shared_<MultiLineString>()(qi::_1)];

      polygon = multilinestring;
      polygon_text = multilinestring_text;
      // polygon_tagged_text =
      // 	(ascii::no_case[qi::lit("polygon")] >>
      // 	 qi::omit[*qi::space] >>
      // 	 multilinestring_text);
      polygon_tagged_text =
	(ascii::no_case[qi::lit("polygon")] >>
	 -(qi::omit[*qi::space] >>
	   ascii::no_case[ascii::char_("z")]) >>
	 -(qi::omit[*qi::space] >>
	   ascii::no_case[ascii::char_("m")]) >>
	 qi::omit[*qi::space] >>
	 polygon_text)
	[qi::_val = phx::construct<Polygon>(qi::_3, qi::_1, qi::_2)];
      polygon_ptr = (polygon_tagged_text)
	[qi::_val = make_shared_<Polygon>()(qi::_1)];
      
      multipolygon = (polygon_text % (*qi::space >>
				      ',' >>
				      *qi::space))[qi::_val = qi::_1];
      multipolygon_text %= (qi::lit('(') >>
			    qi::omit[*qi::space] >>
			    multipolygon >>
			    qi::omit[*qi::space] >>
			    qi::lit(')'));
      // multipolygon_tagged_text =
      // 	(ascii::no_case[qi::lit("multipolygon")] >>
      // 	 qi::omit[*qi::space] >>
      // 	 multipolygon_text);
      multipolygon_tagged_text =
	(ascii::no_case[qi::lit("multipolygon")] >>
	 -(qi::omit[*qi::space] >>
	   ascii::no_case[ascii::char_("z")]) >>
	 -(qi::omit[*qi::space] >>
	   ascii::no_case[ascii::char_("m")]) >>
	 qi::omit[*qi::space] >>
	 multipolygon_text)
	[qi::_val = phx::construct<MultiPolygon>(qi::_3, qi::_1, qi::_2)];
      multipolygon_ptr = (multipolygon_tagged_text)
	[qi::_val = make_shared_<MultiPolygon>()(qi::_1)];

      geometry_tagged_text =
	point_ptr | multipoint_ptr |
	linestring_ptr | multilinestring_ptr |
	polygon_ptr | multipolygon_ptr;
      
      start %= geometry_tagged_text % (*qi::space >> ',' >> *qi::space);
    }
    
  };

};

void read_wkt_geometry(const std::string& str, GeometryCollection& gc);

void read_gdal_geometry(const Config& config, GeometryCollection& gc);

#endif
