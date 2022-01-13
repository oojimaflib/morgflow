/***********************************************************************
 * mf_parser/mf_parser_config.hpp
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

#ifndef mf_parser_config_hpp
#define mf_parser_config_hpp

#include <boost/property_tree/ptree.hpp>

namespace boost { namespace property_tree { namespace mf_parser
{
  const char mf_comment_char = '!';

  template<typename Ch>
  const std::basic_string<Ch> mf_keyword_sep = " ";

  template<typename Ch>
  const std::basic_string<Ch> mf_keyval_delimiter = "==";
  
    }}}

#endif
