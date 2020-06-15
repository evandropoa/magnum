#ifndef Magnum_Vk_Device_h
#define Magnum_Vk_Device_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

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
 * @brief Class @ref Magnum::Vk::Extension
 * @m_since_latest
 */

#include <Corrade/Containers/StringView.h>

#include "Magnum/Magnum.h"
#include "Magnum/Vk/Vk.h"
#include "Magnum/Vk/Vulkan.h"
#include "Magnum/Vk/visibility.h"

namespace Magnum { namespace Vk {

namespace Implementation {
    enum: std::size_t { ExtensionCount = 72 };

    /** @todo filter out GL/AL extensions also */
    template<class...> class IsExtension;
    CORRADE_HAS_TYPE(IsExtension<U>, decltype(T::Index));
    template<class T, class U, class ...Args> class IsExtension<T, U, Args...> {
        /** @todo C++17: use &&... instead of all this */
        public: enum: bool { value = IsExtension<T>::value && IsExtension<U, Args...>::value };
    };
}

/**
@brief Run-time information about a Vulkan device extension
@m_since_latest

Encapsulates runtime information about a Vulkan extension, such as name string,
minimal required Vulkan version and version in which the extension was adopted
to core.

See also the @ref Extensions namespace, which contain compile-time information
about Vulkan extensions.
*/
class MAGNUM_VK_EXPORT Extension {
    public:
        /** @brief All known device extensions for given Vulkan version */
        static Containers::ArrayView<const Extension> extensions(Version version);

        /** @brief Internal unique extension index */
        constexpr std::size_t index() const { return _index; }

        /** @brief Minimal version required by this extension */
        constexpr Version requiredVersion() const { return _requiredVersion; }

        /** @brief Version in which this extension was adopted to core */
        constexpr Version coreVersion() const { return _coreVersion; }

        /** @brief Extension string */
        constexpr Containers::StringView string() const { return _string; }

        /** @brief Construct from a compile-time device extension */
        /** @todo prohibit conversion from GL/AL extensions also? */
        template<class E, class = typename std::enable_if<Implementation::IsExtension<E>::value>::type> constexpr Extension(const E&): _index{E::Index}, _requiredVersion{E::requiredVersion()}, _coreVersion{E::coreVersion()}, _string{E::string()} {}

    private:
        std::size_t _index;
        Version _requiredVersion;
        Version _coreVersion;
        Containers::StringView _string;
};

}}

#endif
