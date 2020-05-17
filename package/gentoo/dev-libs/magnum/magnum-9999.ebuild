EAPI=7

EGIT_REPO_URI="git://github.com/mosra/magnum.git"

inherit cmake git-r3

DESCRIPTION="C++11/C++14 graphics middleware for games and data visualization"
HOMEPAGE="https://magnum.graphics"

LICENSE="MIT"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

RDEPEND="
	dev-libs/corrade
	media-libs/openal
	media-libs/glfw
	media-libs/libsdl2
"
DEPEND="${RDEPEND}"

src_configure() {
	local mycmakeargs=(
		-DCMAKE_INSTALL_PREFIX="${EPREFIX}/usr"
		-DCMAKE_BUILD_TYPE=Release
		-DWITH_AUDIO=ON
		-DWITH_GLXAPPLICATION=ON
		-DWITH_GLFWAPPLICATION=ON
		-DWITH_SDL2APPLICATION=ON
		-DWITH_WINDOWLESSGLXAPPLICATION=ON
		-DWITH_EGLCONTEXT=ON
		-DWITH_GLXCONTEXT=ON
		-DWITH_OPENGLTESTER=ON
		-DWITH_ANYAUDIOIMPORTER=ON
		-DWITH_ANYIMAGECONVERTER=ON
		-DWITH_ANYIMAGEIMPORTER=ON
		-DWITH_ANYSCENECONVERTER=ON
		-DWITH_ANYSCENEIMPORTER=ON
		-DWITH_MAGNUMFONT=ON
		-DWITH_MAGNUMFONTCONVERTER=ON
		-DWITH_OBJIMPORTER=ON
		-DWITH_TGAIMAGECONVERTER=ON
		-DWITH_TGAIMPORTER=ON
		-DWITH_WAVAUDIOIMPORTER=ON
		-DWITH_DISTANCEFIELDCONVERTER=ON
		-DWITH_IMAGECONVERTER=ON
		-DWITH_SCENECONVERTER=ON
		-DWITH_FONTCONVERTER=ON
		-DWITH_GL_INFO=ON
		-DWITH_AL_INFO=ON
	)
	cmake_src_configure
}

src_install() {
	cmake_src_install
	mkdir "${ED}/usr/$(get_libdir)/magnum/"
	cp -av "${BUILD_DIR}"/Gentoo/lib/magnum/* "${ED}/usr/$(get_libdir)/magnum/" || die
}

# kate: replace-tabs off;
