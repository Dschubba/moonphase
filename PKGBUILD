pkgname=moonphase
pkgver=$(<VERSION.txt)
pkgrel=1
pkgdesc='Moon phase and moonrise calculator'
arch=('x86_64' 'aarch64')
license=('GPL-3.0-only')
makedepends=('cmake')
depends=('gcc-libs')
source=('CMakeLists.txt'
        'VERSION.txt'
        'main.cpp'
        'main.h'
        'calculations.cpp'
        'calculations.h'
        'README.md'
        'README.de.md'
        'LICENSE')
sha256sums=('51b148ee9d6c151cd2a765c1afd0bdda862112e43c580d06fee87df3d71925ac'
            '1f2110568627307becae713f8b808f3d9d912d4b2a24ed6a78605f03f7437d3c'
            '1833e85eacc268199d84e552c28116be58f3844c474f2198323a6ac4083d1291'
            '78f631fbe5c3c60114995fb4ee5d5213170328910e6880722eb91baa97a38991'
            '163a4d315faffb042a15d842c70037268ae8cc704dad9fbff091509c0f537de4'
            'daf6353d454d7be1ac20b3158a7d02c01bef6d5744a8332e816b25c6320ee48e'
            'a4adb3a9e92f8d6ae8a2b0f699d0f48e41682065c156d2cf4c7939a1874817d6'
            '9b37dc0d3471a61ba7abecde189109017f54b26b647162c82979b7e8a19db8b4'
            'd4a986deaac94e3e0dfcfa1abca1841f4fbd9e45d2db5afcf5c2b15001390634')

build() {
    cmake -S "$srcdir" -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build --parallel
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
