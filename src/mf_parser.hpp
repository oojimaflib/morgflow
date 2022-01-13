/***********************************************************************
 * mf_parser.hpp
 *
 * Parser to read "mf" format files into a boost::property_tree
 *
 * Copyright (C) Gerald C J Morgan 2020.
 *
 * Sections lifted extensively from boost/property_tree which is
 * Copyright (C) 2009 Sebastian Redl
 *
 * Distributed under the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or copy at 
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * For more information, see www.boost.org
 ***********************************************************************/

#ifndef mf_parser_hpp
#define mf_parser_hpp

#include <boost/property_tree/ptree.hpp>
#include "mf_parser/mf_parser_error.hpp"
#include "mf_parser/mf_parser_writer_settings.hpp"
#include "mf_parser/mf_parser_read.hpp"
#include "mf_parser/mf_parser_write.hpp"

#include <istream>
#include <ostream>
#include <vector>
#include <locale>

template<typename Ch, typename Traits>
inline std::basic_string<Ch, Traits> to_lower(const std::basic_string<Ch, Traits>& str,
					      const std::locale& loc)
{
  std::basic_string<Ch, Traits> result;
  for (auto&& c : str) {
    result += std::tolower(c, loc);
  }
  return result;
}

template<typename Ch, typename Traits, typename T, typename U>
inline void write_pair(std::basic_ostream<Ch,Traits>& stream,
		       const std::pair<T, U>& p)
{
  stream << Ch('(') << Ch(' ') << p.first << Ch(' ') << p.second << Ch(' ') << Ch(')');
}

template<typename Ch, typename Traits, typename T, typename U>
inline std::basic_ostream<Ch, Traits>& operator<<(std::basic_ostream<Ch,Traits>& stream,
						  const std::pair<T, U>& p)
{
  write_pair(stream, p);
  return stream;
}

template<typename Ch, typename Traits, typename T, typename U>
inline void read_pair(std::basic_istream<Ch, Traits>& stream,
		      std::pair<T, U>& p)
{
  Ch paren;
  if ((stream >> paren) && paren == Ch('(')) {
    T v1;
    U v2;
    stream >> v1 >> v2;
    if ((stream >> paren) && paren == Ch(')')) {
      p.first = v1;
      p.second = v2;
      return;
    }
  }
  stream.setstate(std::ios::badbit);
}

template<typename Ch, typename Traits, typename T, typename U>
inline std::basic_istream<Ch, Traits>& operator>>(std::basic_istream<Ch, Traits>& stream,
						  std::pair<T, U>& p)
{
  read_pair(stream, p);
  return stream;
}

template<typename Ch, typename Traits, typename T>
inline void write_vec(std::basic_ostream<Ch,Traits>& stream,
		      const std::vector<T>& vec)
{
  stream << Ch('[');
  for (const T v : vec) {
    stream << Ch(' ') << v;
  }
  stream << Ch(' ') << Ch(']');
}

template<typename Ch, typename Traits, typename T>
inline std::basic_ostream<Ch, Traits>& operator<<(std::basic_ostream<Ch,Traits>& stream,
						  const std::vector<T>& vec)
{
  write_vec(stream, vec);
  return stream;
}

template<typename Ch, typename Traits, typename T>
inline void read_vec(std::basic_istream<Ch, Traits>& stream,
		     std::vector<T>& vec)
{
  Ch open_brace;
  if ((stream >> open_brace) && (open_brace == Ch('['))) {
    std::basic_string<Ch, Traits> value_str;
    while (stream >> value_str) {
      if (value_str.size() > 0 && value_str.find_first_of(Ch(']')) == std::string::npos) {
	T value;
	std::basic_istringstream<Ch, Traits> iss(value_str);
	if (iss >> value) {
	  vec.push_back(value);
	}
      } else if (value_str.size() > 1) {
	value_str = value_str.substr(0, value_str.find_first_of(Ch(']')));
	T value;
	std::basic_istringstream<Ch, Traits> iss(value_str);
	if (iss >> value) {
	  vec.push_back(value);
	}
	return;
      } else if (value_str.size() == 1 && value_str.at(0) == Ch(']')) {
	return;
      }
    }
  }
  stream.setstate(std::ios::badbit);
}

template<typename Ch, typename Traits, typename T>
inline std::basic_istream<Ch, Traits>& operator>>(std::basic_istream<Ch, Traits>& stream,
						  std::vector<T>& vec)
{
  read_vec(stream, vec);
  return stream;
}


namespace boost {
  namespace property_tree {

    template<typename Ch, typename Traits, typename Alloc, typename T>
    struct vector_stream_translator
    {
      typedef std::basic_string<Ch, Traits, Alloc> internal_type;
      typedef std::vector<T> external_type;

      explicit vector_stream_translator(std::locale loc = std::locale())
	: m_loc(loc)
      {}

      boost::optional<external_type> get_value(const internal_type &v) {
	std::basic_istringstream<Ch, Traits, Alloc> iss(v);
	iss.imbue(m_loc);
	std::vector<T> vec;
	read_vec(iss, vec);
	if(iss.fail() || iss.bad() || iss.get() != Traits::eof()) {
	  return boost::optional< std::vector<T> >();
	}
	return vec;
      }
      boost::optional<internal_type> put_value(const external_type &vec) {
	std::basic_ostringstream<Ch, Traits, Alloc> oss;
	oss.imbue(m_loc);
	write_vec(oss, vec);
	if(oss) {
	  return oss.str();
	}
	return boost::optional<internal_type>();
      }

    private:
      std::locale m_loc;
    };
    
    template<typename Ch, typename Traits, typename Alloc, typename T>
    struct translator_between< std::basic_string<Ch,Traits,Alloc>, std::vector<T> >
    {
      typedef vector_stream_translator<Ch, Traits, Alloc, T > type;
    };

    template<typename Ch, typename Traits, typename Alloc, typename T, typename U>
    struct pair_stream_translator
    {
      typedef std::basic_string<Ch, Traits, Alloc> internal_type;
      typedef std::pair<T, U> external_type;

      explicit pair_stream_translator(std::locale loc = std::locale())
	: m_loc(loc)
      {}

      boost::optional<external_type> get_value(const internal_type &v) {
	std::basic_istringstream<Ch, Traits, Alloc> iss(v);
	iss.imbue(m_loc);
	std::pair<T, U> p;
	read_pair(iss, p);
	if(iss.fail() || iss.bad() || iss.get() != Traits::eof()) {
	  return boost::optional< std::pair<T, U> >();
	}
	return p;
      }
      boost::optional<internal_type> put_value(const external_type &p) {
	std::basic_ostringstream<Ch, Traits, Alloc> oss;
	oss.imbue(m_loc);
	write_pair(oss, p);
	if(oss) {
	  return oss.str();
	}
	return boost::optional<internal_type>();
      }

    private:
      std::locale m_loc;
    };
    
    template<typename Ch, typename Traits, typename Alloc, typename T, typename U>
    struct translator_between< std::basic_string<Ch,Traits,Alloc>, std::pair<T, U> >
    {
      typedef pair_stream_translator<Ch, Traits, Alloc, T, U> type;
    };

    namespace mf_parser {

      /**
       * Read MF from the given stream and translate it to a property
       * tree.  
       * @note Replaces the existing contents. String exception
       * guarantee.
       * @throw mf_parser_error If the stream cannot be read, doesn't 
       * contain valid MF data, or a conversion fails.
       */
      template<class Ptree, class Ch>
      void read_mf(std::basic_istream<Ch>& stream, Ptree& pt)
      {
	Ptree local;
	read_mf_internal(stream, local, std::string(), 0);
	pt.swap(local);
      }

      /**
       * Read MF from the given stream and translate it to a property
       * tree.  
       * @note Replaces the existing contents. String exception
       * guarantee.
       * @param default_ptree If parsing fails, pt is set to a copy of 
       * this ptree.
       */
      template<class Ptree, class Ch>
      void read_mf(std::basic_istream<Ch>& stream, Ptree& pt,
                   const Ptree &default_ptree)
      {
	try {
	  read_mf(stream, pt);
	} catch (file_parser_error&) {
	  pt = default_ptree;
	}
      }

      

      /**
       * Read MF from a the given file and translate it to a property tree. The
       * tree's key type must be a string type, i.e. it must have a nested
       * value_type typedef that is a valid parameter for basic_ifstream.
       * @note Replaces the existing contents. Strong exception guarantee.
       * @throw mf_parser_error If the file cannot be read, doesn't contain
       *                          valid MF, or a conversion fails.
       */
      template<class Ptree>
      void read_mf(const std::string &filename, Ptree &pt,
		   const std::locale &loc = std::locale())
      {
        std::basic_ifstream<typename Ptree::key_type::value_type>
	  stream(filename.c_str());
        if (!stream) {
	  BOOST_PROPERTY_TREE_THROW(mf_parser_error(
						    "cannot open file for reading", filename, 0));
        }
        stream.imbue(loc);
        Ptree local;
        read_mf_internal(stream, local, filename, 0);
        pt.swap(local);
      }

      
      /**
       * Read MF from a the given file and translate it to a property tree. The
       * tree's key type must be a string type, i.e. it must have a nested
       * value_type typedef that is a valid parameter for basic_ifstream.
       * @note Replaces the existing contents. Strong exception guarantee.
       * @param default_ptree If parsing fails, pt is set to a copy of this tree.
       */
      template<class Ptree>
      void read_mf(const std::string &filename,
		   Ptree &pt,
		   const Ptree &default_ptree,
		   const std::locale &loc = std::locale())
      {
	try {
	  read_mf(filename, pt, loc);
	} catch(file_parser_error &) {
	  pt = default_ptree;
	}
      }
      
      /**
       * Writes a tree to the stream in MF format.
       * @throw mf_parser_error If the stream cannot be written to, or a
       *                          conversion fails.
       * @param settings The settings to use when writing the MF data.
       */
      template<class Ptree, class Ch>
      void write_mf(std::basic_ostream<Ch> &stream,
		    const Ptree &pt,
		    const mf_writer_settings<Ch> &settings =
		    mf_writer_settings<Ch>())
      {
        write_mf_internal(stream, pt, std::string(), settings);
      }
      
      /**
       * Writes a tree to the file in MF format. The tree's key type must be a
       * string type, i.e. it must have a nested value_type typedef that is a
       * valid parameter for basic_ofstream.
       * @throw mf_parser_error If the file cannot be written to, or a
       *                          conversion fails.
       * @param settings The settings to use when writing the MF data.
       */
      template<class Ptree>
      void write_mf(const std::string &filename,
                    const Ptree &pt,
                    const std::locale &loc = std::locale(),
                    const mf_writer_settings<
		    typename Ptree::key_type::value_type
                    > &settings =
		    mf_writer_make_settings<
		    typename Ptree::key_type::value_type>())
      {
        std::basic_ofstream<typename Ptree::key_type::value_type>
	  stream(filename.c_str());
        if (!stream) {
	  BOOST_PROPERTY_TREE_THROW(mf_parser_error(
						    "cannot open file for writing", filename, 0));
        }
        stream.imbue(loc);
        write_mf_internal(stream, pt, filename, settings);
      }

    }
  }
}

namespace boost { namespace property_tree
{
    using mf_parser::mf_parser_error;
    using mf_parser::read_mf;
    using mf_parser::write_mf;
    using mf_parser::mf_writer_settings;
    using mf_parser::mf_writer_make_settings;
} }


#endif

