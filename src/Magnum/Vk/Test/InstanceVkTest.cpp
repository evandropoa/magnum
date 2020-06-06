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
};

InstanceVkTest::InstanceVkTest() {
    addTests({&InstanceVkTest::propertiesVersion,
              &InstanceVkTest::propertiesIsVersionSupported,
              &InstanceVkTest::propertiesLayers,
              &InstanceVkTest::propertiesLayerOutOfRange,
              &InstanceVkTest::propertiesIsLayerSupported});
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

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::InstanceVkTest)
