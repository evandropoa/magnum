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

#include <set>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>

#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/Instance.h"

namespace Magnum { namespace Vk { namespace Test { namespace {

struct InstanceTest: TestSuite::Tester {
    explicit InstanceTest();

    void isInstanceExtension();

    void extensionConstructFromCompileTimeExtension();
    void extensionExtensions();

    void instanceCreateInfoConstructDefault();
    void instanceCreateInfoConstructNoInit();
    void instanceCreateInfoConstructFromVk();
    void instanceCreateInfoApplicationInfo();
    void instanceCreateInfoLayers();
    void instanceCreateInfoExtensions();
    void instanceCreateInfoCopiedStrings();

    void constructNoCreate();
    void constructCopy();
};

InstanceTest::InstanceTest() {
    addTests({&InstanceTest::isInstanceExtension,
              &InstanceTest::extensionConstructFromCompileTimeExtension,
              &InstanceTest::extensionExtensions,

              &InstanceTest::instanceCreateInfoConstructDefault,
              &InstanceTest::instanceCreateInfoConstructNoInit,
              &InstanceTest::instanceCreateInfoConstructFromVk,
              &InstanceTest::instanceCreateInfoApplicationInfo,
              &InstanceTest::instanceCreateInfoLayers,
              &InstanceTest::instanceCreateInfoExtensions,
              &InstanceTest::instanceCreateInfoCopiedStrings,

              &InstanceTest::constructNoCreate,
              &InstanceTest::constructCopy});
}

using namespace Containers::Literals;

void InstanceTest::isInstanceExtension() {
    CORRADE_VERIFY(Implementation::IsInstanceExtension<Extensions::KHR::get_physical_device_properties2>::value);
    CORRADE_VERIFY(!Implementation::IsInstanceExtension<Extensions::KHR::external_memory>::value);
    CORRADE_VERIFY(!Implementation::IsInstanceExtension<int>::value);

    /* Variadic check (used in variadic addEnabledExtensions()), check that it
       properly fails for each occurence of a device extension */
    CORRADE_VERIFY((Implementation::IsInstanceExtension<
        Extensions::KHR::get_physical_device_properties2,
        Extensions::KHR::external_memory_capabilities,
        Extensions::KHR::external_fence_capabilities>::value));
    CORRADE_VERIFY(!(Implementation::IsInstanceExtension<
        Extensions::KHR::draw_indirect_count, /* not */
        Extensions::KHR::external_memory_capabilities,
        Extensions::KHR::external_fence_capabilities>::value));
    CORRADE_VERIFY(!(Implementation::IsInstanceExtension<
        Extensions::KHR::get_physical_device_properties2,
        Extensions::KHR::external_memory, /* not */
        Extensions::KHR::external_fence_capabilities>::value));
    CORRADE_VERIFY(!(Implementation::IsInstanceExtension<
        Extensions::KHR::get_physical_device_properties2,
        Extensions::KHR::external_memory_capabilities,
        Extensions::KHR::external_fence>::value)); /* not */
}

void InstanceTest::extensionConstructFromCompileTimeExtension() {
    InstanceExtension a{Extensions::KHR::get_physical_device_properties2{}};
    CORRADE_COMPARE(a.index(), Extensions::KHR::get_physical_device_properties2::InstanceIndex);
    CORRADE_COMPARE(a.requiredVersion(), Extensions::KHR::get_physical_device_properties2::requiredVersion());
    CORRADE_COMPARE(a.coreVersion(), Extensions::KHR::get_physical_device_properties2::coreVersion());
    CORRADE_COMPARE(a.string(), Extensions::KHR::get_physical_device_properties2::string());

    /* Should be convertible from device extensions, but not instance exts */
    CORRADE_VERIFY((std::is_convertible<Extensions::KHR::get_physical_device_properties2, InstanceExtension>::value));
    CORRADE_VERIFY(!(std::is_convertible<Extensions::KHR::external_memory, InstanceExtension>::value));

    /* Shouldn't be convertible from strings to avoid ambiguity in APIs that
       have string/extension overloads */
    CORRADE_VERIFY(!(std::is_convertible<Containers::StringView, InstanceExtension>::value));
}

void InstanceTest::extensionExtensions() {
    Containers::StringView used[Implementation::InstanceExtensionCount]{};

    std::set<Containers::StringView> unique;

    /* Check that all extension indices are unique, are in correct lists, are
       listed just once etc. */
    for(Version version: {
        Version::Vk10,
        Version::Vk11,
        Version::Vk12,
        Version::None})
    {
        Containers::StringView previous;
        for(const InstanceExtension& e: InstanceExtension::extensions(version)) {
            CORRADE_ITERATION(e.string());

            /** @todo convert to CORRADE_ERROR() when that's done */

            /* Binary search is performed on each list to find known
               extensions, so the exts have to be sorted */
            if(!previous.isEmpty() && previous >= e.string()) {
                Error{} << "Extension not sorted after" << previous;
                CORRADE_VERIFY(false);
            }

            if(e.index() >= Implementation::InstanceExtensionCount) {
                Error{} << "Index" << e.index() << "larger than" << Implementation::InstanceExtensionCount;
                CORRADE_VERIFY(false);
            }

            if(used[e.index()] != nullptr) {
                Error{} << "Index" << e.index() << "already used by" << used[e.index()];
                CORRADE_VERIFY(false);
            }

            used[e.index()] = e.string();
            if(!unique.insert(e.string()).second) {
                Error{} << "Extension listed more than once";
                CORRADE_VERIFY(false);
            }

            CORRADE_COMPARE_AS(e.coreVersion(), e.requiredVersion(), TestSuite::Compare::GreaterOrEqual);
            if(e.coreVersion() != version) {
                Error{} << "Extension should have core version" << version << "but has" << e.coreVersion();
                CORRADE_VERIFY(false);
            }

            previous = e.string();
        }
    }

    CORRADE_VERIFY(true);
}

void InstanceTest::instanceCreateInfoConstructDefault() {
    InstanceCreateInfo info;
    CORRADE_VERIFY(info->sType);
    CORRADE_VERIFY(!info->pNext);
    CORRADE_VERIFY(!info->ppEnabledLayerNames);
    CORRADE_COMPARE(info->enabledLayerCount, 0);
    CORRADE_VERIFY(!info->ppEnabledExtensionNames);
    CORRADE_COMPARE(info->enabledExtensionCount, 0);

    CORRADE_VERIFY(info->pApplicationInfo);
    CORRADE_COMPARE(info->pApplicationInfo->apiVersion, 0);
    CORRADE_COMPARE(info->pApplicationInfo->applicationVersion, 0);
    CORRADE_COMPARE(info->pApplicationInfo->engineVersion, 0);
    CORRADE_COMPARE(info->pApplicationInfo->pEngineName, "Magnum"_s);
}

void InstanceTest::instanceCreateInfoConstructNoInit() {
    InstanceCreateInfo info;
    info->sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
    new(&info) InstanceCreateInfo{NoInit};
    CORRADE_COMPARE(info->sType, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2);

    CORRADE_VERIFY((std::is_nothrow_constructible<InstanceCreateInfo, NoInitT>::value));

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!(std::is_convertible<NoInitT, InstanceCreateInfo>::value));
}

void InstanceTest::instanceCreateInfoConstructFromVk() {
    VkInstanceCreateInfo vkInfo;
    vkInfo.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;

    InstanceCreateInfo info{vkInfo};
    CORRADE_COMPARE(info->sType, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2);

    CORRADE_VERIFY((std::is_nothrow_constructible<InstanceCreateInfo, VkInstanceCreateInfo>::value));

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!(std::is_convertible<VkInstanceCreateInfo, InstanceCreateInfo>::value));
}

void InstanceTest::instanceCreateInfoApplicationInfo() {
    Containers::StringView name = "Magnum::Vk::Test::InstanceTest"_s;

    InstanceCreateInfo info;
    CORRADE_VERIFY(info->pApplicationInfo);
    CORRADE_VERIFY(!info->pApplicationInfo->pApplicationName);
    CORRADE_COMPARE(Version(info->pApplicationInfo->applicationVersion), Version{});

    /* Setting an empty name should do nothing */
    info.setApplicationInfo({}, {});
    CORRADE_VERIFY(!info->pApplicationInfo->pApplicationName);
    CORRADE_COMPARE(Version(info->pApplicationInfo->applicationVersion), Version{});

    info.setApplicationInfo(name, version(0, 0, 1));
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->pApplicationInfo->pApplicationName), name.data());
    CORRADE_COMPARE(Version(info->pApplicationInfo->applicationVersion), version(0, 0, 1));

    /* Setting an empty view should put nullptr back */
    info.setApplicationInfo({}, {});
    CORRADE_VERIFY(!info->pApplicationInfo->pApplicationName);
    CORRADE_COMPARE(Version(info->pApplicationInfo->applicationVersion), Version{});
}

void InstanceTest::instanceCreateInfoLayers() {
    Containers::StringView layer = "VK_LAYER_KHRONOS_validation"_s;
    Containers::StringView another = "VK_LAYER_this_doesnt_exist"_s;

    InstanceCreateInfo info;
    CORRADE_VERIFY(!info->ppEnabledLayerNames);
    CORRADE_COMPARE(info->enabledLayerCount, 0);

    info.addEnabledLayers({layer});
    CORRADE_VERIFY(info->ppEnabledLayerNames);
    CORRADE_COMPARE(info->enabledLayerCount, 1);
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledLayerNames[0]), layer.data());

    info.addEnabledLayers({another, layer});
    CORRADE_COMPARE(info->enabledLayerCount, 3);
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledLayerNames[0]), layer.data());
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledLayerNames[1]), another.data());
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledLayerNames[2]), layer.data());
}

void InstanceTest::instanceCreateInfoExtensions() {
    InstanceCreateInfo info;
    CORRADE_VERIFY(!info->ppEnabledExtensionNames);
    CORRADE_COMPARE(info->enabledExtensionCount, 0);

    info.addEnabledExtensions<Extensions::KHR::external_fence_capabilities>();
    CORRADE_VERIFY(info->ppEnabledExtensionNames);
    CORRADE_COMPARE(info->enabledExtensionCount, 1);
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[0]),
        Extensions::KHR::external_fence_capabilities::string().data());

    info.addEnabledExtensions(
        {Extensions::KHR::external_semaphore_capabilities{},
         Extensions::KHR::get_physical_device_properties2{}});
    CORRADE_COMPARE(info->enabledExtensionCount, 3);
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[0]),
        Extensions::KHR::external_fence_capabilities::string().data());
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[1]),
        Extensions::KHR::external_semaphore_capabilities::string().data());
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[2]),
        Extensions::KHR::get_physical_device_properties2::string().data());
}

void InstanceTest::instanceCreateInfoCopiedStrings() {
    Containers::StringView globalButNotNullTerminated = "VK_LAYER_KHRONOS_validation3"_s.except(1);
    Containers::String localButNullTerminated = Extensions::KHR::external_memory_capabilities::string();

    InstanceCreateInfo info;
    info.setApplicationInfo(localButNullTerminated, {})
        .addEnabledLayers({globalButNotNullTerminated})
        .addEnabledExtensions({localButNullTerminated});
    CORRADE_COMPARE(info->enabledLayerCount, 1);
    CORRADE_COMPARE(info->enabledExtensionCount, 1);

    CORRADE_COMPARE(info->pApplicationInfo->pApplicationName, localButNullTerminated);
    CORRADE_VERIFY(info->pApplicationInfo->pApplicationName != localButNullTerminated.data());

    CORRADE_COMPARE(info->ppEnabledLayerNames[0], globalButNotNullTerminated);
    CORRADE_VERIFY(info->ppEnabledLayerNames[0] != globalButNotNullTerminated.data());

    CORRADE_COMPARE(info->ppEnabledExtensionNames[0], localButNullTerminated);
    CORRADE_VERIFY(info->ppEnabledExtensionNames[0] != localButNullTerminated.data());
}

void InstanceTest::constructNoCreate() {
    {
        Instance instance{NoCreate};
        CORRADE_COMPARE(instance.handle(), nullptr);
        /* Instance function pointers should be null */
        CORRADE_VERIFY(!instance->CreateDevice);
    }

    CORRADE_VERIFY(true);
}

void InstanceTest::constructCopy() {
    CORRADE_VERIFY(!(std::is_constructible<Instance, const Instance&>{}));
    CORRADE_VERIFY(!(std::is_assignable<Instance, const Instance&>{}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::InstanceTest)
