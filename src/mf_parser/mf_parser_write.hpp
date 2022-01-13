/***********************************************************************
 * mf_parser/mf_parser_read.hpp
 *
 * Parser to read "mf" format files into a boost::property_tree
 *
 * Copyright (C) Gerald C J Morgan 2020
 * Sections lifted extensively from boost/property_tree which is
 * Copyright (C) 2009 Sebastian Redl
 *
 * Distributed under the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or copy at 
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * For more information, see www.boost.org
 ***********************************************************************/

#ifndef mf_parser_write_hpp
#define mf_parser_write_hpp

#include <boost/property_tree/ptree.hpp>
#include "mf_parser_writer_settings.hpp"
#include "mf_parser_utils.hpp"
#include "mf_parser_config.hpp"
#include <string>

namespace boost { namespace property_tree { namespace mf_parser
{

    template<class Ch>
    void write_mf_indent(std::basic_ostream<Ch> &stream,
          int indent,
          const mf_writer_settings<Ch> &settings
          )
    {
        stream << std::basic_string<Ch>(indent * settings.indent_count, settings.indent_char);
    }
  
    // Create necessary escape sequences from illegal characters
    template<class Ch>
    std::basic_string<Ch> create_escapes(const std::basic_string<Ch> &s)
    {
        std::basic_string<Ch> result;
        typename std::basic_string<Ch>::const_iterator b = s.begin();
        typename std::basic_string<Ch>::const_iterator e = s.end();
        while (b != e)
        {
            if (*b == Ch('\0')) result += Ch('\\'), result += Ch('0');
            else if (*b == Ch('\a')) result += Ch('\\'), result += Ch('a');
            else if (*b == Ch('\b')) result += Ch('\\'), result += Ch('b');
            else if (*b == Ch('\f')) result += Ch('\\'), result += Ch('f');
            else if (*b == Ch('\n')) result += Ch('\\'), result += Ch('n');
            else if (*b == Ch('\r')) result += Ch('\\'), result += Ch('r');
            else if (*b == Ch('\v')) result += Ch('\\'), result += Ch('v');
            else if (*b == Ch('"')) result += Ch('\\'), result += Ch('"');
            else if (*b == Ch('\\')) result += Ch('\\'), result += Ch('\\');
            else
                result += *b;
            ++b;
        }
        return result;
    }

    template<class Ch>
    bool is_simple_key(const std::basic_string<Ch> &key)
    {
        const static std::basic_string<Ch> chars = convert_chtype<Ch, char>("\t{};\n\"");
        return !key.empty() && key.find_first_of(chars) == key.npos;
    }
    
    template<class Ch>
    bool is_simple_data(const std::basic_string<Ch> &data)
    {
        const static std::basic_string<Ch> chars = convert_chtype<Ch, char>("\t{};\n\"");
        return !data.empty() && data.find_first_of(chars) == data.npos;
    }

  
    template<class Ptree>
    void write_mf_helper(std::basic_ostream<typename Ptree::key_type::value_type> &stream, 
			 const Ptree &pt, 
			 int indent,
			 const mf_writer_settings<typename Ptree::key_type::value_type> &settings)
    {

      // Character type
      typedef typename Ptree::key_type::value_type Ch;
      
      // Write data
      if (indent >= 0)
        {
	  if (!pt.data().empty())
            {
	      std::basic_string<Ch> data = create_escapes(pt.template get_value<std::basic_string<Ch> >());
	      if (is_simple_data(data))
		stream << mf_keyword_sep<Ch> << mf_keyval_delimiter<Ch> << Ch(' ') << data << Ch('\n');
	      else
		stream << mf_keyword_sep<Ch> << mf_keyval_delimiter<Ch> << Ch(' ') << Ch('\"') << data << Ch('\"') << Ch('\n');
            }
	  //else if (pt.empty())
	  //  stream << Ch(' ') << Ch('\"') << Ch('\"') << Ch('\n');
	  else
	    stream << Ch('\n');
        }
        
      // Write keys
      if (!pt.empty())
        {
	  
	  // Open brace
	  if (indent >= 0)
            {
	      write_mf_indent( stream, indent, settings);
	      stream << Ch('{') << Ch('\n');
            }
            
	  // Write keys
	  typename Ptree::const_iterator it = pt.begin();
	  for (; it != pt.end(); ++it)
            {
	      
	      // Output key
	      std::basic_string<Ch> key = create_escapes(it->first);
	      write_mf_indent( stream, indent+1, settings);
	      if (is_simple_key(key))
		stream << key;
	      else
		stream << Ch('\"') << key << Ch('\"');
	      
	      // Output data and children  
	      write_mf_helper(stream, it->second, indent + 1, settings);
	      
            }
	  
	  // Close brace
	  if (indent >= 0)
            {
	      write_mf_indent( stream, indent, settings);
	      stream << Ch('}') << Ch('\n');
            }
	  
        }
    }

    // Write ptree to mf stream
    template<class Ptree>
    void write_mf_internal(std::basic_ostream<typename Ptree::key_type::value_type> &stream, 
			   const Ptree &pt,
			   const std::string &filename,
			   const mf_writer_settings<typename Ptree::key_type::value_type> &settings)
    {
      write_mf_helper(stream, pt, -1, settings);
      if (!stream.good())
	BOOST_PROPERTY_TREE_THROW(mf_parser_error("write error", filename, 0));
    }
  
  
    }}}

#endif
