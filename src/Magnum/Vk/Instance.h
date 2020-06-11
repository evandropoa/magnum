#ifndef Magnum_Vk_Instance_h
#define Magnum_Vk_Instance_h
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
 * @brief Class @ref Magnum::Vk::InstanceProperties, @ref Magnum::Vk::InstanceExtension, @ref Magnum::Vk::InstanceExtensionProperties
 * @m_since_latest
 */

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/StringView.h>

#include "Magnum/Tags.h"
#include "Magnum/Vk/Handle.h"
#include "Magnum/Vk/Vk.h"
#include "Magnum/Vk/visibility.h"

namespace Magnum { namespace Vk {

namespace Implementation {
    enum: std::size_t { InstanceExtensionCount = 16 };

    /** @todo filter out GL/AL extensions also */
    CORRADE_HAS_TYPE(IsInstanceExtension, decltype(T::InstanceIndex));
}

/**
@brief Global Vulkan instance properties
@m_since_latest

Assembles static information about Vulkan version and available layers, which
is available without having to create an instance. See also
@ref InstanceExtensionProperties which contains information about extensions
available in a particular set of enabled layers.

@section Vk-InstanceProperties-thread-safety Thread safety

Constructor of this class retrieves a pointer to the
@fn_vk{EnumerateInstanceVersion} function (which is new in Vulkan 1.1) and
stores it in a global variable if it's not there already. This operation
currently isn't guarded for thread safety in any way.
*/
class MAGNUM_VK_EXPORT InstanceProperties {
    public:
        /**
         * @brief Constructor
         *
         * Calls @fn_vk{GetInstanceProcAddr} to retrieve
         * @fn_vk{EnumerateInstanceVersion} function pointer, if not already.
         * No other operation is done, version and layer information is
         * populated lazily.
         */
        explicit InstanceProperties();

        /**
         * @brief Instance version
         *
         * On Vulkan 1.0 where @fn_vk{EnumerateInstanceVersion} isn't present,
         * returns @ref Version::Vk10. Otherwise returns the version reported
         * by the driver, which includes patch information as well and thus may
         * not correspond to the predefined @ref Version enums --- use
         * @ref isVersionSupported() to check for a particular version.
         * @see @ref versionMajor(), @ref versionMinor(), @ref versionPatch(),
         *      @fn_vk_keyword{EnumerateInstanceVersion}
         */
        Version version();

        /** @brief Whether given version is supported */
        bool isVersionSupported(Version version);

        /**
         * @brief Instance layers
         *
         * A list of all layers reported by the driver. Use
         * @ref isLayerSupported() to query support of a particular layer. Note
         * that the list is sorted and thus may be different than the order in
         * which the @ref layer(), @ref layerRevision(), @ref layerVersion()
         * and @ref layerDescription() accessors return values.
         *
         * The returned views are owned by the @ref InstanceProperties instance
         * (i.e., *not* a global memory).
         * @see @fn_vk_keyword{EnumerateInstanceLayerProperties}
         */
        Containers::ArrayView<const Containers::StringView> layers();

        /**
         * @brief Whether given layer is supported
         *
         * Search complexity is @f$ \mathcal{O}(\log n) @f$ in the total layer
         * count.
         */
        bool isLayerSupported(Containers::StringView layer);

        /** @brief Count of layers reported by the driver */
        UnsignedInt layerCount();

        /**
         * @brief Layer name
         * @param id Layer index, expected to be smaller than @ref layerCount()
         *
         * The returned view is owned by the @ref InstanceProperties instance
         * (i.e., *not* a global memory).
         */
        Containers::StringView layer(UnsignedInt id);

        /**
         * @brief Layer revision
         * @param id Layer index, expected to be smaller than @ref layerCount()
         */
        UnsignedInt layerRevision(UnsignedInt id);

        /**
         * @brief Vulkan version the layer is implemented against
         * @param id Layer index, expected to be smaller than @ref layerCount()
         */
        Version layerVersion(UnsignedInt id);

        /**
         * @brief Layer description
         * @param id Layer index, expected to be smaller than @ref layerCount()
         *
         * The returned view is owned by the @ref InstanceProperties instance
         * (i.e., *not* a global memory).
         */
        Containers::StringView layerDescription(UnsignedInt id);

    private:
        MAGNUM_VK_LOCAL void populateVersion();
        MAGNUM_VK_LOCAL void populateLayers();

        Version _version{};
        Containers::Array<VkLayerProperties> _layers;
};

/**
@brief Run-time information about a Vulkan instance extension
@m_since_latest

Encapsulates runtime information about a Vulkan extension, such as name string,
minimal required Vulkan version and version in which the extension was adopted
to core.

See also the @ref Extensions namespace, which contain compile-time information
about Vulkan extensions.
*/
class MAGNUM_VK_EXPORT InstanceExtension {
    public:
        /** @brief All known instance extensions for given Vulkan version */
        static Containers::ArrayView<const InstanceExtension> extensions(Version version);

        /** @brief Internal unique extension index */
        constexpr std::size_t index() const { return _index; }

        /** @brief Minimal version required by this extension */
        constexpr Version requiredVersion() const { return _requiredVersion; }

        /** @brief Version in which this extension was adopted to core */
        constexpr Version coreVersion() const { return _coreVersion; }

        /**
         * @brief Extension string
         *
         * The returned view is a global memory.
         */
        constexpr Containers::StringView string() const { return _string; }

        /** @brief Construct from a compile-time instance extension */
        template<class E, class = typename std::enable_if<Implementation::IsInstanceExtension<E>::value>::type> constexpr InstanceExtension(const E&): _index{E::InstanceIndex}, _requiredVersion{E::requiredVersion()}, _coreVersion{E::coreVersion()}, _string{E::string()} {}

    private:
        std::size_t _index;
        Version _requiredVersion;
        Version _coreVersion;
        Containers::StringView _string;
};

/**
@brief Global Vulkan extension properties
@m_since_latest

Assembles information about extensions in a desired set of layers. See also
@ref InstanceProperties which contains information about available Vulkan
version and layers.
*/
class MAGNUM_VK_EXPORT InstanceExtensionProperties {
    public:
        /**
         * @brief Constructor
         * @param layers        Additional layers to list extensions from
         *
         * Expects that all listed layers are supported.
         * @see @ref InstanceProperties::isLayerSupported(),
         *      @fn_vk_keyword{EnumerateInstanceExtensionProperties}
         */
        explicit InstanceExtensionProperties(Containers::ArrayView<const Containers::StringView> layers = {});

        /** @overload */
        explicit InstanceExtensionProperties(std::initializer_list<Containers::StringView> layers);

        /**
         * @brief Instance extensions
         *
         * A list of all extension strings reported by the driver for all
         * layers passed to the constructor, with duplicates removed. Use
         * @ref isExtensionSupported() to query support of a particular
         * extension. Note that the list is sorted and thus may be different
         * than the order in which the @ref extension() and
         * @ref extensionRevision() accessors return values.
         *
         * The returned views are owned by the
         * @ref InstanceExtensionProperties instance (i.e., *not* a global
         * memory).
         */
        Containers::ArrayView<const Containers::StringView> extensions() const;

        /**
         * @brief Whether given extension is supported
         *
         * Accepts extensions from the @ref Extension namespace as a template
         * parameter. Use the other overloads to query support of a runtime
         * extension or a plain extension string.
         *
         * Search complexity is @f$ \mathcal{O}(\log n) @f$ in the total
         * extension count; in contrast extension queries on a created instance
         * are @f$ \mathcal{O}(1) @f$.
         * @see @ref extensionRevision()
         */
        bool isExtensionSupported(Containers::StringView extension) const;

        /** @overload */
        bool isExtensionSupported(const InstanceExtension& extension) const {
            return isExtensionSupported(extension.string());
        }

        /** @overload */
        template<class E> bool isExtensionSupported() const {
            static_assert(Implementation::IsInstanceExtension<E>::value, "expected a Vulkan instance extension");
            return isExtensionSupported(E::string());
        }

        /**
         * @brief Count of extensions reported by the driver for all layers
         *
         * The count includes potential duplicates when an extension is both
         * available globally and through a particular layer.
         */
        UnsignedInt extensionCount() const { return _extensions.size(); }

        /**
         * @brief Extension name
         * @param id Extension index, expected to be smaller than
         *      @ref extensionCount()
         *
         * The returned view is owned by the
         * @ref InstanceExtensionProperties instance (i.e., *not* a global
         * memory).
         */
        Containers::StringView extension(UnsignedInt id) const;

        /**
         * @brief Extension revision
         * @param id Extension index, expected to be smaller than
         *      @ref extensionCount()
         */
        UnsignedInt extensionRevision(UnsignedInt id) const;

        /**
         * @brief Revision of a particular extension name
         *
         * If the extension is not supported, returns @cpp 0 @ce, supported
         * extensions always have a non-zero revision. If the extension is
         * implemented by more than one layer, returns revision of the first
         * layer implementing it --- use @ref extensionRevision(UnsignedInt) const
         * to get revision of a concrete extension in a concrete layer.
         * @see @ref isExtensionSupported()
         */
        UnsignedInt extensionRevision(Containers::StringView extension) const;

        /** @overload */
        UnsignedInt extensionRevision(const InstanceExtension& extension) const {
            return extensionRevision(extension.string());
        }

        /** @overload */
        template<class E> UnsignedInt extensionRevision() const {
            static_assert(Implementation::IsInstanceExtension<E>::value, "expected a Vulkan instance extension");
            return extensionRevision(E::string());
        }

        /**
         * @brief Extension layer index
         * @param id Extension index, expected to be smaller than
         *      @ref extensionCount()
         *
         * Returns ID of the layer the extension comes from. @cpp 0 @ce is
         * global extensions, @cpp 1 @ce is the first layer passed to
         * @ref InstanceExtensionProperties(Containers::ArrayView<const Containers::StringView>)
         * and so on.
         */
        UnsignedInt extensionLayer(UnsignedInt id) const;

    private:
        Containers::Array<VkExtensionProperties> _extensions;
        std::size_t _uniqueExtensionCount;
};

}}

#endif
