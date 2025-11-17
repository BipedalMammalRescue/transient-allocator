#pragma once

#include <cstdint>
#include <cstdlib>

class TransientAllocator
{
private:
    enum class BlockState
    {
        Available,
        Allocated,
        Freed
    };

    struct Block 
    {
        BlockState State;
        uint32_t Length;
    };

    unsigned char* m_Storage = nullptr;
    uint32_t m_Capacity = 0;

    Block* m_RightFreeBlock = nullptr;
    Block* m_LeftFreeBlock = nullptr;

    Block* GetNextBlock(Block* self)
    {
        return (Block*)((char*)self + sizeof(Block) + self->Length);
    }

    Block* FromPayload(void* buffer)
    {
        return (Block*)((char*)buffer - sizeof(Block));
    }

    void* ToPayload(Block* header)
    {
        return (char*)header + sizeof(Block);
    }

    bool RangeCheck(void* buffer)
    {
        size_t bufferInteger = (size_t)buffer;
        size_t maxAddress = (size_t)m_Storage + m_Capacity;
        return bufferInteger < maxAddress;
    }

public:
    TransientAllocator(uint32_t totalSize)
    {
        m_Capacity = totalSize;
        m_Storage = (unsigned char*)malloc(totalSize);

        // initialize the first free block
        *((Block*)m_Storage) = {
            TransientAllocator::BlockState::Available,
            totalSize - (uint32_t)sizeof(Block)
        };
        m_RightFreeBlock = (Block*)m_Storage;

        // initialize other pointers to invalid positions
        m_LeftFreeBlock = nullptr;
    }

    ~TransientAllocator()
    {
        free(m_Storage);
    }

    void Free(void* buffer)
    {
        Block* block = FromPayload(buffer);
        block->State = BlockState::Freed;

        // consolidate rightward
        for (Block* candidate = GetNextBlock(block); RangeCheck(candidate) && candidate->State == BlockState::Freed; candidate = GetNextBlock(candidate))
        {
            block->Length += candidate->Length + sizeof(Block);
        }

        // merge rightward
        if (m_RightFreeBlock == nullptr || GetNextBlock(block) == m_RightFreeBlock)
        {
            block->Length += (m_RightFreeBlock != nullptr ? m_RightFreeBlock->Length + sizeof(Block) : 0);
            block->State = BlockState::Available;
            m_RightFreeBlock = block;
        }

        // merge leftward
        if (m_LeftFreeBlock == nullptr && block == (Block*)m_Storage)
        {
            m_LeftFreeBlock = block;
            m_LeftFreeBlock->State = BlockState::Available;
        }
        else if (m_LeftFreeBlock != nullptr && GetNextBlock(m_LeftFreeBlock) == block)
        {
            m_LeftFreeBlock->Length += block->Length + sizeof(Block);
        }

        // handle the case when the left and right blocks are merged together into one large block
        if (m_LeftFreeBlock != nullptr && m_LeftFreeBlock->Length >= m_Capacity - sizeof(Block))
        {
            m_RightFreeBlock = m_LeftFreeBlock;
            m_LeftFreeBlock = nullptr;
        }
    }

    void* Allocate(uint32_t size)
    {
        // try growing to the right side
        if (m_RightFreeBlock != nullptr && m_RightFreeBlock->Length >= size)
        {
            uint32_t leftover = m_RightFreeBlock->Length - size;
            
            // decrement the free block
            Block* allocation = m_RightFreeBlock;
            allocation->State = BlockState::Allocated;
            allocation->Length = size;

            // allocate the new rightmost block
            if (leftover <= sizeof(Block))
            {
                // in this case, merge the leftover into the new allocated block
                allocation->Length += leftover;
                m_RightFreeBlock = nullptr;
            }
            else 
            {
                m_RightFreeBlock = GetNextBlock(allocation);
                m_RightFreeBlock->Length = leftover - sizeof(Block);
                m_RightFreeBlock->State = BlockState::Available;
            }

            return ToPayload(allocation);
        }
        // try growing to the left side
        else if (m_LeftFreeBlock != nullptr && m_LeftFreeBlock->Length > size)
        {
            uint32_t leftover = m_LeftFreeBlock->Length - size;

            if (leftover <= sizeof(Block))
            {
                // destroy the left block and return it as the result
                Block* allocation = m_LeftFreeBlock;
                m_LeftFreeBlock = nullptr;

                allocation->State = BlockState::Allocated;
                return ToPayload(allocation);
            }
            else 
            {
                // shrink the left block
                m_LeftFreeBlock->Length = leftover;

                // calculate the allocation
                Block* allocation = GetNextBlock(m_LeftFreeBlock);
                
                allocation->State = BlockState::Allocated;
                return ToPayload(allocation);
            }
        }
        else
            return nullptr;
    }
};