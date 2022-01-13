
#include <fstream>
#include <iostream>
#include <array>

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
	  ? read_vector_be_on_le<T,NT>(is, arr)
	  : read_vector_be_on_be<T,NT>(is, arr));
}

int main(int argc, char* argv[])
{
  std::ifstream in_file(argv[1], std::ios::binary);
  if (not in_file.is_open()) {
    std::cerr << "Could not open NIMROD data file at "
	      << argv[1] << std::endl;
    throw std::runtime_error("Could not open NIMROD data file.");
  }

  uint32_t test;
  std::array<int16_t,31> h1;
  bool sil = get_system_is_le();
  test = read_be<uint32_t>(sil, in_file);
  read_array_be<int16_t>(sil, in_file, h1);

  std::cout << test << std::endl;
  for (size_t i = 0; i < 31; ++i) {
    std::cout << h1.at(i) << std::endl;
  }
  
  return 0;
}
