/***********************************************************************
 * mf_parser/mf_parser_error.hpp
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

#ifndef mf_parser_error_hpp
#define mf_parser_error_hpp

#include <boost/property_tree/detail/file_parser_error.hpp>
#include <string>

namespace boost { namespace property_tree { namespace mf_parser
{

    class mf_parser_error: public file_parser_error
    {
    public:
        mf_parser_error(const std::string &message,
			const std::string &filename,
			unsigned long line) :
            file_parser_error(message, filename, line)
        {
        }
    };

} } }

#endif
