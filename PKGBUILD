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
            '800cf1c0392b24de7c0a1c6ea6778ecb433dec71c49a150bce96a98477527b2f'
            'a94049d75b881acda5641f181a2c185be362b5d4b81767594dbcb7f672b233a4'
            '78f631fbe5c3c60114995fb4ee5d5213170328910e6880722eb91baa97a38991'
            'b46f07873e258a71ecf5f137aaeb64ddec537f706e060f809f7cb4016eb3d041'
            '5f20fe97bde6d0581ad52c47c232c7c4a94601b702c37fafd7aadefc9baebbd2'
            '515fe275743c67871062ef55ea886e9e5ebbeec2eced53a6746c23ce3e13775d'
            '258070fa26980bf6a693173b947d87d3c3b28d12f5f3cad13b9c800ebedc0c82'
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
