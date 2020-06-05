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
};

InstanceTest::InstanceTest() {
    addTests({&InstanceTest::isInstanceExtension,
              &InstanceTest::extensionConstructFromCompileTimeExtension,
              &InstanceTest::extensionExtensions});
}

void InstanceTest::isInstanceExtension() {
    CORRADE_VERIFY(Implementation::IsInstanceExtension<Extensions::KHR::get_physical_device_properties2>::value);
    CORRADE_VERIFY(!Implementation::IsInstanceExtension<Extensions::KHR::external_memory>::value);
    CORRADE_VERIFY(!Implementation::IsInstanceExtension<int>::value);
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

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::InstanceTest)
