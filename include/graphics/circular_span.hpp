#ifndef GRAPHICS_CIRCULAR_SPAN_HPP
#define GRAPHICS_CIRCULAR_SPAN_HPP

#include <queue>
#include <cstring>

#include <vulkan/vulkan.hpp>

namespace fi::graphics
{
    class circular_span
    {
      public:
        struct block
        {
            std::byte* data_ = nullptr;
            std::size_t size_ = 0;
        };

      private:
        std::byte* begin_ = nullptr;
        std::byte* end_ = nullptr;
        std::size_t front_ = 0;
        std::size_t size_ = 0;
        std::queue<block> blocks_{};

      public:
        circular_span() = default;
        circular_span(void* data, std::size_t size);
        template <typename T>
        circular_span(const T& continues_container)
            : circular_span(continues_container.data(), sizeof(continues_container[0]) * continues_container.size())
        {
        }

        void reference(void* data, std::size_t size);
        bool push_back(void* data, std::size_t new_size);
        void pop_front();
        void pop_front_cleared();
        bool copy_front_block_to(std::byte* dst);
        std::array<block, 2> front_block_region();
        std::size_t front_block_size() { return blocks_.front().size_; }
        std::size_t front() { return front_; }
        std::size_t empty() { return !size_; }
        std::size_t size() { return size_; }
        std::size_t capacity() { return end_ - begin_; }
        std::size_t remainning() { return capacity() - size(); }
    };

}; // namespace fi::graphics

#endif // GRAPHICS_CIRCULAR_SPAN_HPP
