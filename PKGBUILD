pkgname=moonphase
pkgver=$(<VERSION)
pkgrel=1
pkgdesc='Moon phase and moonrise calculator'
arch=('x86_64' 'aarch64')
license=('GPL-3.0-only')
makedepends=('cmake')
depends=('gcc-libs')
source=('CMakeLists.txt'
        'VERSION'
        'main.cpp'
        'main.h'
        'calculations.cpp'
        'calculations.h'
        'README.md'
        'LICENSE')
sha256sums=('SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP')

build() {
    cmake -S "$srcdir" -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build --parallel
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
