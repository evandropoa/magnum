#ifndef Magnum_Math_PackingBatch_h
#define Magnum_Math_PackingBatch_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Functions @ref Magnum::Math::packInto(), @ref Magnum::Math::unpackInto(), @ref Magnum::Math::castInto()
 * @m_since_latest
 */

#include <Corrade/Containers/Containers.h>

#include "Magnum/Types.h"
#include "Magnum/visibility.h"

namespace Magnum { namespace Math {

/**
@{ @name Batch packing functions

These functions process an ubounded range of values, as opposed to single
vectors or scalars.
*/

/**
@brief Unpack unsigned integral values into a floating-point representation
@param[in]  src     Source integral values
@param[out] dst     Destination floating-point values
@m_since_latest

Converts integral values from full range of given *unsigned* integral type to
floating-point values in range @f$ [0, 1] @f$. Second dimension is meant to
contain vector/matrix components, or have a size of 1 for scalars. Expects that
@p src and @p dst have the same size and that the second dimension in both is
contiguous.
@see @ref packInto(), @ref castInto(),
    @ref Corrade::Containers::StridedArrayView::isContiguous()
*/
MAGNUM_EXPORT void unpackInto(const Corrade::Containers::StridedArrayView2D<const UnsignedByte>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void unpackInto(const Corrade::Containers::StridedArrayView2D<const UnsignedShort>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
@brief Unpack signed integral values into a floating-point representation
@param[in]  src     Source integral values
@param[out] dst     Destination floating-point values
@m_since_latest

Converts integral values from full range of given *signed* integral type to
floating-point values in range @f$ [-1, 1] @f$. Second dimension is meant to
contain vector/matrix components, or have a size of 1 for scalars. Expects that
@p src and @p dst have the same size and that the second dimension in both is
contiguous.
@see @ref packInto(), @ref castInto(),
    @ref Corrade::Containers::StridedArrayView::isContiguous()
*/
MAGNUM_EXPORT void unpackInto(const Corrade::Containers::StridedArrayView2D<const Byte>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void unpackInto(const Corrade::Containers::StridedArrayView2D<const Short>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
@brief Pack floating-point values into an integer representation
@param[in]  src     Source floating-point values
@param[out] dst     Destination integral values
@m_since_latest

Converts floating-point value in range @f$ [0, 1] @f$ to full range of
given *unsigned* integral type or range @f$ [-1, 1] @f$ to full range of
given *signed* integral type. Second dimension is meant to contain
vector/matrix components, or have a size of 1 for scalars. Expects that @p src
and @p dst have the same size and that the second dimension in both is
contiguous.

@attention Conversion result for floating-point numbers outside the normalized
    range is undefined.

@see @ref unpackInto(), @ref castInto(),
    @ref Corrade::Containers::StridedArrayView::isContiguous()
*/
MAGNUM_EXPORT void packInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<UnsignedByte>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void packInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<Byte>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void packInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<UnsignedShort>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void packInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<Short>& dst);

/**
@brief Cast integer values into a floating-point representation
@param[in]  src     Source integral values
@param[out] dst     Destination floating-point values
@m_since_latest

Unlike @ref packInto(), this function performs only an equivalent of
@cpp Float(a) @ce over the range, so e.g. @cpp 135 @ce becomes @cpp 135.0f @ce.
Second dimension is meant to contain vector/matrix components, or have a size
of 1 for scalars. Expects that @p src and @p dst have the same size and that
the second dimension in both is contiguous.

@attention Numbers with more than 23 bits of precision will not be represented
    accurately when cast into a @ref Magnum::Float "Float".

@see @ref castInto(const Corrade::Containers::StridedArrayView2D<const Float>&, const Corrade::Containers::StridedArrayView2D<UnsignedByte>&),
    @ref Corrade::Containers::StridedArrayView::isContiguous()
*/
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const UnsignedByte>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Byte>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const UnsignedShort>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Short>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const UnsignedInt>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Int>& src, const Corrade::Containers::StridedArrayView2D<Float>& dst);

/**
@brief Cast floating-point values into an integer representation
@param[in]  src     Source floating-point values
@param[out] dst     Destination integral values
@m_since_latest

Unlike @ref packInto(), this function performs only an equivalent of
@cpp Float(a) @ce over the range, so e.g. @cpp 135 @ce becomes @cpp 135.0f @ce.
Second dimension is meant to contain vector/matrix components, or have a size
of 1 for scalars. Expects that @p src and @p dst have the same size and that
the second dimension in both is contiguous.

@attention Numbers with more than 23 bits of precision will not be represented
    accurately when cast into a @ref Magnum::Float "Float".

@see @ref castInto(const Corrade::Containers::StridedArrayView2D<const Float>&, const Corrade::Containers::StridedArrayView2D<UnsignedByte>&),
    @ref Corrade::Containers::StridedArrayView::isContiguous()
*/
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<UnsignedByte>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<Byte>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<UnsignedShort>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<Short>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<UnsignedInt>& dst);

/**
 * @overload
 * @m_since_latest
 */
MAGNUM_EXPORT void castInto(const Corrade::Containers::StridedArrayView2D<const Float>& src, const Corrade::Containers::StridedArrayView2D<Int>& dst);

/*@}*/

}}

#endif
