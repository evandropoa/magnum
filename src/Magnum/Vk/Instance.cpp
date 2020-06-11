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
#include <Corrade/Utility/Assert.h>

#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/Version.h"

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

}}
