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

#include <sstream>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/FormatStl.h>

#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/Instance.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/Version.h"

#include "MagnumExternal/Vulkan/flextVkGlobal.h"

namespace Magnum { namespace Vk { namespace Test { namespace {

struct InstanceVkTest: TestSuite::Tester {
    explicit InstanceVkTest();

    void propertiesVersion();
    void propertiesIsVersionSupported();
    void propertiesLayers();
    void propertiesLayerOutOfRange();
    void propertiesIsLayerSupported();

    void extensionPropertiesGlobal();
    void extensionPropertiesWithKhronosValidationLayer();
    void extensionPropertiesNonexistentLayer();
    void extensionPropertiesOutOfRange();
    void extensionPropertiesIsExtensionSupported();
    void extensionPropertiesNamedExtensionRevision();

    void construct();
    void constructLayerExtension();
    void constructCommandLineDisable();
    void constructCommandLineEnable();
    void constructMove();
    void constructUnknownLayer();
    void constructUnknownExtension();
    void wrap();
    void populateGlobalFunctionPointers();
};

struct {
    const char* nameDisable;
    const char* nameEnable;
    Containers::Array<const char*> argsDisable, argsEnable;
    bool debugReportEnabled, validationFeaturesEnabled;
    const char* log;
} ConstructCommandLineData[] {
    /* Shouldn't print anything about enabled layers/exts if verbose output
       isn't enabled */
    {"", "enabled layer + both extensions", nullptr,
        Containers::array({"",
            "--magnum-enable-instance-layers", "VK_LAYER_KHRONOS_validation",
            "--magnum-enable-instance-extensions", "VK_EXT_debug_report VK_EXT_validation_features"}),
        true, true,
        ""},
    /* Only with verbose log */
    {"verbose", "enabled layer + both extensions, verbose",
        Containers::array({"", "--magnum-log", "verbose"}),
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-enable-instance-layers", "VK_LAYER_KHRONOS_validation",
            "--magnum-enable-instance-extensions", "VK_EXT_debug_report VK_EXT_validation_features"}),
        true, true,
        "Enabled instance layers:\n"
        "    VK_LAYER_KHRONOS_validation\n"
        "Enabled instance extensions:\n"
        "    VK_EXT_debug_report\n"
        "    VK_EXT_validation_features\n"},
    {"disabled layer + layer-only extension", "enabled extension",
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-disable-layers", "VK_LAYER_KHRONOS_validation",
            "--magnum-disable-extensions", "VK_EXT_validation_features"}),
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-enable-instance-extensions", "VK_EXT_debug_report"}),
        true, false,
        "Enabled instance extensions:\n"
        "    VK_EXT_debug_report\n"},
    {"disabled extension", "enabled layer + one extension",
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-disable-extensions", "VK_EXT_debug_report"}),
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-enable-instance-layers", "VK_LAYER_KHRONOS_validation",
            "--magnum-enable-instance-extensions", "VK_EXT_validation_features"}),
        false, true,
        "Enabled instance layers:\n"
        "    VK_LAYER_KHRONOS_validation\n"
        "Enabled instance extensions:\n"
        "    VK_EXT_validation_features\n"},
    {"disabled extensions + layer", "verbose",
        Containers::array({"", "--magnum-log", "verbose"
            "--magnum-disable-layers", "VK_LAYER_KHRONOS_validation",
            "--magnum-disable-extensions", "VK_EXT_debug_report VK_EXT_validation_features"}),
        Containers::array({"", "--magnum-log", "verbose"}),
        false, false,
        ""},
};

InstanceVkTest::InstanceVkTest() {
    addTests({&InstanceVkTest::propertiesVersion,
              &InstanceVkTest::propertiesIsVersionSupported,
              &InstanceVkTest::propertiesLayers,
              &InstanceVkTest::propertiesLayerOutOfRange,
              &InstanceVkTest::propertiesIsLayerSupported,

              &InstanceVkTest::extensionPropertiesGlobal,
              &InstanceVkTest::extensionPropertiesWithKhronosValidationLayer,
              &InstanceVkTest::extensionPropertiesNonexistentLayer,
              &InstanceVkTest::extensionPropertiesOutOfRange,
              &InstanceVkTest::extensionPropertiesIsExtensionSupported,
              &InstanceVkTest::extensionPropertiesNamedExtensionRevision,

              &InstanceVkTest::construct,
              &InstanceVkTest::constructLayerExtension});

    addInstancedTests({&InstanceVkTest::constructCommandLineDisable,
                       &InstanceVkTest::constructCommandLineEnable},
        Containers::arraySize(ConstructCommandLineData));

    addTests({&InstanceVkTest::constructMove,
              &InstanceVkTest::constructUnknownLayer,
              &InstanceVkTest::constructUnknownExtension,
              &InstanceVkTest::wrap,
              &InstanceVkTest::populateGlobalFunctionPointers});
}

using namespace Containers::Literals;

void InstanceVkTest::propertiesVersion() {
    InstanceProperties properties;
    Debug{} << "Available version:" << properties.version();
    CORRADE_COMPARE_AS(properties.version(), Version::Vk10,
        TestSuite::Compare::GreaterOrEqual);
}

void InstanceVkTest::propertiesIsVersionSupported() {
    InstanceProperties properties;
    CORRADE_VERIFY(properties.isVersionSupported(Version::Vk10));
    CORRADE_VERIFY(!properties.isVersionSupported(version(2, 0, 0)));
}

void InstanceVkTest::propertiesLayers() {
    InstanceProperties properties;

    if(!properties.layerCount())
        CORRADE_SKIP("The driver reported no layers, can't test.");

    CORRADE_COMPARE(properties.layerCount(), properties.layers().size());
    Debug{} << "Available layers:" << properties.layers();

    /* Verify also that lazy loading works in every case */
    {
        InstanceProperties properties;
        Containers::ArrayView<const Containers::StringView> layers = properties.layers();
        CORRADE_COMPARE_AS(layers.size(), 0, TestSuite::Compare::Greater);
        /* The list should be sorted */
        for(std::size_t i = 1; i != layers.size(); ++i) {
            CORRADE_COMPARE_AS(layers[i - 1], layers[i],
                TestSuite::Compare::Less);
        }
    } {
        InstanceProperties properties;
        CORRADE_COMPARE_AS(properties.layer(0).size(), "VK_LAYER_"_s.size(),
            TestSuite::Compare::Greater);
    } {
        InstanceProperties properties;
        CORRADE_COMPARE_AS(properties.layerRevision(0), 0,
            TestSuite::Compare::Greater);
    } {
        InstanceProperties properties;
        CORRADE_COMPARE_AS(properties.layerVersion(0), Version::Vk10,
            TestSuite::Compare::GreaterOrEqual);
    } {
        InstanceProperties properties;
        CORRADE_COMPARE_AS(properties.layerDescription(0).size(), 10,
            TestSuite::Compare::Greater);
    }
}

void InstanceVkTest::propertiesLayerOutOfRange() {
    InstanceProperties properties;
    const UnsignedInt count = properties.layerCount();

    std::ostringstream out;
    Error redirectError{&out};
    properties.layer(count);
    properties.layerRevision(count);
    properties.layerVersion(count);
    properties.layerDescription(count);
    CORRADE_COMPARE(out.str(), Utility::formatString(
        "Vk::InstanceProperties::layer(): index {0} out of range for {0} entries\n"
        "Vk::InstanceProperties::layerRevision(): index {0} out of range for {0} entries\n"
        "Vk::InstanceProperties::layerVersion(): index {0} out of range for {0} entries\n"
        "Vk::InstanceProperties::layerDescription(): index {0} out of range for {0} entries\n", count));
}

void InstanceVkTest::propertiesIsLayerSupported() {
    InstanceProperties properties;

    CORRADE_VERIFY(!properties.isLayerSupported("this layer doesn't exist"));

    if(!properties.layerCount())
        CORRADE_SKIP("The driver reported no layers, can't fully test.");

    for(UnsignedInt i = 0; i != properties.layerCount(); ++i) {
        CORRADE_ITERATION(properties.layer(i));
        CORRADE_VERIFY(properties.isLayerSupported(properties.layer(i)));
    }

    /* Verify that we're not just comparing a prefix */
    const std::string layer = std::string(properties.layer(0)) + "_hello";
    CORRADE_VERIFY(!properties.isLayerSupported(layer));
}

void InstanceVkTest::extensionPropertiesGlobal() {
    InstanceExtensionProperties properties;
    Debug{} << "Available instance extension count:" << properties.extensions().size();

    CORRADE_COMPARE_AS(properties.extensionCount(), 0, TestSuite::Compare::Greater);
    for(std::size_t i = 0; i != properties.extensionCount(); ++i) {
        using namespace Containers::Literals;
        CORRADE_ITERATION(properties.extension(i));
        CORRADE_COMPARE_AS(properties.extension(i).size(), "VK_"_s.size(),
            TestSuite::Compare::Greater);
        CORRADE_COMPARE_AS(properties.extensionRevision(i), 0,
            TestSuite::Compare::Greater);
        /* All extensions are from the global layer */
        CORRADE_COMPARE(properties.extensionLayer(i), 0);
    }

    /* The extension list should be sorted and unique (so Less, not
       LessOrEqual) */
    Containers::ArrayView<const Containers::StringView> extensions = properties.extensions();
    for(std::size_t i = 1; i != extensions.size(); ++i) {
        CORRADE_COMPARE_AS(extensions[i - 1], extensions[i],
            TestSuite::Compare::Less);
    }
}

void InstanceVkTest::extensionPropertiesWithKhronosValidationLayer() {
    if(!InstanceProperties{}.isLayerSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");

    /* There should be more extensions with this layer enabled */
    InstanceExtensionProperties global;
    InstanceExtensionProperties withKhronosValidation{"VK_LAYER_KHRONOS_validation"};
    CORRADE_COMPARE_AS(global.extensionCount(),
        withKhronosValidation.extensionCount(),
        TestSuite::Compare::Less);

    /* The extension list should be sorted even including the extra layers, and
       unique (so Less, not LessOrEqual) */
    Containers::ArrayView<const Containers::StringView> extensions = withKhronosValidation.extensions();
    for(std::size_t i = 1; i != extensions.size(); ++i) {
        CORRADE_COMPARE_AS(extensions[i - 1], extensions[i],
            TestSuite::Compare::Less);
    }

    /* The VK_LAYER_KHRONOS_validation adds extensions that are supported
       globally, which means extensionCount() should be larger than
       extensions.size() as it has some duplicates */
    CORRADE_COMPARE_AS(withKhronosValidation.extensionCount(), extensions.size(),
        TestSuite::Compare::Greater);

    /* The last extension should be from the validation layer */
    CORRADE_COMPARE(withKhronosValidation.extensionLayer(0), 0);
    CORRADE_COMPARE(withKhronosValidation.extensionLayer(withKhronosValidation.extensionCount() - 1), 1);

    /* VK_EXT_validation_features is only in the layer */
    CORRADE_VERIFY(!global.isExtensionSupported("VK_EXT_validation_features"));
    CORRADE_VERIFY(withKhronosValidation.isExtensionSupported("VK_EXT_validation_features"));
}

void InstanceVkTest::extensionPropertiesNonexistentLayer() {
    CORRADE_SKIP("Currently this hits an internal assert, which can't be tested.");

    std::ostringstream out;
    Error redirectError{&out};
    InstanceExtensionProperties{"VK_LAYER_this_doesnt_exist"};
    CORRADE_COMPARE(out.str(), "TODO");
}

void InstanceVkTest::extensionPropertiesOutOfRange() {
    InstanceExtensionProperties properties;
    const UnsignedInt count = properties.extensionCount();

    std::ostringstream out;
    Error redirectError{&out};
    properties.extension(count);
    properties.extensionRevision(count);
    CORRADE_COMPARE(out.str(), Utility::formatString(
        "Vk::InstanceExtensionProperties::extension(): index {0} out of range for {0} entries\n"
        "Vk::InstanceExtensionProperties::extensionRevision(): index {0} out of range for {0} entries\n", count));
}

void InstanceVkTest::extensionPropertiesIsExtensionSupported() {
    InstanceExtensionProperties properties;
    CORRADE_COMPARE_AS(properties.extensionCount(), 0, TestSuite::Compare::Greater);

    for(UnsignedInt i = 0; i != properties.extensionCount(); ++i) {
        CORRADE_ITERATION(properties.extension(i));
        CORRADE_VERIFY(properties.isExtensionSupported(properties.extension(i)));
    }

    CORRADE_VERIFY(!properties.isExtensionSupported("this extension doesn't exist"));

    /* Verify that we're not just comparing a prefix */
    const std::string extension = std::string(properties.extension(0)) + "_hello";
    CORRADE_VERIFY(!properties.isExtensionSupported(extension));

    /* This extension should be available almost always */
    if(!properties.isExtensionSupported("VK_KHR_get_physical_device_properties2"))
        CORRADE_SKIP("VK_KHR_get_physical_device_properties2 not supported, can't fully test");

    /* Verify the overloads that take our extension wrappers work as well */
    CORRADE_VERIFY(properties.isExtensionSupported<Extensions::KHR::get_physical_device_properties2>());
    CORRADE_VERIFY(properties.isExtensionSupported(Extensions::KHR::get_physical_device_properties2{}));
}

void InstanceVkTest::extensionPropertiesNamedExtensionRevision() {
    InstanceExtensionProperties properties;
    /** @todo use Extensions::KHR::surface once the extension is recognized */
    if(!properties.isExtensionSupported("VK_KHR_surface"))
        CORRADE_SKIP("VK_KHR_surface not supported, can't test");
    if(!properties.isExtensionSupported<Extensions::KHR::get_physical_device_properties2>())
        CORRADE_SKIP("VK_KHR_get_physical_device_properties2 not supported, can't test");

    /* It was at revision 25 in January 2016, which is four months before
       Vulkan first came out, so it's safe to assume all drivers have this
       revision by now */
    CORRADE_COMPARE_AS(properties.extensionRevision("VK_KHR_surface"), 25,
        TestSuite::Compare::GreaterOrEqual);

    /* Unknown extensions return 0 */
    CORRADE_COMPARE(properties.extensionRevision("VK_this_doesnt_exist"), 0);

    /* Verify the overloads that take our extension wrappers work as well */
    CORRADE_COMPARE_AS(properties.extensionRevision<Extensions::KHR::get_physical_device_properties2>(), 0,
        TestSuite::Compare::Greater);
    CORRADE_COMPARE_AS(properties.extensionRevision(Extensions::KHR::get_physical_device_properties2{}), 0,
        TestSuite::Compare::Greater);
}

void InstanceVkTest::construct() {
    {
        Instance instance;
        CORRADE_VERIFY(instance.handle());
        /* Instance function pointers should be populated */
        CORRADE_VERIFY(instance->CreateDevice);
        CORRADE_COMPARE(instance.handleFlags(), HandleFlag::DestroyOnDestruction);
        /* No extensions are enabled by default ... */
        CORRADE_VERIFY(!instance.isExtensionEnabled<Extensions::EXT::debug_report>());
        /* ... and thus also no function pointers loaded */
        CORRADE_VERIFY(!instance->CreateDebugReportCallbackEXT);
    }

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void InstanceVkTest::constructLayerExtension() {
    if(!InstanceProperties{}.isLayerSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");
    if(!InstanceExtensionProperties{"VK_LAYER_KHRONOS_validation"}.isExtensionSupported<Extensions::EXT::debug_report>())
        CORRADE_SKIP("VK_EXT_debug_report not supported, can't test");

    Instance instance{InstanceCreateInfo{}
        .setApplicationInfo("InstanceVkTest", version(0, 0, 1))
        .addEnabledLayers({"VK_LAYER_KHRONOS_validation"_s})
        .addEnabledExtensions({
            Extensions::EXT::debug_report::string(),
            "VK_EXT_validation_features"_s
        })};
    CORRADE_VERIFY(instance.handle());

    /* Extensions should be reported as enabled ... */
    CORRADE_VERIFY(instance.isExtensionEnabled<Extensions::EXT::debug_report>());
    CORRADE_VERIFY(instance.isExtensionEnabled(Extensions::EXT::validation_features{}));
    /* ... and function pointers loaded */
    CORRADE_VERIFY(instance->CreateDebugReportCallbackEXT);
    /* no entrypoints to test for EXT_validation_features */
}

void InstanceVkTest::constructCommandLineDisable() {
    auto&& data = ConstructCommandLineData[testCaseInstanceId()];
    setTestCaseDescription(data.nameDisable);

    if(!InstanceProperties{}.isLayerSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");
    if(!InstanceExtensionProperties{"VK_LAYER_KHRONOS_validation"}.isExtensionSupported<Extensions::EXT::validation_features>())
        CORRADE_SKIP("VK_EXT_validation_features not supported, can't test");

    InstanceCreateInfo info{Int(data.argsDisable.size()), data.argsDisable};
    info.setApplicationInfo("InstanceVkTest", version(0, 0, 1))
        .addEnabledLayers({"VK_LAYER_KHRONOS_validation"_s})
        .addEnabledExtensions<Extensions::EXT::debug_report,
            Extensions::EXT::validation_features>();

    std::ostringstream out;
    Debug redirectOutput{&out};
    Instance instance{info};
    CORRADE_VERIFY(instance.handle());
    CORRADE_COMPARE(instance.isExtensionEnabled<Extensions::EXT::debug_report>(), data.debugReportEnabled);
    CORRADE_COMPARE(instance.isExtensionEnabled<Extensions::EXT::validation_features>(), data.validationFeaturesEnabled);
    CORRADE_COMPARE(out.str(), data.log);

    /* Verify that the entrypoint is actually (not) loaded as expected, to
       avoid all the above reporting being just smoke & mirrors */
    CORRADE_COMPARE(!!instance->CreateDebugReportCallbackEXT, data.debugReportEnabled);
}

void InstanceVkTest::constructCommandLineEnable() {
    auto&& data = ConstructCommandLineData[testCaseInstanceId()];
    setTestCaseDescription(data.nameEnable);

    if(!InstanceProperties{}.isLayerSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");
    if(!InstanceExtensionProperties{"VK_LAYER_KHRONOS_validation"}.isExtensionSupported<Extensions::EXT::validation_features>())
        CORRADE_SKIP("VK_EXT_validation_features not supported, can't test");

    InstanceCreateInfo info{Int(data.argsEnable.size()), data.argsEnable};
    /* Nothing enabled by the application */

    std::ostringstream out;
    Debug redirectOutput{&out};
    Instance instance{info};
    CORRADE_VERIFY(instance.handle());
    CORRADE_COMPARE(instance.isExtensionEnabled<Extensions::EXT::debug_report>(), data.debugReportEnabled);
    CORRADE_COMPARE(instance.isExtensionEnabled<Extensions::EXT::validation_features>(), data.validationFeaturesEnabled);
    CORRADE_COMPARE(out.str(), data.log);

    /* Verify that the entrypoint is actually (not) loaded as expected, to
       avoid all the above reporting being just smoke & mirrors */
    CORRADE_COMPARE(!!instance->CreateDebugReportCallbackEXT, data.debugReportEnabled);
}

void InstanceVkTest::constructMove() {
    Instance a{InstanceCreateInfo{}
        .setApplicationInfo("InstanceVkTest", version(0, 0, 1))};
    CORRADE_VERIFY(a.handle());
    CORRADE_COMPARE(a.handleFlags(), HandleFlag::DestroyOnDestruction);
    CORRADE_VERIFY(a->CreateDevice);

    Instance b = std::move(a);
    CORRADE_VERIFY(!a.handle());
    CORRADE_COMPARE(b.handleFlags(), HandleFlag::DestroyOnDestruction);
    CORRADE_VERIFY(b.handle());
    /* Function pointers in a are left in whatever state they were before, as
       that doesn't matter */
    CORRADE_VERIFY(b->CreateDevice);

    Instance c{NoCreate};
    c = std::move(b);
    CORRADE_VERIFY(!b.handle());
    CORRADE_COMPARE(b.handleFlags(), HandleFlags{});
    CORRADE_COMPARE(c.handleFlags(), HandleFlag::DestroyOnDestruction);
    CORRADE_VERIFY(c.handle());
    /* Everything is swapped, including function pointers */
    CORRADE_VERIFY(!b->CreateDevice);
    CORRADE_VERIFY(c->CreateDevice);
}

void InstanceVkTest::constructUnknownLayer() {
    CORRADE_SKIP("Currently this hits an internal assert, which can't be tested.");

    std::ostringstream out;
    Error redirectError{&out};
    Instance instance{InstanceCreateInfo{}
        .addEnabledLayers({"VK_LAYER_this_doesnt_exist"_s})};
    CORRADE_COMPARE(out.str(), "TODO");
}

void InstanceVkTest::constructUnknownExtension() {
    CORRADE_SKIP("Currently this hits an internal assert, which can't be tested.");

    std::ostringstream out;
    Error redirectError{&out};
    Instance instance{InstanceCreateInfo{}
        .addEnabledExtensions({"VK_this_doesnt_exist"_s})};
    CORRADE_COMPARE(out.str(), "TODO");
}

void InstanceVkTest::wrap() {
    InstanceExtensionProperties properties;
    if(!properties.isExtensionSupported<Extensions::EXT::debug_report>())
        CORRADE_SKIP("VK_EXT_debug_report not supported, can't test");
    if(!properties.isExtensionSupported<Extensions::KHR::get_physical_device_properties2>())
        CORRADE_SKIP("VK_KHR_get_physical_device_properties2 not supported, can't test");

    InstanceCreateInfo info;
    info.addEnabledExtensions<
        Extensions::EXT::debug_report,
        Extensions::KHR::get_physical_device_properties2>();

    VkInstance instance;
    CORRADE_COMPARE(Result(vkCreateInstance(info, nullptr, &instance)), Result::Success);
    CORRADE_VERIFY(instance);

    {
        /* Wrapping should load the basic function pointers */
        auto wrapped = Instance::wrap(instance, {
            Extensions::EXT::debug_report::string()
        }, HandleFlag::DestroyOnDestruction);
        CORRADE_VERIFY(wrapped->DestroyInstance);

        /* Listed extensions should be reported as enabled and function
           pointers loaded as well */
        CORRADE_VERIFY(wrapped.isExtensionEnabled<Extensions::EXT::debug_report>());
        CORRADE_VERIFY(wrapped->CreateDebugReportCallbackEXT);

        /* Unlisted not, but function pointers should still be loaded as the
           actual instance does have the extension enabled */
        CORRADE_VERIFY(!wrapped.isExtensionEnabled<Extensions::KHR::get_physical_device_properties2>());
        CORRADE_VERIFY(wrapped->GetPhysicalDeviceProperties2KHR);

        /* Releasing won't destroy anything ... */
        CORRADE_COMPARE(wrapped.release(), instance);
    }

    /* ...so we can wrap it again, non-owned, and then destroy it manually */
    auto wrapped = Instance::wrap(instance);
    CORRADE_VERIFY(wrapped->DestroyInstance);
    wrapped->DestroyInstance(instance, nullptr);
}

void InstanceVkTest::populateGlobalFunctionPointers() {
    vkDestroyInstance = nullptr;

    Instance instance;
    CORRADE_VERIFY(!vkDestroyInstance);
    instance.populateGlobalFunctionPointers();
    CORRADE_VERIFY(vkDestroyInstance);
    CORRADE_VERIFY(vkDestroyInstance == instance->DestroyInstance);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::InstanceVkTest)
