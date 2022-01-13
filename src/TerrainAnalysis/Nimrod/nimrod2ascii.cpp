
#include <iostream>
#include <iomanip>
#include <fstream>
#include <array>
#include <vector>

#include <boost/filesystem.hpp>
namespace stdfs = boost::filesystem;

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
struct NIMRODImage {
  
  std::array<int16_t,31> h1;
  std::array<float,28> h2;
  std::array<float,45> h3;
  std::array<char,56> h4;
  std::array<int16_t,51> h5;
  
  std::vector<T> buffer;
  size_t nxpx;
  size_t nypx;
  std::array<double,6> geotrans;
  T nodata_value;

  size_t ulc_xpx;
  size_t ulc_ypx;
  size_t lrc_xpx;
  size_t lrc_ypx;

  size_t total_cols(void) const
  {
    return nxpx;
  }
  
  size_t total_rows(void) const
  {
    return nypx;
  }

  size_t ncols(void) const
  {
    return 1 + lrc_xpx - ulc_xpx;
  }

  size_t nrows(void) const
  {
    return 1 + lrc_ypx - ulc_ypx;
  }

  std::array<double,2> llc(void) const
  {
    return {
      geotrans[0] + ulc_xpx * geotrans[1],
      geotrans[3] + (nypx - lrc_ypx - 1) * geotrans[5],
    };
  }

  T value(size_t col, size_t row)
  {
    size_t j = col + ulc_xpx;
    size_t i = row + ulc_ypx;
    return buffer.at(i * nxpx + j);
  }
  
  template<typename U>
  void read_nimrod_vector(const bool& sil,
			  std::istream& in_file,
			  std::vector<T>& buffer)
  {
    nxpx = h1[16];
    nypx = h1[15];
    buffer.resize(nxpx*nypx, T());
    
    std::vector<U> in_data;
    in_data.resize(nxpx*nypx, U());
    read_vector_be<U>(sil, in_file, in_data);
    for (size_t i = 0; i < in_data.size(); ++i) {
      buffer.at(i) = in_data.at(i);
    }
    in_data.clear();
  }

  NIMRODImage(const stdfs::path& filepath,
	      const std::array<double, 4>& bbox = {0.0, 0.0, -1.0, -1.0})
  {
    std::ifstream in_file(filepath, std::ios::binary);
    if (not in_file.is_open()) {
      std::cerr << "Could not open NIMROD data file at "
		<< filepath << std::endl;
      throw std::runtime_error("Could not open NIMROD data file.");
    }

    const bool sil = get_system_is_le();
    
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
    read_array_be<int16_t,31>(sil, in_file, h1);
    read_array_be<float,28>(sil, in_file, h2);
    read_array_be<float,45>(sil, in_file, h3);
    read_array_be<char,56>(sil, in_file, h4);
    read_array_be<int16_t,51>(sil, in_file, h5);

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

    const uint16_t& data_type = h1[11];
    const uint16_t& data_bpp = h1[12];
    
    switch (data_type) {
    case 0:
      // Float
      if (data_bpp != 4) {
	std::cerr << "Unexpected value for data bpp. "
		  << "Must be 4 for real data. Got " << data_bpp << std::endl;
	throw std::runtime_error("Unsupported bpp for float data.");
      }
      read_nimrod_vector<float>(sil, in_file, buffer);
      nodata_value = h2[6];
      break;
    case 1:
      // Integer data
      switch (data_bpp) {
      case 2:
	read_nimrod_vector<int16_t>(sil, in_file, buffer);
	break;
      case 4:
	read_nimrod_vector<int32_t>(sil, in_file, buffer);
	break;
      default:
	std::cerr << "Unexpected value for data bpp. "
		  << "Must be 2 or 4 for integer data. "
		  << " Got " << data_bpp << std::endl;
	throw std::runtime_error("Unsupported bpp for integer data.");
      };
      nodata_value = h1[24];
      break;
    case 2:
      // Char data
      if (data_bpp != 1) {
	std::cerr << "Unexpected value for data bpp. "
		  << "Must be 1 for char data. Got " << data_bpp << std::endl;
	throw std::runtime_error("Unsupported bpp for char data.");
      }
      read_nimrod_vector<char>(sil, in_file, buffer);
      nodata_value = h1[24];
      break;
    default:
      std::cerr << "Unexpected value for data type. "
		<< "Expected 0, 1 or 2. Got " << data_type << std::endl;
      throw std::runtime_error("Unsupported NIMROD data type");
    };

    if (block_size != read_be<uint32_t>(sil, in_file)) {
      std::cerr << "Expected block size indicator matching "
		<< block_size << " at offset "
		<< in_file.tellg() << std::endl;
      throw std::runtime_error("Error reading NIMROD file.");
    }
    
    in_file.close();

    std::cout << "Read NIMROD data file" << std::endl;
    std::cout << "  Validity Time: "
	      << std::setw(4) << std::setfill('0') << h1[0] << "-"
	      << std::setw(2) << std::setfill('0') << h1[1] << "-"
	      << std::setw(2) << std::setfill('0') << h1[2] << " "
	      << std::setw(2) << std::setfill('0') << h1[3] << ":"
	      << std::setw(2) << std::setfill('0') << h1[4] << ":"
	      << std::setw(2) << std::setfill('0') << h1[5] << std::endl;
    std::cout << "  Data Time: "
	      << std::setw(4) << std::setfill('0') << h1[6] << "-"
	      << std::setw(2) << std::setfill('0') << h1[7] << "-"
	      << std::setw(2) << std::setfill('0') << h1[8] << " "
	      << std::setw(2) << std::setfill('0') << h1[9] << ":"
	      << std::setw(2) << std::setfill('0') << h1[10]
	      << ":00" << std::endl;
    std::cout << "  Data Type: ";
    switch (h1[11]) {
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
    std::cout << "(" << h1[12] << " bytes per datum)" << std::endl;
    if (h1[13] != -32767) {
      std::cout << "  Experiment No.: " << h1[13] << std::endl;
    }
    std::cout << "  Grid Type: ";
    switch (h1[14]) {
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
    std::cout << "  Grid: " << h1[16] << "×"
	      << h1[15] << " cells." << std::endl;
    std::cout << "  Origin at ";
    double llc_x, llc_y, urc_y;
    switch (h1[23]) {
    case 0:
      std::cout << "top-left" << std::endl;
      llc_x = h2[4] - 0.5 * h2[5];
      llc_y = h2[2] + (0.5 - (double)h1[15]) * h2[3];
      urc_y = h2[2] + 0.5 * h2[3];
      break;
    case 1:
      std::cout << "bottom-left" << std::endl;
      throw std::runtime_error("Bottom-left grid origin location.");
      /*
      llc_x = h2[4] - 0.5 * h2[5];
      llc_y = h2[2] + 0.5 * h2[3];
      urc_y = h2[2] + (0.5 - (double)h1[15]) * h2[3];      
      break;
      */
    case 2:
      std::cout << "top-right" << std::endl;
      throw std::runtime_error("Top-right grid origin location.");      
    case 3:
      std::cout << "bottom-right" << std::endl;
      throw std::runtime_error("Bottom-right grid origin location.");      
    default:
      std::cout << "Unknown" << std::endl;
      throw std::runtime_error("Unknown grid origin location.");      
    };
    geotrans = {
      llc_x,
      h2[5],
      0.0,
      llc_y,
      0.0,
      h2[3]
    };
    std::cout << "  Origin: " << h2[4] << ", " << h2[2] << std::endl;
    std::cout << "  Pixel Size: " << h2[5] << ", " << h2[3] << std::endl;
    std::cout << "  LLC: " << llc_x << ", " << llc_y << std::endl;
    
    if (bbox[0] < bbox[2] and bbox[1] < bbox[3]) {
      ulc_xpx = (size_t)((bbox[0] - geotrans[0]) / geotrans[1]);
      if (ulc_xpx >= nxpx) ulc_xpx = 0;
      ulc_ypx = (size_t)((urc_y - bbox[3]) / geotrans[5]);
      if (ulc_ypx >= nypx) ulc_ypx = nypx - 1;
      lrc_xpx = (size_t)((bbox[2] - geotrans[0]) / geotrans[1]);
      if (lrc_xpx >= nxpx) lrc_xpx = nxpx - 1;
      lrc_ypx = (size_t)((urc_y - bbox[1]) / geotrans[5]);
      if (lrc_ypx >= nypx) lrc_ypx = 0;
      std::cout << "  bbox XY: " << bbox[0] << ", " << bbox[1] << " -> " << bbox[2] << ", " << bbox[3] << std::endl;
      std::cout << "  bbox MN: " << ulc_xpx << ", " << ulc_ypx << " -> " << lrc_xpx << ", " << lrc_ypx << std::endl;
    } else {
      ulc_xpx = 0;
      ulc_ypx = 0;
      lrc_xpx = nxpx - 1;
      lrc_ypx = nypx - 1;
    }

    std::cout << "  Units: " << std::string(h4.data(), 8) << std::endl;
    std::cout << "  Data Source: " << std::string(h4.data()+8, 24) << std::endl;
    std::cout << "  Field Name: " << std::string(h4.data()+8+24, 24) << std::endl;
    std::cout << "  Scaling Factor: " << h2[7] << std::endl;
    std::cout << "  Data Offset: " << h2[8] << std::endl;
  }
};

int main(int argc, char* argv[])
{
  std::array<double, 4> bbox = {0.0,0.0,-1.0,-1.0};
  if (argc != 3) {
    if (argc != 7) {
      std::cerr << "Expect 2 arguments: input file and output file" << std::endl;
      return 1;
    } else {
      for (size_t i = 0; i < 4; ++i) {
	bbox[i] = std::stod(argv[i+3]);
      }
    }
  }
  
  stdfs::path filepath = argv[1];

  NIMRODImage<float> img(filepath, bbox);

  std::ofstream of(argv[2]);
  if (!of.is_open()) {
    std::cerr << "Could not open file: " << argv[2] << std::endl;
    return 1;
  }

  auto llc = img.llc();
  
  of << "ncols " << img.ncols() << std::endl;
  of << "nrows " << img.nrows() << std::endl;
  of << "xllcorner " << llc[0] << std::endl;
  of << "yllcorner " << llc[1] << std::endl;
  of << "cellsize " << img.geotrans[1] << std::endl;
  of << "nodata_value " << img.nodata_value << std::endl;

  for (size_t i = 0; i < img.nrows(); ++i) {
    for (size_t j = 0; j < img.ncols(); ++j) {
      if (j > 0) of << " ";
      of << img.value(j, i);
    }
    of << std::endl;
  }

  of.close();
  
  return 0;
}


  
