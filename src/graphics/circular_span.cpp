#include "graphics/circular_span.hpp"

fi::graphics::circular_span::circular_span(void* data, std::size_t size)
{
    reference(data, size);
}

void fi::graphics::circular_span::reference(void* data, std::size_t size)
{
    begin_ = (std::byte*)data;
    end_ = (std::byte*)data + size;
    front_ = 0;
    size_ = 0;
    while (!blocks_.empty())
    {
        blocks_.pop();
    }
}

bool fi::graphics::circular_span::push_back(std::byte* data, std::size_t new_size)
{
    if (remainning() >= new_size)
    {
        std::size_t seek0 = (front_ + size_) % capacity();
        std::size_t seek0_size = std::min(new_size, capacity() - seek0);
        std::size_t seek1_size = new_size - seek0_size;
        memcpy(begin_ + seek0, data, seek0_size);
        if (seek1_size)
        {
            memcpy(begin_, (std::byte*)data + seek0_size, seek1_size);
        }

        size_ += new_size;
        blocks_.emplace(begin_ + seek0, new_size);
        return true;
    }
    return false;
}

void fi::graphics::circular_span::pop_front()
{
    if (size_)
    {
        size_ -= blocks_.front().size_;
        front_ += blocks_.front().size_;
        front_ %= capacity();
        blocks_.pop();
    }
}

void fi::graphics::circular_span::pop_front_cleared()
{
    block& block = blocks_.front();
    std::size_t seek0_size = std::min(block.size_, capacity() - front_);
    std::size_t seek1_size = block.size_ - seek0_size;
    memset(block.data_, 0x0, seek0_size);
    if (seek1_size)
    {
        memset(block.data_ + seek0_size, 0x0, seek0_size);
    }
    pop_front();
}

bool fi::graphics::circular_span::copy_front_block_to(std::byte* dst)
{
    if (size_)
    {
        block& block = blocks_.front();
        std::size_t seek0_size = std::min(block.size_, capacity() - front_);
        std::size_t seek1_size = block.size_ - seek0_size;
        memcpy(dst, block.data_, seek0_size);
        if (seek1_size)
        {
            memcpy(dst + seek0_size, begin_, seek1_size);
        }
        return true;
    }
    return false;
}

std::array<fi::graphics::circular_span::block, 2> fi::graphics::circular_span::front_block_region()
{
    block& b = blocks_.front();
    std::array<block, 2> seeks;
    seeks[0].data_ = begin_ + front_;
    seeks[0].size_ = std::min(b.size_, capacity() - front_);
    seeks[1].data_ = begin_;
    seeks[1].size_ = b.size_ - seeks[0].size_;
    return seeks;
}
