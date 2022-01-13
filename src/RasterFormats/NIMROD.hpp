/***********************************************************************
 * RasterFormats/NIMROD.hpp
 *
 * Copyright (C) Gerald C J Morgan 2021
 ***********************************************************************/

#ifndef RasterFormats_NIMROD_hpp
#define RasterFormats_NIMROD_hpp

#include "../RasterFormat.hpp"

const bool get_system_is_le(void)
{
  union {
    uint8_t c[4];
    uint32_t i;
  } u;
  u.i = 0x01020304;
  return (u.c[0] == 0x04);
}

template<uint8_t N>
void reverse_endianness(char* in, char* out);

template<>
void reverse_endianness<1>(char* in, char* out)
{
  out[0] = in[0];
}

template<>
void reverse_endianness<2>(char* in, char* out)
{
  uint16_t* a = (uint16_t*) in;
  uint16_t* t = (uint16_t*) out;
  *t = ((*a)>>8) | ((*a) << 8);
}

template<>
void reverse_endianness<4>(char* in, char* out)
{
  uint32_t* a = (uint32_t*) in;
  uint32_t* t = (uint32_t*) out;
  *t = ((*a>>24)&0xff) | ((*a<<8)&0xff0000) |
    ((*a>>8)&0xff00) | ((*a<<24)&0xff000000);
}

template<typename T,
	 uint8_t N = sizeof(T)>
T read_be_on_le(std::istream& is)
{
  char data[N];
  if (is.read(data, N)) {
    T result;
    reverse_endianness<N>(data, (char*)&result);
    return result;
  } else {
    std::cerr << "Could not read " << N << " bytes at offset"
	      << is.tellg() << "." << std::endl;
    throw std::runtime_error("Could not read bytes from file.");
  }
}

template<typename T,
	 uint8_t N = sizeof(T)>
T read_be_on_be(std::istream& is)
{
  char data[N];
  if (is.read(data, N)) {
    return *((T*)(data));
  } else {
    std::cerr << "Could not read " << N << " bytes at offset"
	      << is.tellg() << "." << std::endl;
    throw std::runtime_error("Could not read bytes from file.");
  }
}

template<typename T,
	 uint8_t N = sizeof(T)>
T read_be(bool system_is_le, std::istream& is)
{
  return (system_is_le ? read_be_on_le<T>(is) : read_be_on_be<T>(is));
}

template<typename T,
	 size_t NT,
	 uint8_t N = sizeof(T)>
void read_array_be_on_le(std::istream& is, std::array<T,NT>& arr)
{
  char data[N*NT];
  if (is.read(data, N*NT)) {
    for (size_t i = 0; i < NT; ++i) {
      reverse_endianness<N>(data + i*N, (char*)(&arr.at(i)));
    }
  } else {
    std::cerr << "Could not read " << N << " bytes at offset"
	      << is.tellg() << "." << std::endl;
    throw std::runtime_error("Could not read bytes from file.");
  }
}

template<typename T,
	 size_t NT,
	 uint8_t N = sizeof(T)>
void read_array_be_on_be(std::istream& is, std::array<T,NT>& arr)
{
  if (is.read((char*)arr.data(), NT*N)) {
    // success
    return;
  } else {
    std::cerr << "Could not read " << NT*N << " bytes at offset"
	      << is.tellg() << "." << std::endl;
    throw std::runtime_error("Could not read bytes from file.");
  }
}

template<typename T, size_t NT,
	 uint8_t N = sizeof(T)>
void read_array_be(bool system_is_le, std::istream& is, std::array<T,NT>& arr)
{
  return (system_is_le
	  ? read_array_be_on_le<T,NT>(is, arr)
	  : read_array_be_on_be<T,NT>(is, arr));
}

template<typename T,
	 uint8_t N = sizeof(T)>
void read_vector_be_on_le(std::istream& is, std::vector<T>& arr)
{
  size_t NT = arr.size();
  char data[N*NT];
  if (is.read(data, N*NT)) {
    for (size_t i = 0; i < NT; ++i) {
      reverse_endianness<N>(data + i*N, (char*)(&arr.at(i)));
    }
  } else {
    std::cerr << "Could not read " << N << " bytes at offset"
	      << is.tellg() << "." << std::endl;
    throw std::runtime_error("Could not read bytes from file.");
  }
}

template<typename T,
	 uint8_t N = sizeof(T)>
void read_vector_be_on_be(std::istream& is, std::vector<T>& arr)
{
  size_t NT = arr.size();
  if (is.read((char*)arr.data(), NT*N)) {
    // success
    return;
  } else {
    std::cerr << "Could not read " << NT*N << " bytes at offset"
	      << is.tellg() << "." << std::endl;
    throw std::runtime_error("Could not read bytes from file.");
  }
}

template<typename T,
	 uint8_t N = sizeof(T)>
void read_vector_be(bool system_is_le, std::istream& is, std::vector<T>& arr)
{
  return (system_is_le
	  ? read_vector_be_on_le<T>(is, arr)
	  : read_vector_be_on_be<T>(is, arr));
}

template<typename T>
class NIMRODRasterFormat : public RasterFormat<T>
{
private:

  std::array<int16_t,31> h1_;
  std::array<float,28> h2_;
  std::array<float,45> h3_;
  std::array<char,56> h4_;
  std::array<int16_t,51> h5_;
  
  std::vector<T> buffer_;
  size_t nxpx_;
  size_t nypx_;
  std::array<double,6> geotrans_;
  T nodata_value_;

  size_t ulc_xpx_;
  size_t ulc_ypx_;
  size_t lrc_xpx_;
  size_t lrc_ypx_;
  std::vector<T> values_;

  size_t total_cols(void) const
  {
    return nxpx_;
  }
  
  size_t total_rows(void) const
  {
    return nypx_;
  }

  std::array<double,2> llc(void) const
  {
    return {
      geotrans_[0] + ulc_xpx_ * geotrans_[1],
      geotrans_[3] + (nypx_ - lrc_ypx_ - 1) * geotrans_[5],
    };
  }

  T value(size_t col, size_t row)
  {
    size_t j = col + ulc_xpx_;
    size_t i = row + ulc_ypx_;
    return buffer_.at(i * nxpx_ + j);
  }
  
  template<typename U>
  void read_nimrod_vector(const bool& sil,
			  std::istream& in_file)
  {
    nxpx_ = h1_[16];
    nypx_ = h1_[15];
    buffer_.resize(nxpx_*nypx_, T());
    
    std::vector<U> in_data;
    in_data.resize(nxpx_*nypx_, U());
    read_vector_be<U>(sil, in_file, in_data);
    for (size_t i = 0; i < in_data.size(); ++i) {
      buffer_.at(i) = in_data.at(i);
    }
    in_data.clear();
  }

protected:

  virtual const std::vector<T>& values(void) const
  {
    return values_;
  }
  
  virtual size_t ncols(void) const
  {
    return 1 + lrc_xpx_ - ulc_xpx_;
  }

  virtual size_t nrows(void) const
  {
    return 1 + lrc_ypx_ - ulc_ypx_;
  }

  virtual const std::array<double, 6>& geo_transform(void) const
  {
    return geotrans_;
  }
  
  virtual T nodata_value(void) const
  {
    return nodata_value_;
  }

public:

  NIMRODRasterFormat(const stdfs::path& filepath,
		     const Config& conf)
    : RasterFormat<T>()
  {
    std::ifstream in_file(filepath, std::ios::binary);
    if (not in_file.is_open()) {
      std::cerr << "Could not open NIMROD data file at "
		<< filepath << std::endl;
      throw std::runtime_error("Could not open NIMROD data file.");
    }
    
    const bool sil = get_system_is_le();

    std::array<double, 4> bbox = {0.0, 0.0, -1.0, -1.0};
    if (conf.count("bbox") > 0) {
      if (conf.count("bbox") > 1) {
	std::cerr << "Only one bounding box can be applied"
		  << " to a NIMROD data file." << std::endl;
	throw std::runtime_error("Too many bounding boxes.");
      }

      bbox = split_string<double,4>(conf.get<std::string>("bbox"));
      if (bbox[2] <= bbox[0]) {
	std::cerr << "Bounding box has negative x-dimension: "
		  << bbox[2] << " <= " << bbox[0] << std::endl;
	throw std::runtime_error("Invalid bounding box.");
      }
      if (bbox[3] <= bbox[1]) {
	std::cerr << "Bounding box has negative y-dimension: "
		  << bbox[3] << " <= " << bbox[1] << std::endl;
	throw std::runtime_error("Invalid bounding box.");
      }
    }

    
    // Undocumented 4-byte unsigned integer: length of the following
    // data block in bytes:
    uint32_t block_size = read_be<uint32_t>(sil, in_file);
    if (block_size != 512) {
      std::cerr << "Expected header size indicator of 512. Got "
		<< block_size << " at offset "
		<< in_file.tellg() << std::endl;
      throw std::runtime_error("Error reading NIMROD file.");
    }

    // Read the header data
    read_array_be<int16_t,31>(sil, in_file, h1_);
    read_array_be<float,28>(sil, in_file, h2_);
    read_array_be<float,45>(sil, in_file, h3_);
    read_array_be<char,56>(sil, in_file, h4_);
    read_array_be<int16_t,51>(sil, in_file, h5_);

    // Undocumented 4-byte unsigned integer: length of the preceding
    // data block in bytes:
    block_size = read_be<uint32_t>(sil, in_file);
    if (block_size != 512) {
      std::cerr << "Expected header size indicator of 512. Got "
		<< block_size << " at offset "
		<< in_file.tellg() << std::endl;
      throw std::runtime_error("Error reading NIMROD file.");
    }
    
    // Undocumented 4-byte unsigned integer: length of the following
    // data block in bytes:
    block_size = read_be<uint32_t>(sil, in_file);

    // Get type of data in the NIMROD file
    const uint16_t& data_type = h1_[11];
    const uint16_t& data_bpp = h1_[12];
    
    switch (data_type) {
    case 0:
      // Float
      if (data_bpp != 4) {
	std::cerr << "Unexpected value for data bpp. "
		  << "Must be 4 for real data. Got " << data_bpp << std::endl;
	throw std::runtime_error("Unsupported bpp for float data.");
      }
      read_nimrod_vector<float>(sil, in_file);
      nodata_value_ = h2_[6];
      break;
    case 1:
      // Integer data
      switch (data_bpp) {
      case 2:
	read_nimrod_vector<int16_t>(sil, in_file);
	break;
      case 4:
	read_nimrod_vector<int32_t>(sil, in_file);
	break;
      default:
	std::cerr << "Unexpected value for data bpp. "
		  << "Must be 2 or 4 for integer data. "
		  << " Got " << data_bpp << std::endl;
	throw std::runtime_error("Unsupported bpp for integer data.");
      };
      nodata_value_ = h1_[24];
      break;
    case 2:
      // Char data
      if (data_bpp != 1) {
	std::cerr << "Unexpected value for data bpp. "
		  << "Must be 1 for char data. Got " << data_bpp << std::endl;
	throw std::runtime_error("Unsupported bpp for char data.");
      }
      read_nimrod_vector<char>(sil, in_file);
      nodata_value_ = h1_[24];
      break;
    default:
      std::cerr << "Unexpected value for data type. "
		<< "Expected 0, 1 or 2. Got " << data_type << std::endl;
      throw std::runtime_error("Unsupported NIMROD data type");
    };

    // Undocumented 4-byte unsigned integer: length of the preceding
    // data block in bytes:    
    if (block_size != read_be<uint32_t>(sil, in_file)) {
      std::cerr << "Expected block size indicator matching "
		<< block_size << " at offset "
		<< in_file.tellg() << std::endl;
      throw std::runtime_error("Error reading NIMROD file.");
    }

    in_file.close();

    double llc_x, llc_y, urc_y;
    switch (h1_[23]) {
    case 0:
      llc_x = h2_[4] - 0.5 * h2_[5];
      llc_y = h2_[2] + (0.5 - (double)h1_[15]) * h2_[3];
      urc_y = h2_[2] + 0.5 * h2_[3];
      break;
    case 1:
      throw std::runtime_error("Bottom-left grid origin location.");
      /*
	llc_x = h2[4] - 0.5 * h2[5];
	llc_y = h2[2] + 0.5 * h2[3];
	urc_y = h2[2] + (0.5 - (double)h1[15]) * h2[3];      
	break;
      */
    case 2:
      throw std::runtime_error("Top-right grid origin location.");      
    case 3:
      throw std::runtime_error("Bottom-right grid origin location.");      
    default:
      throw std::runtime_error("Unknown grid origin location.");      
    };
    
    geotrans_ = {
      llc_x,
      h2_[5],
      0.0,
      llc_y,
      0.0,
      h2_[3]
    };

    if (bbox[0] < bbox[2] and bbox[1] < bbox[3]) {
      ulc_xpx_ = (size_t)((bbox[0] - geotrans_[0]) / geotrans_[1]);
      if (ulc_xpx_ >= nxpx_) ulc_xpx_ = 0;
      ulc_ypx_ = (size_t)((urc_y - bbox[3]) / geotrans_[5]);
      if (ulc_ypx_ >= nypx_) ulc_ypx_ = nypx_ - 1;
      lrc_xpx_ = (size_t)((bbox[2] - geotrans_[0]) / geotrans_[1]);
      if (lrc_xpx_ >= nxpx_) lrc_xpx_ = nxpx_ - 1;
      lrc_ypx_ = (size_t)((urc_y - bbox[1]) / geotrans_[5]);
      if (lrc_ypx_ >= nypx_) lrc_ypx_ = 0;

      values_.resize(ncols() * nrows());
      for (size_t i = 0; i < nrows(); ++i) {
	for (size_t j = 0; j < ncols(); ++j) {
	  values_.at(i * ncols() + j) = value(j, i);
	}
      }
    } else {
      ulc_xpx_ = 0;
      ulc_ypx_ = 0;
      lrc_xpx_ = nxpx_ - 1;
      lrc_ypx_ = nypx_ - 1;
    }

    if (conf.get<bool>("verbose", false)) {
      std::cout << "Read NIMROD data file" << std::endl;
      std::cout << "  Validity Time: "
		<< std::setw(4) << std::setfill('0') << h1_[0] << "-"
		<< std::setw(2) << std::setfill('0') << h1_[1] << "-"
		<< std::setw(2) << std::setfill('0') << h1_[2] << " "
		<< std::setw(2) << std::setfill('0') << h1_[3] << ":"
		<< std::setw(2) << std::setfill('0') << h1_[4] << ":"
		<< std::setw(2) << std::setfill('0') << h1_[5] << std::endl;
      std::cout << "  Data Time: "
		<< std::setw(4) << std::setfill('0') << h1_[6] << "-"
		<< std::setw(2) << std::setfill('0') << h1_[7] << "-"
		<< std::setw(2) << std::setfill('0') << h1_[8] << " "
		<< std::setw(2) << std::setfill('0') << h1_[9] << ":"
		<< std::setw(2) << std::setfill('0') << h1_[10]
		<< ":00" << std::endl;
      std::cout << "  Data Type: ";
      switch (h1_[11]) {
      case 0:
	std::cout << "Real ";
	break;
      case 1:
	std::cout << "Integer ";
	break;
      case 2:
	std::cout << "Character ";
	break;
      default:
	std::cout << "Unknown ";
      };
      std::cout << "(" << h1_[12] << " bytes per datum)" << std::endl;
      if (h1_[13] != -32767) {
	std::cout << "  Experiment No.: " << h1_[13] << std::endl;
      }
      std::cout << "  Grid Type: ";
      switch (h1_[14]) {
      case 0:
	std::cout << "NG" << std::endl;
	break;
      case 1:
	std::cout << "lat/long" << std::endl;
	throw std::runtime_error("Lat/long grid not supported in NIMROD data file.");
      case 2:
	std::cout << "space view" << std::endl;
	throw std::runtime_error("Space view grid not supported in NIMROD data file.");
      case 3:
	std::cout << "polar stereographic" << std::endl;
	throw std::runtime_error("Polar stereographic grid not supported in NIMROD data file.");
      case 4:
	std::cout << "XY" << std::endl;
	throw std::runtime_error("XY grid not supported in NIMROD data file.");
      default:
	std::cout << "Unknown" << std::endl;
	throw std::runtime_error("Unknown grid not supported in NIMROD data file.");
      }
      std::cout << "  Grid: " << h1_[16] << "×"
		<< h1_[15] << " cells." << std::endl;
      std::cout << "  Origin: " << h2_[4] << ", " << h2_[2] << std::endl;
      std::cout << "  Pixel Size: " << h2_[5] << ", " << h2_[3] << std::endl;
      std::cout << "  LLC: " << llc_x << ", " << llc_y << std::endl;
      if (bbox[0] < bbox[2] and bbox[1] < bbox[3]) {
	std::cout << "  bbox XY: " << bbox[0] << ", " << bbox[1] << " -> " << bbox[2] << ", " << bbox[3] << std::endl;
	std::cout << "  bbox MN: " << ulc_xpx_ << ", " << ulc_ypx_ << " -> " << lrc_xpx_ << ", " << lrc_ypx_ << std::endl;
      }
    
      std::cout << "  Units: " << std::string(h4_.data(), 8) << std::endl;
      std::cout << "  Data Source: " << std::string(h4_.data()+8, 24) << std::endl;
      std::cout << "  Field Name: " << std::string(h4_.data()+8+24, 24) << std::endl;
      std::cout << "  Scaling Factor: " << h2_[7] << std::endl;
      std::cout << "  Data Offset: " << h2_[8] << std::endl;
    }
  }

  virtual ~NIMRODRasterFormat(void) {}
  
};

#endif
