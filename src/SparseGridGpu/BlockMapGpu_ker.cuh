#ifndef BLOCK_MAP_GPU_KER_CUH_
#define BLOCK_MAP_GPU_KER_CUH_

#include "util/cuda_util.hpp"
#include <cstdlib>
#include "Vector/map_vector_sparse.hpp"
#include "DataBlock.cuh"

template<typename AggregateT, unsigned int p>
using BlockTypeOf = typename std::remove_reference<typename boost::fusion::result_of::at_c<typename AggregateT::type, p>::type>::type;

template<typename AggregateT, unsigned int p>
using ScalarTypeOf = typename std::remove_reference<typename boost::fusion::result_of::at_c<typename AggregateT::type, p>::type>::type::scalarType;

template <typename AggregateT>
struct LastPOf
{
    static const unsigned int value = AggregateT::max_prop_real - 1;
};

template <typename AggregateT, unsigned int pMask>
struct InsertBlockWrapper
{
    AggregateT aggregate;

    InsertBlockWrapper() = default;

    InsertBlockWrapper(AggregateT aggregate) : aggregate(aggregate) {}

    InsertBlockWrapper(const InsertBlockWrapper<AggregateT, pMask> &other)
    {
#ifdef __NVCC__
        aggregate = other.aggregate;
#else // __NVCC__
        std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
    }

    InsertBlockWrapper<AggregateT, pMask> &operator=(const InsertBlockWrapper<AggregateT, pMask> &other)
    {
#ifdef __NVCC__
        aggregate = other.aggregate;
        return *this;
#else // __NVCC__
        std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
    }

    template <unsigned int p>
    inline auto get() -> decltype(aggregate.template get<p>())
    {
#ifdef __NVCC__
        return aggregate.template get<p>();
#else // __NVCC__
        std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
    }

    inline auto getMask() -> decltype(aggregate.template get<pMask>())
    {
#ifdef __NVCC__
        return aggregate.template get<pMask>();
#else // __NVCC__
        std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
    }
};

template<typename AggregateBlockT=aggregate<DataBlock<float, 64>>, typename indexT=int, template<typename> class layout_base=memory_traits_inte>
class BlockMapGpu_ker
{
private:
    openfpm::vector_sparse_gpu_ker<AggregateBlockT, indexT, layout_base> blockMap;

protected:
    const static unsigned char EXIST_BIT = 0;

public:
    static const unsigned int pMask = AggregateBlockT::max_prop_real - 1;
    typedef AggregateBlockT AggregateType;
    typedef InsertBlockWrapper<AggregateBlockT, pMask> InsertBlockWrapperType;

public:
    template<typename BitMaskT>
    inline static __device__ __host__ bool getBit(const BitMaskT &bitMask, unsigned char pos)
    {
        return (bitMask>>pos)&1U;
    }

    template<typename BitMaskT>
    inline static __device__ __host__ bool setBit(BitMaskT &bitMask, unsigned char pos)
    {
        return bitMask |= 1U<<pos;
    }

    template<typename BitMaskT>
    inline static __device__ __host__ bool unsetBit(BitMaskT &bitMask, unsigned char pos)
    {
        return bitMask &= !(1U<<pos);
    }

public:
    BlockMapGpu_ker(openfpm::vector_sparse_gpu_ker<AggregateBlockT, indexT, layout_base> blockMap)
            : blockMap(blockMap) {};

    template<unsigned int p>
    inline __device__ auto get(unsigned int linId) const -> ScalarTypeOf<AggregateBlockT, p>;

    template<unsigned int p>
    inline __device__ auto get(unsigned int blockId, unsigned int offset) const -> ScalarTypeOf<AggregateBlockT, p>;

    inline __device__ auto getBlock(unsigned int blockId) -> decltype(blockMap.get(0));

    template<unsigned int p>
    inline __device__ auto insert(unsigned int linId) -> ScalarTypeOf<AggregateBlockT, p>&;

    template<unsigned int p>
    inline __device__ auto insert(unsigned int blockId, unsigned int offset) -> ScalarTypeOf<AggregateBlockT, p>&;

    inline __device__ auto insertBlock(unsigned int blockId) -> decltype(blockMap.insert(0));

    inline static __device__ unsigned int getBlockId(unsigned int linId);

    inline __device__ void init()
    {
#ifdef __NVCC__
        blockMap.init();
#else // __NVCC__
        std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
    }

    inline __device__ void flush_block_insert()
    {
#ifdef __NVCC__
        blockMap.flush_block_insert();
#else // __NVCC__
        std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
    }

    template<typename BitMaskT>
    inline static __device__ bool exist(BitMaskT &bitMask)
    {
        return getBit(bitMask, EXIST_BIT);
    }

    template<typename BitMaskT>
    inline static __device__ void setExist(BitMaskT &bitMask)
    {
        setBit(bitMask, EXIST_BIT);
    }
};

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
template<unsigned int p>
inline __device__ auto BlockMapGpu_ker<AggregateBlockT, indexT, layout_base>
::get(unsigned int linId) const -> ScalarTypeOf<AggregateBlockT, p>
{
#ifdef __NVCC__
    typedef BlockTypeOf<AggregateBlockT, p> BlockT;
    unsigned int blockId = linId / BlockT::size;
    unsigned int offset = linId % BlockT::size;
    return get<p>(blockId, offset);
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
template<unsigned int p>
inline __device__ auto BlockMapGpu_ker<AggregateBlockT, indexT, layout_base>
::get(unsigned int blockId, unsigned int offset) const -> ScalarTypeOf<AggregateBlockT, p>
{
#ifdef __NVCC__
    auto &block = blockMap.template get<p>(blockId);
    return block[offset];
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
inline __device__ auto BlockMapGpu_ker<AggregateBlockT, indexT, layout_base>
::getBlock(unsigned int blockId) -> decltype(blockMap.get(0))
{
#ifdef __NVCC__
    return blockMap.get(blockId);
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
template<unsigned int p>
inline __device__ auto BlockMapGpu_ker<AggregateBlockT, indexT, layout_base>
::insert(unsigned int linId) -> ScalarTypeOf<AggregateBlockT, p>&
{
#ifdef __NVCC__
    typedef BlockTypeOf<AggregateBlockT, p> BlockT;
    unsigned int blockId = linId / BlockT::size;
    unsigned int offset = linId % BlockT::size;
    return insert<p>(blockId, offset);
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
template<unsigned int p>
inline __device__ auto BlockMapGpu_ker<AggregateBlockT, indexT, layout_base>
::insert(unsigned int blockId, unsigned int offset) -> ScalarTypeOf<AggregateBlockT, p>&
{
#ifdef __NVCC__
    auto aggregate = blockMap.insert(blockId);
    auto &block = aggregate.template get<p>();
    auto &mask = aggregate.template get<pMask>();
    setExist(mask[offset]);
    return block[offset];
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
inline __device__ auto BlockMapGpu_ker<AggregateBlockT, indexT, layout_base>
        ::insertBlock(unsigned int blockId) -> decltype(blockMap.insert(0))
{
#ifdef __NVCC__
    return blockMap.insert(blockId);
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}

template<typename AggregateBlockT, typename indexT, template<typename> class layout_base>
inline static __device__ unsigned int getBlockId(unsigned int linId)
{
#ifdef __NVCC__
    return linId / BlockTypeOf<AggregateBlockT, 0>::size;
#else // __NVCC__
    std::cout << __FILE__ << ":" << __LINE__ << " error: you are supposed to compile this file with nvcc, if you want to use it with gpu" << std::endl;
#endif // __NVCC__
}


#endif /* BLOCK_MAP_GPU_KER_CUH_ */