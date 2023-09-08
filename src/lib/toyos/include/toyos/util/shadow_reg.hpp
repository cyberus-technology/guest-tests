/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <functional>

#include <toyos/util/buffer_view.hpp>
#include <toyos/util/traits.hpp>

namespace cbl
{

   template<typename RegType, typename HostResource>
   class shadow_reg
   {
   public:
      using reg_t = RegType;
      using resource_t = HostResource;
      using acquire_fn = std::function<resource_t(reg_t)>;
      using delete_fn = std::function<void(resource_t&)>;
      using value_fn = std::function<reg_t(const resource_t&)>;

      static void free_resource_impl(resource_t& res)
      {
         res = nullptr;
      }

   private:
      reg_t shadow_ = {};
      volatile reg_t* host_addr_;

      resource_t host_resource_;
      acquire_fn acquire_func_;
      delete_fn free_func_;
      value_fn host_value_;

   public:
      shadow_reg(reg_t* addr, acquire_fn a_fn, value_fn v_fn, delete_fn f_fn = free_resource_impl)
         : host_addr_(addr), acquire_func_(a_fn), free_func_(f_fn), host_value_(v_fn)
      {}

      void acquire_resource()
      {
         host_resource_ = acquire_func_(shadow_);
         if(host_resource_) {
            auto v = host_value_(host_resource_);
            *host_addr_ = v;
         }
      }

      void free_resource()
      {
         free_resource_impl(host_resource_);
      }

      void read(cbl::buffer_view& buf, off_t offset) const
      {
         ASSERT(sizeof(reg_t) <= buf.bytes() + offset, "Invalid read on shadow_reg");

         if(sizeof(reg_t) == buf.bytes() + offset) {
            buf.write<reg_t>(shadow_);
         }

         const cbl::buffer_view shadow_buf(const_cast<reg_t*>(&shadow_), sizeof(shadow_));

         switch(buf.bytes()) {
            DEFAULT_TO_PANIC("Unsupported size: {}", buf.bytes());
            case 1:
               buf.write<uint8_t>(shadow_buf.read<const uint8_t>(offset));
               break;
            case 2:
               buf.write<uint16_t>(shadow_buf.read<const uint16_t>(offset));
               break;
            case 4:
               buf.write<uint32_t>(shadow_buf.read<const uint32_t>(offset));
               break;
            case 8:
               buf.write<uint64_t>(shadow_buf.read<const uint64_t>(offset));
               break;
         }
      }

      void set_shadow(const cbl::buffer_view& buf, off_t offset)
      {
         ASSERT(sizeof(reg_t) >= buf.bytes() + offset,
                "Invalid access on shadow_reg regsz: {#x} bytes: {#x} offset: {#x}",
                sizeof(reg_t),
                buf.bytes(),
                offset);

         if(sizeof(reg_t) == buf.bytes()) {
            shadow_ = buf.read<reg_t>();
            return;
         }

         cbl::buffer_view shadow_buf(&shadow_, sizeof(shadow_));

         switch(buf.bytes()) {
            DEFAULT_TO_PANIC("Unsupported size: {#x}", buf.bytes());
            case 1:
               shadow_buf.write<uint8_t>(buf.read<const uint8_t>(), offset);
               break;
            case 2:
               shadow_buf.write<uint16_t>(buf.read<const uint16_t>(), offset);
               break;
            case 4:
               shadow_buf.write<uint32_t>(buf.read<const uint32_t>(), offset);
               break;
            case 8:
               shadow_buf.write<uint64_t>(buf.read<const uint64_t>(), offset);
               break;
         }
      }

      void write(cbl::buffer_view& buf, off_t offset, bool reacquire = true)
      {
         set_shadow(buf, offset);
         if(reacquire) {
            acquire_resource();
         }
      }
   };

}  // namespace cbl
