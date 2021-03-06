/**
* \file
* Copyright 2014-2015 Benjamin Worpitz, Rene Widera
*
* This file is part of alpaka.
*
* alpaka is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* alpaka is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with alpaka.
* If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <alpaka/dev/Traits.hpp>            // dev::getDev
#include <alpaka/dim/DimIntegralConst.hpp>  // dim::DimInt<N>
#include <alpaka/extent/Traits.hpp>         // mem::view::getXXX
#include <alpaka/mem/view/Traits.hpp>       // mem::view::Set
#include <alpaka/stream/Traits.hpp>         // stream::Enqueue

#include <alpaka/core/Cuda.hpp>             // cudaMemset, ...

#include <cassert>                          // assert

namespace alpaka
{
    namespace dev
    {
        class DevCudaRt;
    }
}

namespace alpaka
{
    //-----------------------------------------------------------------------------
    // Trait specializations for Set.
    //-----------------------------------------------------------------------------
    namespace mem
    {
        namespace view
        {
            namespace cuda
            {
                namespace detail
                {
                    //#############################################################################
                    //! The CUDA memory copy trait.
                    //#############################################################################
                    template<
                        typename TDim,
                        typename TBuf,
                        typename TExtent>
                    struct TaskSet
                    {
                        //-----------------------------------------------------------------------------
                        //!
                        //-----------------------------------------------------------------------------
                        TaskSet(
                            TBuf & buf,
                            std::uint8_t const & byte,
                            TExtent const & extent) :
                                m_buf(buf),
                                m_byte(byte),
                                m_extent(extent),
                                m_iDevice(dev::getDev(buf).m_iDevice)
                        {
                            static_assert(
                                dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                                "The destination buffer and the extent are required to have the same dimensionality!");
                        }

                        TBuf & m_buf;
                        std::uint8_t const m_byte;
                        TExtent const m_extent;
                        std::int32_t const m_iDevice;
                    };
                }
            }
            namespace traits
            {
                //#############################################################################
                //! The CPU device memory set trait specialization.
                //#############################################################################
                template<
                    typename TDim>
                struct TaskSet<
                    TDim,
                    dev::DevCudaRt>
                {
                    //-----------------------------------------------------------------------------
                    //!
                    //-----------------------------------------------------------------------------
                    template<
                        typename TExtent,
                        typename TBuf>
                    ALPAKA_FN_HOST static auto taskSet(
                        TBuf & buf,
                        std::uint8_t const & byte,
                        TExtent const & extent)
                    -> mem::view::cuda::detail::TaskSet<
                        TDim,
                        TBuf,
                        TExtent>
                    {
                        return
                            mem::view::cuda::detail::TaskSet<
                                TDim,
                                TBuf,
                                TExtent>(
                                    buf,
                                    byte,
                                    extent);
                    }
                };
            }
        }
    }
    namespace stream
    {
        namespace traits
        {
            //#############################################################################
            //! The CUDA async device stream 1D set enqueue trait specialization.
            //#############################################################################
            template<
                typename TBuf,
                typename TExtent>
            struct Enqueue<
                stream::StreamCudaRtAsync,
                mem::view::cuda::detail::TaskSet<dim::DimInt<1u>, TBuf, TExtent>>
            {
                //-----------------------------------------------------------------------------
                //
                //-----------------------------------------------------------------------------
                ALPAKA_FN_HOST static auto enqueue(
                    stream::StreamCudaRtAsync & stream,
                    mem::view::cuda::detail::TaskSet<dim::DimInt<1u>, TBuf, TExtent> const & task)
                -> void
                {
                    ALPAKA_DEBUG_MINIMAL_LOG_SCOPE;

                    static_assert(
                        dim::Dim<TBuf>::value == 1u,
                        "The destination buffer is required to be 1-dimensional for this specialization!");
                    static_assert(
                        dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                        "The destination buffer and the extent are required to have the same dimensionality!");

                    auto & buf(task.m_buf);
                    auto const & byte(task.m_byte);
                    auto const & extent(task.m_extent);
                    auto const & iDevice(task.m_iDevice);

                    auto const extentWidth(extent::getWidth(extent));
                    auto const extentWidthBytes(extentWidth * sizeof(elem::Elem<TBuf>));
                    auto const dstWidth(extent::getWidth(buf));
                    auto const dstNativePtr(reinterpret_cast<void *>(mem::view::getPtrNative(buf)));
                    assert(extentWidth <= dstWidth);

                    // Set the current device.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaSetDevice(
                            iDevice));
                    // Initiate the memory set.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaMemsetAsync(
                            dstNativePtr,
                            static_cast<int>(byte),
                            extentWidthBytes,
                            stream.m_spStreamCudaRtAsyncImpl->m_CudaStream));
                }
            };
            //#############################################################################
            //! The CUDA sync device stream 1D set enqueue trait specialization.
            //#############################################################################
            template<
                typename TBuf,
                typename TExtent>
            struct Enqueue<
                stream::StreamCudaRtSync,
                mem::view::cuda::detail::TaskSet<dim::DimInt<1u>, TBuf, TExtent>>
            {
                //-----------------------------------------------------------------------------
                //
                //-----------------------------------------------------------------------------
                ALPAKA_FN_HOST static auto enqueue(
                    stream::StreamCudaRtSync & stream,
                    mem::view::cuda::detail::TaskSet<dim::DimInt<1u>, TBuf, TExtent> const & task)
                -> void
                {
                    ALPAKA_DEBUG_MINIMAL_LOG_SCOPE;

                    static_assert(
                        dim::Dim<TBuf>::value == 1u,
                        "The destination buffer is required to be 1-dimensional for this specialization!");
                    static_assert(
                        dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                        "The destination buffer and the extent are required to have the same dimensionality!");

                    auto & buf(task.m_buf);
                    auto const & byte(task.m_byte);
                    auto const & extent(task.m_extent);
                    auto const & iDevice(task.m_iDevice);

                    auto const extentWidth(extent::getWidth(extent));
                    auto const extentWidthBytes(extentWidth * sizeof(elem::Elem<TBuf>));
                    auto const dstWidth(extent::getWidth(buf));
                    auto const dstNativePtr(reinterpret_cast<void *>(mem::view::getPtrNative(buf)));
                    assert(extentWidth <= dstWidth);

                    // Set the current device.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaSetDevice(
                            iDevice));
                    // Initiate the memory set.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaMemset(
                            dstNativePtr,
                            static_cast<int>(byte),
                            extentWidthBytes));
                }
            };
            //#############################################################################
            //! The CUDA async device stream 2D set enqueue trait specialization.
            //#############################################################################
            template<
                typename TBuf,
                typename TExtent>
            struct Enqueue<
                stream::StreamCudaRtAsync,
                mem::view::cuda::detail::TaskSet<dim::DimInt<2u>, TBuf, TExtent>>
            {
                //-----------------------------------------------------------------------------
                //
                //-----------------------------------------------------------------------------
                ALPAKA_FN_HOST static auto enqueue(
                    stream::StreamCudaRtAsync & stream,
                    mem::view::cuda::detail::TaskSet<dim::DimInt<2u>, TBuf, TExtent> const & task)
                -> void
                {
                    ALPAKA_DEBUG_MINIMAL_LOG_SCOPE;

                    static_assert(
                        dim::Dim<TBuf>::value == 2u,
                        "The destination buffer is required to be 2-dimensional for this specialization!");
                    static_assert(
                        dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                        "The destination buffer and the extent are required to have the same dimensionality!");

                    auto & buf(task.m_buf);
                    auto const & byte(task.m_byte);
                    auto const & extent(task.m_extent);
                    auto const & iDevice(task.m_iDevice);

                    auto const extentWidth(extent::getWidth(extent));
                    auto const extentWidthBytes(extentWidth * sizeof(elem::Elem<TBuf>));
                    auto const extentHeight(extent::getHeight(extent));
                    auto const dstWidth(extent::getWidth(buf));
                    auto const dstHeight(extent::getHeight(buf));
                    auto const dstPitchBytes(mem::view::getPitchBytes<dim::Dim<TBuf>::value - 1u>(buf));
                    auto const dstNativePtr(reinterpret_cast<void *>(mem::view::getPtrNative(buf)));
                    assert(extentWidth <= dstWidth);
                    assert(extentHeight <= dstHeight);

                    // Set the current device.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaSetDevice(
                            iDevice));
                    // Initiate the memory set.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaMemset2DAsync(
                            dstNativePtr,
                            dstPitchBytes,
                            static_cast<int>(byte),
                            extentWidthBytes,
                            extentHeight,
                            stream.m_spStreamCudaRtAsyncImpl->m_CudaStream));
                }
            };
            //#############################################################################
            //! The CUDA sync device stream 2D set enqueue trait specialization.
            //#############################################################################
            template<
                typename TBuf,
                typename TExtent>
            struct Enqueue<
                stream::StreamCudaRtSync,
                mem::view::cuda::detail::TaskSet<dim::DimInt<2u>, TBuf, TExtent>>
            {
                //-----------------------------------------------------------------------------
                //
                //-----------------------------------------------------------------------------
                ALPAKA_FN_HOST static auto enqueue(
                    stream::StreamCudaRtSync & stream,
                    mem::view::cuda::detail::TaskSet<dim::DimInt<2u>, TBuf, TExtent> const & task)
                -> void
                {
                    ALPAKA_DEBUG_MINIMAL_LOG_SCOPE;

                    static_assert(
                        dim::Dim<TBuf>::value == 2u,
                        "The destination buffer is required to be 2-dimensional for this specialization!");
                    static_assert(
                        dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                        "The destination buffer and the extent are required to have the same dimensionality!");

                    auto & buf(task.m_buf);
                    auto const & byte(task.m_byte);
                    auto const & extent(task.m_extent);
                    auto const & iDevice(task.m_iDevice);

                    auto const extentWidth(extent::getWidth(extent));
                    auto const extentWidthBytes(extentWidth * sizeof(elem::Elem<TBuf>));
                    auto const extentHeight(extent::getHeight(extent));
                    auto const dstWidth(extent::getWidth(buf));
                    auto const dstHeight(extent::getHeight(buf));
                    auto const dstPitchBytes(mem::view::getPitchBytes<dim::Dim<TBuf>::value - 1u>(buf));
                    auto const dstNativePtr(reinterpret_cast<void *>(mem::view::getPtrNative(buf)));
                    assert(extentWidth <= dstWidth);
                    assert(extentHeight <= dstHeight);

                    // Set the current device.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaSetDevice(
                            iDevice));
                    // Initiate the memory set.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaMemset2D(
                            dstNativePtr,
                            dstPitchBytes,
                            static_cast<int>(byte),
                            extentWidthBytes,
                            extentHeight));
                }
            };
            //#############################################################################
            //! The CUDA async device stream 3D set enqueue trait specialization.
            //#############################################################################
            template<
                typename TBuf,
                typename TExtent>
            struct Enqueue<
                stream::StreamCudaRtAsync,
                mem::view::cuda::detail::TaskSet<dim::DimInt<3u>, TBuf, TExtent>>
            {
                //-----------------------------------------------------------------------------
                //
                //-----------------------------------------------------------------------------
                ALPAKA_FN_HOST static auto enqueue(
                    stream::StreamCudaRtAsync & stream,
                    mem::view::cuda::detail::TaskSet<dim::DimInt<3u>, TBuf, TExtent> const & task)
                -> void
                {
                    ALPAKA_DEBUG_MINIMAL_LOG_SCOPE;

                    static_assert(
                        dim::Dim<TBuf>::value == 3u,
                        "The destination buffer is required to be 3-dimensional for this specialization!");
                    static_assert(
                        dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                        "The destination buffer and the extent are required to have the same dimensionality!");

                    auto & buf(task.m_buf);
                    auto const & byte(task.m_byte);
                    auto const & extent(task.m_extent);
                    auto const & iDevice(task.m_iDevice);

                    auto const extentWidth(extent::getWidth(extent));
                    auto const extentHeight(extent::getHeight(extent));
                    auto const extentDepth(extent::getDepth(extent));
                    auto const dstWidth(extent::getWidth(buf));
                    auto const dstHeight(extent::getHeight(buf));
                    auto const dstDepth(extent::getDepth(buf));
                    auto const dstPitchBytes(mem::view::getPitchBytes<dim::Dim<TBuf>::value - 1u>(buf));
                    auto const dstNativePtr(reinterpret_cast<void *>(mem::view::getPtrNative(buf)));
                    assert(extentWidth <= dstWidth);
                    assert(extentHeight <= dstHeight);
                    assert(extentDepth <= dstDepth);

                    // Fill CUDA parameter structures.
                    cudaPitchedPtr const cudaPitchedPtrVal(
                        make_cudaPitchedPtr(
                            dstNativePtr,
                            dstPitchBytes,
                            dstWidth,
                            dstHeight));

                    cudaExtent const cudaExtentVal(
                        make_cudaExtent(
                            extentWidth,
                            extentHeight,
                            extentDepth));

                    // Set the current device.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaSetDevice(
                            iDevice));
                    // Initiate the memory set.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaMemset3DAsync(
                            cudaPitchedPtrVal,
                            static_cast<int>(byte),
                            cudaExtentVal,
                            stream.m_spStreamCudaRtAsyncImpl->m_CudaStream));
                }
            };
            //#############################################################################
            //! The CUDA sync device stream 3D set enqueue trait specialization.
            //#############################################################################
            template<
                typename TBuf,
                typename TExtent>
            struct Enqueue<
                stream::StreamCudaRtSync,
                mem::view::cuda::detail::TaskSet<dim::DimInt<3u>, TBuf, TExtent>>
            {
                //-----------------------------------------------------------------------------
                //
                //-----------------------------------------------------------------------------
                ALPAKA_FN_HOST static auto enqueue(
                    stream::StreamCudaRtSync & stream,
                    mem::view::cuda::detail::TaskSet<dim::DimInt<3u>, TBuf, TExtent> const & task)
                -> void
                {
                    ALPAKA_DEBUG_MINIMAL_LOG_SCOPE;

                    static_assert(
                        dim::Dim<TBuf>::value == 3u,
                        "The destination buffer is required to be 3-dimensional for this specialization!");
                    static_assert(
                        dim::Dim<TBuf>::value == dim::Dim<TExtent>::value,
                        "The destination buffer and the extent are required to have the same dimensionality!");

                    auto & buf(task.m_buf);
                    auto const & byte(task.m_byte);
                    auto const & extent(task.m_extent);
                    auto const & iDevice(task.m_iDevice);

                    auto const extentWidth(extent::getWidth(extent));
                    auto const extentHeight(extent::getHeight(extent));
                    auto const extentDepth(extent::getDepth(extent));
                    auto const dstWidth(extent::getWidth(buf));
                    auto const dstHeight(extent::getHeight(buf));
                    auto const dstDepth(extent::getDepth(buf));
                    auto const dstPitchBytes(mem::view::getPitchBytes<dim::Dim<TBuf>::value - 1u>(buf));
                    auto const dstNativePtr(reinterpret_cast<void *>(mem::view::getPtrNative(buf)));
                    assert(extentWidth <= dstWidth);
                    assert(extentHeight <= dstHeight);
                    assert(extentDepth <= dstDepth);

                    // Fill CUDA parameter structures.
                    cudaPitchedPtr const cudaPitchedPtrVal(
                        make_cudaPitchedPtr(
                            dstNativePtr,
                            dstPitchBytes,
                            dstWidth,
                            dstHeight));

                    cudaExtent const cudaExtentVal(
                        make_cudaExtent(
                            extentWidth,
                            extentHeight,
                            extentDepth));

                    // Set the current device.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaSetDevice(
                            iDevice));
                    // Initiate the memory set.
                    ALPAKA_CUDA_RT_CHECK(
                        cudaMemset3D(
                            cudaPitchedPtrVal,
                            static_cast<int>(byte),
                            cudaExtentVal));
                }
            };
        }
    }
}
