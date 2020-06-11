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
#include "Magnum/Vk/Version.h"

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
              &InstanceVkTest::extensionPropertiesNamedExtensionRevision});
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

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::InstanceVkTest)
