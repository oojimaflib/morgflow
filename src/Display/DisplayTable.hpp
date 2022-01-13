/***********************************************************************
 * DisplayTable.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef DisplayTable_hpp
#define DisplayTable_hpp

#include <boost/format.hpp>
using boost::format;

#include <iomanip>

template<typename ... Types>
class DisplayTable
{
public:

  struct Column
  {
    size_t width;
    std::string heading;
    std::string format_type;
    format bff;
    size_t padding;

    Column(const size_t& width_,
	   const std::string& heading_,
	   const std::string& format_type_)
      : width(width_),
	heading(heading_),
	format_type(format_type_),
	bff(format_type_)
    {
      padding = width;
      for (char c : heading) {
	if ((c & 0x80) == 0 ||
	    (c & 0xc0) == 0xc0) {
	  padding--;
	}
      }
      bff.modify_item(1, std::setw(width));
    }
    
  };

private:

  std::vector<Column> cols_;
  
  std::string top_rule_;
  std::string head_row_;
  std::string mid_rule_;
  format data_row_;
  std::string bot_rule_;

  bool table_open_;
  
public:

  DisplayTable(std::initializer_list<Column> cols)
    : cols_(cols)
  {
    assert(cols_.size() == sizeof...(Types));

    std::string data_fmt_;
    top_rule_ = "╭";
    head_row_ = "│";
    mid_rule_ = "├";
    data_fmt_ = "│";
    bot_rule_ = "╰";
    bool first = true;
    for (auto&& col : cols_) {
      if (!first) {
	top_rule_ += "┬";
	head_row_ += "│";
	mid_rule_ += "┼";
	data_fmt_ += "│";
	bot_rule_ += "┴";
      } else {
	first = false;
      }
      for (size_t i = 0; i < col.width; ++i) {
	top_rule_ += "─";
	mid_rule_ += "─";
	bot_rule_ += "─";
      }
      for (size_t i = 0; i < col.padding; ++i) {
	head_row_ += " ";
      }
      head_row_ += col.heading;
      data_fmt_ += col.format_type;
    }
    top_rule_ += "╮";
    head_row_ += "│";
    mid_rule_ += "┤";
    data_fmt_ += "│";
    bot_rule_ += "╯";

    data_row_ = format(data_fmt_);
    for (size_t i = 0; i < cols_.size(); ++i) {
      data_row_.modify_item(i+1, std::setw(cols_.at(i).width));
    }

    table_open_ = false;    
  }
  
  void write_top_rule(void) const
  {
    std::cout << top_rule_ << std::endl;
  }

  void write_mid_rule(void) const
  {
    std::cout << mid_rule_ << std::endl;
  }

  void write_bot_rule(void) const
  {
    std::cout << bot_rule_ << std::endl;
  }

  void write_header_row(void) const
  {
    std::cout << head_row_ << std::endl;
  }

  void write_data_row(const Types&... data) const
  {
    std::cout << "│";
    write_data(0, data...);
    std::cout << std::endl;
  }

  template<typename T, typename... Targs>
  void write_data(const size_t& col_no,
		  const T& value,
		  const Targs&... remainder) const
  {
    write_datum(col_no, value);
    std::cout << "│";
    write_data(col_no + 1, remainder...);
  }

  void write_data(const size_t& col_no) const
  {
    // Nothing left to write
    assert(col_no == cols_.size());
  }

private:

  template<typename T>
  void write_datum(const size_t& col_no, const T& datum) const;
  
};

template<typename ... Types>
template<typename T>
void DisplayTable<Types...>::
write_datum(const size_t& col_no, const T& datum) const
{
  // Get the formatted string
  format fmt(cols_.at(col_no).bff);
  fmt % datum;
  std::string fmted = fmt.str();

  // Work out how much additional padding to give the string
  const Column& col = cols_.at(col_no);
  size_t padding = col.width;
  bool trim = false; // Is the string actually longer than the
		     // available width?
  for (char c : fmted) {
    if ((c & 0x80) == 0 ||
	(c & 0xc0) == 0xc0) {
      padding--;
      if (padding == 0) {
	trim = true;
	break;
      }
    }
  }
  
  std::cout << std::string(padding, ' ')
	    << (trim ? fmted.substr(0, col.width) : fmted);
}

#endif

