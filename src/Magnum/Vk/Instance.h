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
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Containers/StringView.h>

#include "Magnum/Tags.h"
#include "Magnum/Math/BoolVector.h"
#include "Magnum/Vk/Handle.h"
#include "Magnum/Vk/Vk.h"
#include "Magnum/Vk/Vulkan.h"
#include "Magnum/Vk/visibility.h"

namespace Magnum { namespace Vk {

namespace Implementation {
    enum: std::size_t { InstanceExtensionCount = 16 };

    /** @todo filter out GL/AL extensions also */
    template<class...> class IsInstanceExtension;
    CORRADE_HAS_TYPE(IsInstanceExtension<U>, decltype(T::InstanceIndex));
    template<class T, class U, class ...Args> class IsInstanceExtension<T, U, Args...> {
        /** @todo C++17: use &&... instead of all this */
        public: enum: bool { value = IsInstanceExtension<T>::value && IsInstanceExtension<U, Args...>::value };
    };
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
         * extension count, in contrast the @ref Instance::isExtensionEnabled()
         * queries are @f$ \mathcal{O}(1) @f$.
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

/**
@brief Instance creation info
@m_since_latest

Wraps @type_vk_keyword{InstanceCreateInfo} and
@type_vk_keyword{ApplicationInfo}.
@see @ref Instance::Instance(const InstanceCreateInfo&)
*/
class MAGNUM_VK_EXPORT InstanceCreateInfo {
    public:
        /**
         * @brief Instance flag
         *
         * @see @ref Flags, @ref InstanceCreateInfo(Int, const char**, const InstanceProperties*, const InstanceExtensionProperties*, Flags)
         */
        enum class Flag: UnsignedInt {};

        /**
         * @brief Instance flags
         *
         * @see @ref InstanceCreateInfo(Int, const char**, const InstanceProperties*, const InstanceExtensionProperties*, Flags)
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Constructor
         * @param argc          Command-line argument count. Can be @cpp 0 @ce.
         * @param argv          Command-line argument values. Can be
         *      @cpp nullptr @ce.
         * @param properties    Existing @ref InstanceProperties instance for
         *      querying available Vulkan version and layers. If
         *      @cpp nullptr @ce, a new instance may be created internally if
         *      needed.
         * @param extensionProperties Existing @ref InstanceExtensionProperties
         *      instance for querying available Vulkan extensions. If
         *      @cpp nullptr @ce, a new instance may be created internally if
         *      needed.
         * @param flags         Instance flags
         *
         * The following values are pre-filled in addition to `sType`,
         * everything else is zero-filled:
         *
         * -    @cpp pApplicationInfo @ce
         * -    @cpp pApplicationInfo->engineName @ce to @cpp "Magnum" @ce
         */
        explicit InstanceCreateInfo(Int argc, const char** argv, const InstanceProperties* properties, const InstanceExtensionProperties* const extensionProperties, Flags flags = {});

        /** @overload */
        explicit InstanceCreateInfo(Int argc, char** argv, const InstanceProperties* properties, const InstanceExtensionProperties* extensionProperties, Flags flags = {}): InstanceCreateInfo{argc, const_cast<const char**>(argv), properties, extensionProperties, flags} {}

        /** @overload */
        explicit InstanceCreateInfo(Int argc, std::nullptr_t argv, const InstanceProperties* properties, const InstanceExtensionProperties* extensionProperties, Flags flags = {}): InstanceCreateInfo{argc, static_cast<const char**>(argv), properties, extensionProperties, flags} {}

        /** @overload */
        explicit InstanceCreateInfo(Int argc, const char** argv, Flags flags = {}): InstanceCreateInfo{argc, argv, nullptr, nullptr, flags} {}

        /** @overload */
        explicit InstanceCreateInfo(Int argc, char** argv, Flags flags = {}): InstanceCreateInfo{argc, const_cast<const char**>(argv), nullptr, nullptr, flags} {}

        /** @overload */
        explicit InstanceCreateInfo(Int argc, std::nullptr_t argv, Flags flags = {}): InstanceCreateInfo{argc, static_cast<const char**>(argv), nullptr, nullptr, flags} {}

        /** @overload */
        explicit InstanceCreateInfo(Flags flags = {}): InstanceCreateInfo{0, nullptr, flags} {}

        /**
         * @brief Construct without initializing the contents
         *
         * Note that not even the `sType` field is set --- the structure has to
         * be fully initialized afterwards in order to be usable.
         */
        explicit InstanceCreateInfo(NoInitT) noexcept;

        /**
         * @brief Construct from existing data
         *
         * Copies the existing values verbatim, pointers are kept unchanged
         * without taking over the ownership. Modifying the newly created
         * instance will not modify the original data or the pointed-to data.
         */
        explicit InstanceCreateInfo(const VkInstanceCreateInfo& info) noexcept;

        ~InstanceCreateInfo();

        /**
         * @brief Set application info
         *
         * Use the @ref version() helper to create the @p version value. The
         * name is @cpp nullptr @ce by default.
         */
        InstanceCreateInfo& setApplicationInfo(Containers::StringView name, Version version);

        /**
         * @brief Add enabled layers
         *
         * All listed layers are expected be supported, use
         * @ref InstanceProperties::isLayerSupported() to check for their
         * presence.
         *
         * The function makes copies of string views that are not owning or
         * null-terminated, use the @link Containers::Literals::operator""_s() @endlink
         * literal to prevent that where possible.
         */
        InstanceCreateInfo& addEnabledLayers(Containers::ArrayView<const Containers::StringView> layers);
        /** @overload */
        InstanceCreateInfo& addEnabledLayers(std::initializer_list<Containers::StringView> layers);

        /**
         * @brief Add enabled instance extensions
         *
         * All listed extensions are expected to be supported either globally
         * or in at least one of the enabled layers, use
         * @ref InstanceExtensionProperties::isExtensionSupported() to check
         * for their presence.
         *
         * The function makes copies of string views that are not owning or
         * null-terminated, use the @link Containers::Literals::operator""_s() @endlink
         * literal to prevent that where possible.
         */
        InstanceCreateInfo& addEnabledExtensions(Containers::ArrayView<const Containers::StringView> extensions);
        /** @overload */
        InstanceCreateInfo& addEnabledExtensions(std::initializer_list<Containers::StringView> extension);
        /** @overload */
        InstanceCreateInfo& addEnabledExtensions(Containers::ArrayView<const InstanceExtension> extensions);
        /** @overload */
        InstanceCreateInfo& addEnabledExtensions(std::initializer_list<InstanceExtension> extension);
        /** @overload */
        template<class ...E> InstanceCreateInfo& addEnabledExtensions() {
            static_assert(Implementation::IsInstanceExtension<E...>::value, "expected only Vulkan instance extensions");
            return addEnabledExtensions({E{}...});
        }

        /** @brief Underlying @type_vk{InstanceCreateInfo} structure */
        VkInstanceCreateInfo& operator*() { return _info; }
        /** @overload */
        const VkInstanceCreateInfo& operator*() const { return _info; }
        /** @overload */
        VkInstanceCreateInfo* operator->() { return &_info; }
        /** @overload */
        const VkInstanceCreateInfo* operator->() const { return &_info; }
        /** @overload */
        operator const VkInstanceCreateInfo*() const { return &_info; }

    private:
        friend Instance;

        VkInstanceCreateInfo _info;
        VkApplicationInfo _applicationInfo;
        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Instance
@m_since_latest

Wraps a @type_vk_keyword{Instance} and stores all instance-specific function
pointers.
@see @ref vulkan-wrapping
*/
class MAGNUM_VK_EXPORT Instance {
    public:
        /**
         * @brief Wrap existing Vulkan instance
         * @param handle        The @type_vk{Instance} handle
         * @param enabledExtensions Extensions that are assumed to be enabled
         *      on the instance
         * @param flags         Handle flags
         *
         * The @p handle is expected to be of an existing Vulkan instance.
         * The @p enabledExtensions parameter populates internal info about
         * enabled extensions and will be reflected in @ref isExtensionEnabled(),
         * among other things. If empty,
         *
         * Unlike an instance created using a constructor, the Vulkan instance
         * is by default not deleted on destruction, use @p flags for different
         * behavior.
         * @see @ref release()
         */
        static Instance wrap(VkInstance handle, Containers::ArrayView<const Containers::StringView> enabledExtensions = {}, HandleFlags flags = {});

        /** @overload */
        static Instance wrap(VkInstance handle, std::initializer_list<Containers::StringView> enabledExtensions, HandleFlags flags = {}) {
            return wrap(handle, Containers::arrayView(enabledExtensions), flags);
        }

        /**
         * @brief Default constructor
         *
         * @see @fn_vk_keyword{CreateInstance}
         */
        explicit Instance(const InstanceCreateInfo& info = InstanceCreateInfo{});

        /**
         * @brief Construct without creating the instance
         *
         * The constructed instance is equivalent to moved-from state. Useful
         * in cases where you will overwrite the instance later anyway. Move
         * another object over it to make it useful.
         */
        explicit Instance(NoCreateT);

        /** @brief Copying is not allowed */
        Instance(const Instance&) = delete;

        /** @brief Move constructor */
        Instance(Instance&& other) noexcept;

        /**
         * @brief Destructor
         *
         * Destroys associated @type_vk{Instance} object, unless the instance
         * was created using @ref wrap() without @ref HandleFlag::DestroyOnDestruction
         * specified.
         * @see @fn_vk_keyword{DestroyInstance}, @ref release()
         */
        ~Instance();

        /** @brief Copying is not allowed */
        Instance& operator=(const Instance&) = delete;

        /** @brief Move assignment */
        Instance& operator=(Instance&& other) noexcept;

        /** @brief Underlying @type_vk{Instance} handle */
        VkInstance handle() { return _handle; }
        /** @overload */
        operator VkInstance() { return _handle; }

        /** @brief Handle flags */
        HandleFlags handleFlags() const { return _flags; }

        /**
         * @brief Release the underlying Vulkan instance
         *
         * Releases ownership of the Vulkan instance and returns its handle so
         * @fn_vk{DestroyInstance} is not called on destruction. The internal
         * state is then equivalent to moved-from state.
         * @see @ref wrap()
         */
        VkInstance release();

        /**
         * @brief Whether given extension is enabled
         *
         * Accepts instance extensions from the @ref Extensions namespace,
         * listed also in the @ref vulkan-support "Vulkan support tables".
         * Search complexity is @f$ \mathcal{O}(1) @f$. Example usage:
         *
         * @snippet MagnumVk.cpp Instance-isExtensionEnabled
         *
         * Note that this returns @cpp true @ce only if given extension is
         * supported by the driver *and* it was enabled in
         * @ref InstanceCreateInfo when creating the @ref Instance. For
         * querying extension support before creating an instance use
         * @ref InstanceExtensionProperties::isExtensionSupported().
         */
        template<class E> bool isExtensionEnabled() const {
            static_assert(Implementation::IsInstanceExtension<E>::value, "expected a Vulkan instance extension");
            return _extensionStatus[E::InstanceIndex];
        }

        /** @overload */
        bool isExtensionEnabled(const InstanceExtension& extension) const {
            return _extensionStatus[extension.index()];
        }

        /**
         * @brief Instance-specific Vulkan function pointers
         *
         * Function pointers are implicitly stored per-instance, use
         * @ref populateGlobalFunctionPointers() to populate the global `vk*`
         * functions.
         */
        const FlextVkInstance& operator*() const { return _functionPointers; }
        /** @overload */
        const FlextVkInstance* operator->() const { return &_functionPointers; }

        /**
         * @brief Populate global instance-level function pointers to be used with third-party code
         *
         * Populates instance-level global function pointers so third-party
         * code is able to call global instance-level `vk*` functions:
         *
         * @snippet MagnumVk-instance.cpp global-instance-function-pointers
         *
         * @attention This operation is changing global state. You need to
         *      ensure that this function is not called simultaenously from
         *      multiple threads and code using those function points is
         *      calling them with the same instance as the one returned by
         *      @ref handle().
         */
        void populateGlobalFunctionPointers();

    private:
        VkInstance _handle;
        HandleFlags _flags;

        Math::BoolVector<Implementation::InstanceExtensionCount> _extensionStatus;

        /* This member is bigger than you might think */
        FlextVkInstance _functionPointers;

        template<class T> MAGNUM_VK_LOCAL void initialize(Containers::ArrayView<const T> enabledExtensions);
};

}}

#endif
