#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

#include "graphics.hpp"
#include "tools.hpp"
#include "prim_data.hpp"

namespace fi
{
    struct Primitives : private GraphicsObject
    {
      public:
        class AttribSetter : private GraphicsObject // thread safe
        {
          private:
            std::mutex alloc_lock_;
            std::mutex copy_lock_;

          public:
            Primitives* prims_ = nullptr;
            std::vector<PrimInfo> staging_{};

            vk::DeviceSize alloc_mem(vk::DeviceSize required);
            void copy_buffer(vk::Buffer src, vk::Buffer dst, vk::BufferCopy region);

            template <typename T>
            AttribSetter& add_data(PrimInfo::Attribute attribute,
                                   const std::vector<T>& data,
                                   const std::vector<size_t>& count_per_prim = {})
            {
                uint64_t offset = size_as_uint32_t(alloc_mem(sizeof_arr(data)));
                uint64_t accumated = offset;
                for (size_t i = 0; i < staging_.size(); i++)
                {
                    staging_[i].get_attrib_ptr(attribute) = accumated;
                    if (count_per_prim.size() == staging_.size())
                    {
                        accumated += sizeof_as_uint32_t(T) * count_per_prim[i];
                    }
                }

                vk::BufferCreateInfo buffer_info{.size = sizeof_arr(data),
                                                 .usage = vk::BufferUsageFlagBits::eTransferSrc |
                                                          vk::BufferUsageFlagBits::eVertexBuffer};
                vma::AllocationCreateInfo alloc_info{.flags =
                                                         vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
                                                         vma::AllocationCreateFlagBits::eMapped,
                                                     .usage = vma::MemoryUsage::eAutoPreferHost,
                                                     .preferredFlags = vk::MemoryPropertyFlagBits::eHostCached};
                vma::AllocationInfo alloc{};
                auto allocated = allocator().createBuffer(buffer_info, alloc_info, alloc);
                memcpy(alloc.pMappedData, data.data(), buffer_info.size);
                copy_buffer(allocated.first, prims_->data_buffer_, {0, offset * sizeof(uint32_t), buffer_info.size});
                allocator().destroyBuffer(allocated.first, allocated.second);
                return *this;
            }

            AttribSetter& set_mesh_idx(const std::vector<uint32_t>& mesh_idxs);
            AttribSetter& set_material_idx(const std::vector<uint32_t>& material_idxs);
            AttribSetter& add_mesh_infos(const std::vector<MeshInfo>& mesh_infos);
            AttribSetter& add_material_infos(const std::vector<MaterialInfo>& material_infos);
            void update_gpu();
        } setter_;

      private:
        // min alignment in data_buffer is 4 byte
        vk::DeviceSize curr_size_ = 0;
        const vk::DeviceSize max_size_ = 0;
        vk::Buffer data_buffer_{}; // buffer 0
        vma::Allocation data_alloc_{};

        uint32_t prim_count_ = 0;
        const uint32_t max_prims_ = 0;
        vk::Buffer prim_buffer_{}; // buffer 1
        vk::DeviceSize draw_call_offset_ = EMPTY_L;
        vma::Allocation prim_alloc_{};
        std::byte* prim_mapping_ = nullptr;

        struct
        {
            vk::DeviceAddress data_buffer_ = 0;
            vk::DeviceAddress prim_buffer_ = 0;
        } addresses_{};

      public:
        Primitives(vk::DeviceSize max_data_size, uint32_t max_prim_count);
        ~Primitives();

        AttribSetter& add_primitives(const std::vector<vk::DrawIndexedIndirectCommand>& draw_calls);
    };
}; // namespace fi

#endif // GRAPHICS_PRIMS_HPP