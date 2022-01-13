/***********************************************************************
 * DataArray.hpp
 *
 * Class representing a generic multi-dimensional array that can be
 * accessed from both host and device
 *
 * Copyright (C) Gerald C J Morgan 2020
 ***********************************************************************/

#ifndef DataArray_hpp
#define DataArray_hpp

#include <vector>
#include <memory>

#include "sycl.hpp"

template<typename T>
class DataArray
{
protected:

  std::shared_ptr< sycl::queue > queue_;
  std::shared_ptr< std::vector<T> > host_data_;
  std::shared_ptr< sycl::buffer<T,1> > device_data_;

public:
  
  using AccessMode = sycl::access::mode;
  using AccessTarget = sycl::access::target;
  using AccessPlaceholder = sycl::access::placeholder;
  
  template<AccessMode Mode,
	   AccessTarget Target = AccessTarget::global_buffer,
	   AccessPlaceholder IsPlaceholder = AccessPlaceholder::false_t>
  using Accessor = sycl::accessor<T, 1, Mode, Target, IsPlaceholder>;

  template<AccessMode Mode, AccessTarget Target>
  Accessor<Mode, Target> get_accessor(sycl::handler& cgh) const
  {
    return device_data_->template get_access<Mode, Target>(cgh);
  }

  Accessor<sycl::access::mode::read>
  get_read_accessor(sycl::handler& cgh) const
  {
    return device_data_->template get_access<sycl::access::mode::read>(cgh);
  }
  
  Accessor<sycl::access::mode::write>
  get_write_accessor(sycl::handler& cgh) const
  {
    return device_data_->template get_access<sycl::access::mode::write>(cgh);
  }
  
  Accessor<sycl::access::mode::discard_write>
  get_discard_write_accessor(sycl::handler& cgh) const
  {
    return device_data_->template get_access<sycl::access::mode::discard_write>(cgh);
  }
  
  Accessor<sycl::access::mode::read_write>
  get_read_write_accessor(sycl::handler& cgh) const
  {
    return device_data_->template get_access<sycl::access::mode::read_write>(cgh);
  }
  
  template<AccessMode Mode, AccessTarget Target>
  Accessor<Mode, Target, AccessPlaceholder::true_t> get_placeholder_accessor(void) const
  {
    assert((bool)device_data_);
    return Accessor<Mode, Target, AccessPlaceholder::true_t>(*device_data_);
  }

public:
  
  DataArray(const std::shared_ptr<sycl::queue>& queue,
	    const std::vector<T>& data)
    : queue_(queue),
      host_data_(std::make_shared< std::vector<T> >(data)),
      device_data_()
  {
    std::cout << "Allocated data array of "
	      << host_data_->size()
	      << " elements of type "
	      << typeid(T).name() << std::endl;
  }
  
  DataArray(const std::shared_ptr<sycl::queue>& queue,
	    const size_t& size, const T& value = T())
    : queue_(queue),
      host_data_(std::make_shared< std::vector<T> >(size, value)),
      device_data_()
  {
    std::cout << "Allocated data array of "
	      << host_data_->size()
	      << " elements of type "
	      << typeid(T).name() << std::endl;
  }

  DataArray(const std::shared_ptr<sycl::queue>& queue,
	    const size_t& size, bool on_device,
	    const T& value = T())
    : queue_(queue),
      host_data_(),
      device_data_()
  {
    if (on_device) {
      //assert(value == T());
      device_data_ = std::make_shared<sycl::buffer<T,1>>(sycl::range<1>(size));
      queue_->submit([&](sycl::handler& cgh)
      {
	cgh.fill(this->get_discard_write_accessor(cgh), value);
      });
      //      device_data_->set_final_data(host_data_->begin());
    } else {
      host_data_ = std::make_shared<std::vector<T>>(size,value);
    }
  }
  
  DataArray(const DataArray<T>& da)
    : queue_(da.queue_),
      host_data_(),
      device_data_()
  {
    if (da.host_data_) {
      host_data_ = std::make_shared< std::vector<T> >(*da.host_data_);
    }
    if (da.is_on_device()) {
      if (host_data_) {
	move_to_device();
      } else {
	device_data_ = std::make_shared<sycl::buffer<T,1>>(sycl::range<1>(da.size()));
      }
      da.queue_->submit([&](sycl::handler& cgh)
      {
	cgh.copy(da.get_read_accessor(cgh),
		 this->get_discard_write_accessor(cgh));
      });
    }
  }

  DataArray(DataArray<T>&& da) = default;

  ~DataArray(void)
  {
    if (device_data_) device_data_->set_final_data();
  }

  size_t size(void) const
  {
    if (host_data_) {
      return host_data_->size();
    } else if (device_data_) {
      return device_data_->get_count();
    } else {
      throw std::logic_error("Data array has neither host nor device data.");
    }
  }

  const std::shared_ptr<sycl::queue>& queue_ptr(void) const
  {
    return queue_;
  }
  
  sycl::queue& queue(void) const
  {
    return *queue_;
  }
  
  const std::vector<T>& host_vector(void) const
  {
    assert(!device_data_);
    assert(host_data_);
    return *host_data_;
  }

  std::vector<T>& host_vector(void)
  {
    assert(!device_data_);
    assert(host_data_);
    return *host_data_;
  }

  void move_to_device(void)
  {
    if (device_data_) {
      // Data is already on the device. Do nothing.
      return;
    }

    if (host_data_ && host_data_->size() > 0) {
      // Create the SYCL buffer object
      device_data_ =
	std::make_shared<sycl::buffer<T,1>>(host_data_->data(),
					    sycl::range<1>(host_data_->size()));
    } else {
      // No actual data to copy, but we need there to be a thing on
      // the device
      host_data_ = std::make_shared<std::vector<T>>(1);
      device_data_ =
	std::make_shared<sycl::buffer<T,1>>(host_data_->data(),
					    sycl::range<1>(host_data_->size()));

      host_data_->pop_back();
    }
  }

  void move_to_host(void)
  {
    if (not host_data_) {
      host_data_ = std::make_shared<std::vector<T>>(device_data_->get_count());
      device_data_->set_final_data(host_data_->begin());
    }
    device_data_.reset();
  }

  bool is_on_device(void) const
  {
    return (bool) device_data_;
  }

  sycl::buffer<T,1>& get_buffer(void)
  {
    return *device_data_;
  }
  
};

#endif
