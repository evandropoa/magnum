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

#include "Instance.h"

#include <algorithm>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/String.h>

#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/Version.h"
#include "Magnum/Vk/Implementation/Arguments.h"
#include "MagnumExternal/Vulkan/flextVkGlobal.h"

namespace Magnum { namespace Vk {

InstanceProperties::InstanceProperties() {
    /** @todo make the function pointer thread-local? make the
        vkEnumerateInstanceVersion local to the class? */
    if(!vkEnumerateInstanceVersion) flextVkInit();
}

void InstanceProperties::populateVersion() {
    /* Retrieve version */
    if(vkEnumerateInstanceVersion)
        MAGNUM_VK_INTERNAL_ASSERT_RESULT(vkEnumerateInstanceVersion(&reinterpret_cast<UnsignedInt&>(_version)));
    else _version = Version::Vk10;
}

void InstanceProperties::populateLayers() {
    /* Retrieve layer count */
    UnsignedInt count;
    MAGNUM_VK_INTERNAL_ASSERT_RESULT(vkEnumerateInstanceLayerProperties(&count, nullptr));

    /* No layers, make the array non-null so we don't call this function again
       next time */
    if(!count) {
        _layers = Containers::Array<VkLayerProperties>{
            reinterpret_cast<VkLayerProperties*>(this), 0,
            [](VkLayerProperties*, std::size_t) {}};
        return;
    }

    /* Allocate extra for a list of string views that we'll use to sort &
       search the values; query the layers */
    _layers = Containers::Array<VkLayerProperties>{
        reinterpret_cast<VkLayerProperties*>(new char[count*(sizeof(VkLayerProperties) + sizeof(Containers::StringView))]),
        count,
        [](VkLayerProperties* data, std::size_t) {
            delete[] reinterpret_cast<char*>(data);
        }};
    MAGNUM_VK_INTERNAL_ASSERT_RESULT(vkEnumerateInstanceLayerProperties(&count, reinterpret_cast<VkLayerProperties*>(_layers.data())));

    /* Expect the layer count didn't change between calls */
    CORRADE_INTERNAL_ASSERT(count == _layers.size());

    /* Populate the views and sort them so we can search in O(log n) later */
    Containers::ArrayView<Containers::StringView> layerNames{reinterpret_cast<Containers::StringView*>(_layers.end()), count};
    for(std::size_t i = 0; i != layerNames.size(); ++i)
        layerNames[i] = _layers[i].layerName;
    std::sort(layerNames.begin(), layerNames.end());
}

Version InstanceProperties::version() {
    if(_version == Version{}) populateVersion();
    return _version;
}

bool InstanceProperties::isVersionSupported(const Version version) {
    if(_version == Version{}) populateVersion();
    return version <= _version;
}

Containers::ArrayView<const Containers::StringView> InstanceProperties::layers() {
    if(!_layers) populateLayers();
    return {reinterpret_cast<const Containers::StringView*>(_layers.end()), _layers.size()};
}

bool InstanceProperties::isLayerSupported(const Containers::StringView layer) {
    if(!_layers) populateLayers();

    return std::binary_search(
        reinterpret_cast<const Containers::StringView*>(_layers.end()),
        reinterpret_cast<const Containers::StringView*>(_layers.end()) + _layers.size(),
        layer);
}

UnsignedInt InstanceProperties::layerCount() {
    if(!_layers) populateLayers();
    return UnsignedInt(_layers.size());
}

Containers::StringView InstanceProperties::layer(const UnsignedInt id) {
    if(!_layers) populateLayers();
    CORRADE_ASSERT(id < _layers.size(),
        "Vk::InstanceProperties::layer(): index" << id << "out of range for" << _layers.size() << "entries", {});
    /* Not returning the string views at the end because those are in a
       different order */
    return _layers[id].layerName;
}

UnsignedInt InstanceProperties::layerRevision(const UnsignedInt id) {
    if(!_layers) populateLayers();
    CORRADE_ASSERT(id < _layers.size(),
        "Vk::InstanceProperties::layerRevision(): index" << id << "out of range for" << _layers.size() << "entries", {});
    return _layers[id].implementationVersion;
}

Version InstanceProperties::layerVersion(const UnsignedInt id) {
    if(!_layers) populateLayers();
    CORRADE_ASSERT(id < _layers.size(),
        "Vk::InstanceProperties::layerVersion(): index" << id << "out of range for" << _layers.size() << "entries", {});
    return Version(_layers[id].specVersion);
}

Containers::StringView InstanceProperties::layerDescription(const UnsignedInt id) {
    if(!_layers) populateLayers();
    CORRADE_ASSERT(id < _layers.size(),
        "Vk::InstanceProperties::layerDescription(): index" << id << "out of range for" << _layers.size() << "entries", {});
    return _layers[id].description;
}

namespace {

/* When adding a new list, InstanceExtension::extensions() and
   Instance::initialize() needs to be adapted. Binary search is performed on
   the extensions, thus they have to be sorted alphabetically. */
constexpr InstanceExtension InstanceExtensions[] {
    Extensions::EXT::debug_report{},
    Extensions::EXT::debug_utils{},
    Extensions::EXT::validation_features{},
};
constexpr InstanceExtension InstanceExtensions11[] {
    Extensions::KHR::device_group_creation{},
    Extensions::KHR::external_fence_capabilities{},
    Extensions::KHR::external_memory_capabilities{},
    Extensions::KHR::external_semaphore_capabilities{},
    Extensions::KHR::get_physical_device_properties2{},
};
/* No Vulkan 1.2 instance extensions */

}

Containers::ArrayView<const InstanceExtension> InstanceExtension::extensions(const Version version) {
    switch(version) {
        case Version::None: return Containers::arrayView(InstanceExtensions);
        case Version::Vk10: return nullptr;
        case Version::Vk11: return Containers::arrayView(InstanceExtensions11);
        case Version::Vk12: return nullptr;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

InstanceExtensionProperties::InstanceExtensionProperties(const Containers::ArrayView<const Containers::StringView> layers) {
    /* Retrieve total extension count for all layers + the global extensions */
    std::size_t totalCount = 0;
    for(std::size_t i = 0; i <= layers.size(); ++i) {
        UnsignedInt count;
        MAGNUM_VK_INTERNAL_ASSERT_RESULT(vkEnumerateInstanceExtensionProperties(
            i == 0 ? nullptr :
                Containers::String::nullTerminatedView(layers[i - 1]).data(),
            &count, nullptr));

        totalCount += count;
    }

    /* Allocate extra for a list of string views that we'll use to sort &
       search the values and a layer index so we can map the extensions back
       to which layer they come from */
    _extensions = Containers::Array<VkExtensionProperties>{
        reinterpret_cast<VkExtensionProperties*>(new char[totalCount*(sizeof(VkExtensionProperties) + sizeof(Containers::StringView) + sizeof(UnsignedInt))]),
        totalCount,
        [](VkExtensionProperties* data, std::size_t) {
            delete[] reinterpret_cast<char*>(data);
        }};
    Containers::ArrayView<Containers::StringView> extensionNames{reinterpret_cast<Containers::StringView*>(_extensions.end()), totalCount};
    Containers::ArrayView<UnsignedInt> extensionLayers{reinterpret_cast<UnsignedInt*>(extensionNames.end()), totalCount};

    /* Query the extensions, save layer ID for each */
    std::size_t offset = 0;
    for(std::size_t i = 0; i <= layers.size(); ++i) {
        UnsignedInt count = totalCount - offset;
        MAGNUM_VK_INTERNAL_ASSERT_RESULT(vkEnumerateInstanceExtensionProperties(
            i == 0 ? nullptr :
                Containers::String::nullTerminatedView(layers[i - 1]).data(),
            &count, reinterpret_cast<VkExtensionProperties*>(_extensions.data()) + offset));
        for(std::size_t j = 0; j != count; ++j) extensionLayers[offset + j] = i;
        offset += count;
    }

    /* Expect the total extension count didn't change between calls */
    CORRADE_INTERNAL_ASSERT(offset == totalCount);

    /* Populate the views, sort them and remove duplicates so we can search in
       O(log n) later */
    for(std::size_t i = 0; i != extensionNames.size(); ++i)
        extensionNames[i] = _extensions[i].extensionName;
    std::sort(extensionNames.begin(), extensionNames.end());
    _uniqueExtensionCount = std::unique(extensionNames.begin(), extensionNames.end()) - extensionNames.begin();
}

InstanceExtensionProperties::InstanceExtensionProperties(const std::initializer_list<Containers::StringView> layers): InstanceExtensionProperties{Containers::arrayView(layers)} {}

Containers::ArrayView<const Containers::StringView> InstanceExtensionProperties::extensions() const {
    return {reinterpret_cast<const Containers::StringView*>(_extensions.end()), _uniqueExtensionCount};
}

bool InstanceExtensionProperties::isExtensionSupported(const Containers::StringView extension) const {
    return std::binary_search(
        reinterpret_cast<const Containers::StringView*>(_extensions.end()),
        reinterpret_cast<const Containers::StringView*>(_extensions.end()) + _uniqueExtensionCount,
        extension);
}

Containers::StringView InstanceExtensionProperties::extension(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _extensions.size(),
        "Vk::InstanceExtensionProperties::extension(): index" << id << "out of range for" << _extensions.size() << "entries", {});
    /* Not returning the string views at the end because those are in a
       different order */
    return _extensions[id].extensionName;
}

UnsignedInt InstanceExtensionProperties::extensionRevision(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _extensions.size(),
        "Vk::InstanceExtensionProperties::extensionRevision(): index" << id << "out of range for" << _extensions.size() << "entries", {});
    /* WTF, why VkLayerProperties::specVersion is an actual Vulkan version and
       here it is a revision number?! Consistency my ass. */
    return _extensions[id].specVersion;
}

UnsignedInt InstanceExtensionProperties::extensionRevision(const Containers::StringView extension) const {
    /* Thanks, C++, for forcing me to do one more comparison than strictly
       necessary */
    auto found = std::lower_bound(
        reinterpret_cast<const Containers::StringView*>(_extensions.end()),
        reinterpret_cast<const Containers::StringView*>(_extensions.end()) + _uniqueExtensionCount,
        extension);
    if(*found != extension) return 0;

    /* The view target is contents of the VkExtensionProperties structure,
       the revision is stored nearby */
    return reinterpret_cast<const VkExtensionProperties*>(found->data() - offsetof(VkExtensionProperties, extensionName))->specVersion;
}

UnsignedInt InstanceExtensionProperties::extensionLayer(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _extensions.size(),
        "Vk::InstanceExtensionProperties::extensionLayer(): index" << id << "out of range for" << _extensions.size() << "entries", {});
    return reinterpret_cast<const UnsignedInt*>(reinterpret_cast<const Containers::StringView*>(_extensions.end()) + _extensions.size())[id];
}

struct InstanceCreateInfo::State {
    Containers::String applicationName;
    Containers::Array<Containers::String> ownedStrings;
    Containers::Array<const char*> layers;
    Containers::Array<const char*> extensions;

    Containers::String disabledLayersStorage, disabledExtensionsStorage;
    Containers::Array<Containers::StringView> disabledLayers, disabledExtensions;
    bool verboseLog = false;
};

InstanceCreateInfo::InstanceCreateInfo(const Int argc, const char** const argv, const InstanceProperties* const properties, const InstanceExtensionProperties* const extensionProperties, const Flags flags): _info{}, _applicationInfo{} {
    Utility::Arguments args = Implementation::arguments();
    args.parse(argc, argv);

    if(args.value("log") == "verbose")
        _state.emplace().verboseLog = true;

    _info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    /** @todo filter out magnum-specific flags once there are any */
    _info.flags = VkInstanceCreateFlags(flags);
    _info.pApplicationInfo = &_applicationInfo;
    _applicationInfo.pEngineName = "Magnum";
    /** @todo magnum version? 2020 can't fit into Vulkan's version
        representation, sigh */

    /** @todo use this (enabling debug layers etc.) */
    static_cast<void>(properties);
    static_cast<void>(extensionProperties);

    /* If there are any disabled layers or extensions, sort them and save for
       later -- we'll use them to filter the ones added by the app */
    Containers::String disabledLayers = args.value<Containers::String>("disable-layers");
    Containers::String disabledExtensions = args.value<Containers::String>("disable-extensions");
    if(!disabledLayers.isEmpty()) {
        if(!_state) _state.emplace();

        _state->disabledLayersStorage = std::move(disabledLayers);
        _state->disabledLayers = Utility::String::splitWithoutEmptyParts(_state->disabledLayersStorage);
        std::sort(_state->disabledLayers.begin(), _state->disabledLayers.end());
    }
    if(!disabledExtensions.isEmpty()) {
        if(!_state) _state.emplace();

        _state->disabledExtensionsStorage = std::move(disabledExtensions);
        _state->disabledExtensions = Utility::String::splitWithoutEmptyParts(_state->disabledExtensionsStorage);
        std::sort(_state->disabledExtensions.begin(), _state->disabledExtensions.end());
    }

    /* Add all layers and extensions enabled on command-line. The blacklist is
       applied on those as well. */
    /** @todo use a generator split() so we can avoid the growing allocation
        of the output array */
    /** @todo unfortunately even though the split and value retrieval is mostly
        allocation-free, the strings will be turned into owning copies because
        none of them is null-terminated or global -- could be a better idea to
        just grow one giant string internally (once we have growable strings) */
    addEnabledLayers(Utility::String::splitWithoutEmptyParts(args.value<Containers::StringView>("enable-instance-layers")));
    addEnabledExtensions(Utility::String::splitWithoutEmptyParts(args.value<Containers::StringView>("enable-instance-extensions")));
}

InstanceCreateInfo::InstanceCreateInfo(NoInitT) noexcept {}

InstanceCreateInfo::InstanceCreateInfo(const VkInstanceCreateInfo& info) noexcept: _info{info} {}

InstanceCreateInfo::~InstanceCreateInfo() = default;

InstanceCreateInfo& InstanceCreateInfo::setApplicationInfo(const Containers::StringView name, const Version version) {
    /* Keep an owned copy of the name if it's not global / null-terminated;
       Use nullptr if the view is empty */
    if(!name.isEmpty()) {
        if(!_state) _state.emplace();

        _state->applicationName = Containers::String::nullTerminatedGlobalView(name);
        _applicationInfo.pApplicationName = _state->applicationName.data();
    } else {
        if(_state) _state->applicationName = nullptr;
        _applicationInfo.pApplicationName = nullptr;
    }

    _applicationInfo.applicationVersion = UnsignedInt(version);
    return *this;
}

InstanceCreateInfo& InstanceCreateInfo::addEnabledLayers(const Containers::ArrayView<const Containers::StringView> layers) {
    if(layers.empty()) return *this;
    if(!_state) _state.emplace();

    /* Add null-terminated strings to the layer array */
    arrayReserve(_state->layers, _state->layers.size() + layers.size());
    for(const Containers::StringView layer: layers) {
        /* If the layer is blacklisted, skip it */
        if(std::binary_search(_state->disabledLayers.begin(), _state->disabledLayers.end(), layer)) continue;

        /* Keep an owned *allocated* copy of the string if it's not global or
           null-terminated -- ideally, if people use string view literals,
           those will be, so this won't allocate. Allocated so the pointers
           don't get invalidated when the array gets reallocated. */
        const char* data;
        if(!(layer.flags() >= (Containers::StringViewFlag::NullTerminated|Containers::StringViewFlag::Global)))
            data = arrayAppend(_state->ownedStrings, Containers::InPlaceInit,
                Containers::AllocatedInit, layer).data();
        else data = layer.data();

        arrayAppend(_state->layers, data);
    }

    /* Update the layer count, re-route the pointer to the layers array in case
       it got reallocated */
    _info.enabledLayerCount = _state->layers.size();
    _info.ppEnabledLayerNames = _state->layers.data();
    return *this;
}

InstanceCreateInfo& InstanceCreateInfo::addEnabledLayers(const std::initializer_list<Containers::StringView> layers) {
    return addEnabledLayers(Containers::arrayView(layers));
}

InstanceCreateInfo& InstanceCreateInfo::addEnabledExtensions(const Containers::ArrayView<const Containers::StringView> extensions) {
    if(extensions.empty()) return *this;
    if(!_state) _state.emplace();

    /* Add null-terminated strings to the extension array */
    arrayReserve(_state->extensions, _state->extensions.size() + extensions.size());
    for(const Containers::StringView extension: extensions) {
        /* If the extension is blacklisted, skip it */
        if(std::binary_search(_state->disabledExtensions.begin(), _state->disabledExtensions.end(), extension)) continue;

        /* Keep an owned *allocated* copy of the string if it's not global or
           null-terminated -- ideally, if people use string view literals,
           those will be, so this won't allocate. Allocated so the pointers
           don't get invalidated when the array gets reallocated. */
        const char* data;
        if(!(extension.flags() >= (Containers::StringViewFlag::NullTerminated|Containers::StringViewFlag::Global)))
            data = arrayAppend(_state->ownedStrings, Containers::InPlaceInit,
                Containers::AllocatedInit, extension).data();
        else data = extension.data();

        arrayAppend(_state->extensions, data);
    }

    /* Update the extension count, re-route the pointer to the layers array in
       case it got reallocated */
    _info.enabledExtensionCount = _state->extensions.size();
    _info.ppEnabledExtensionNames = _state->extensions.data();
    return *this;
}

InstanceCreateInfo& InstanceCreateInfo::addEnabledExtensions(const std::initializer_list<Containers::StringView> extensions) {
    return addEnabledExtensions(Containers::arrayView(extensions));
}

InstanceCreateInfo& InstanceCreateInfo::addEnabledExtensions(const Containers::ArrayView<const InstanceExtension> extensions) {
    if(extensions.empty()) return *this;
    if(!_state) _state.emplace();

    arrayReserve(_state->extensions, _state->extensions.size() + extensions.size());
    for(const InstanceExtension extension: extensions) {
        /* If the extension is blacklisted, skip it */
        if(std::binary_search(_state->disabledExtensions.begin(), _state->disabledExtensions.end(), extension.string())) continue;

        arrayAppend(_state->extensions, extension.string().data());
    }

    /* Update the extension count, re-route the pointer to the layers array in
       case it got reallocated */
    _info.enabledExtensionCount = _state->extensions.size();
    _info.ppEnabledExtensionNames = _state->extensions.data();
    return *this;
}

InstanceCreateInfo& InstanceCreateInfo::addEnabledExtensions(const std::initializer_list<InstanceExtension> extensions) {
    return addEnabledExtensions(Containers::arrayView(extensions));
}

Instance Instance::wrap(const VkInstance handle, const Containers::ArrayView<const Containers::StringView> enabledExtensions, const HandleFlags flags) {
    /** @todo print enabled layers and extensions if verbose log is requested
        (how to detect that, especially here?!) */

    Instance out{NoCreate};
    out._handle = handle;
    out._flags = flags;
    out.initialize(enabledExtensions);
    return out;
}

Instance::Instance(const InstanceCreateInfo& info): _flags{HandleFlag::DestroyOnDestruction} {
    /* Print all enabled layers and extensions if verbose log is requested */
    if(info._state && info._state->verboseLog) {
        /** @todo instance version? need some non-thread-unsafe way for that */

        if(info->enabledLayerCount) {
            Debug{} << "Enabled instance layers:";
            for(std::size_t i = 0, max = info->enabledLayerCount; i != max; ++i)
                Debug{} << "   " << info->ppEnabledLayerNames[i];
        }

        if(info->enabledExtensionCount) {
            Debug{} << "Enabled instance extensions:";
            for(std::size_t i = 0, max = info->enabledExtensionCount; i != max; ++i)
                Debug{} << "   " << info->ppEnabledExtensionNames[i];
        }
    }

    MAGNUM_VK_INTERNAL_ASSERT_RESULT(vkCreateInstance(&*info, nullptr, &_handle));

    initialize<const char*>({info->ppEnabledExtensionNames, info->enabledExtensionCount});
}

template<class T> void Instance::initialize(const Containers::ArrayView<const T> enabledExtensions) {
    /* Init the function pointers */
    flextVkInitInstance(_handle, &_functionPointers);

    /* Mark all known extensions as enabled */
    for(const T extension: enabledExtensions) {
        for(Containers::ArrayView<const InstanceExtension> knownExtensions: {
            Containers::arrayView(InstanceExtensions),
            Containers::arrayView(InstanceExtensions11)
        }) {
            auto found = std::lower_bound(knownExtensions.begin(), knownExtensions.end(), extension, [](const InstanceExtension& a, const T& b) {
                return a.string() < static_cast<const Containers::StringView&>(b);
            });
            if(found->string() != extension) continue;
            _extensionStatus.set(found->index(), true);
        }
    }
}

Instance::Instance(NoCreateT): _handle{}, _functionPointers{} {}

Instance::Instance(Instance&& other) noexcept: _handle{other._handle}, _flags{other._flags}, _functionPointers{other._functionPointers} {
    other._handle = nullptr;
    other._functionPointers = {};
}

Instance::~Instance() {
    if(_handle && (_flags & HandleFlag::DestroyOnDestruction))
        _functionPointers.DestroyInstance(_handle, nullptr);
}

Instance& Instance::operator=(Instance&& other) noexcept {
    std::swap(other._handle, _handle);
    std::swap(other._flags, _flags);
    std::swap(other._functionPointers, _functionPointers);
    return *this;
}

VkInstance Instance::release() {
    const VkInstance handle = _handle;
    _handle = nullptr;
    return handle;
}

void Instance::populateGlobalFunctionPointers() {
    flextVkInstance = _functionPointers;
}

}}
