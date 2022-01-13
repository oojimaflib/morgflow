/***********************************************************************
 * mf_parser/mf_parser_writer_settings.hpp
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

#ifndef mf_parser_writer_settings_hpp
#define mf_parser_writer_settings_hpp

#include <string>

namespace boost { namespace property_tree { namespace mf_parser
    {

      template <class Ch>
      class mf_writer_settings
      {
      public:
        mf_writer_settings(Ch indent_char = Ch(' '),
			   unsigned indent_count = 4):
	  indent_char(indent_char),
	  indent_count(indent_count)
	{
	}
	Ch indent_char;
	int indent_count;
      };
  
      template <class Ch>
      mf_writer_settings<Ch> mf_writer_make_settings(Ch indent_char = Ch(' '), unsigned indent_count = 4)
      {
        return mf_writer_settings<Ch>(indent_char, indent_count);
      }

} } }

#endif
