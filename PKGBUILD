# Maintainer: ACE : acedmicabhishek@gmail.com

pkgname=kernel-drive-git
pkgver=0.1.0.r0.g3e0affb
pkgrel=1
pkgdesc="A high-performance modular system control app for Arch Linux"
arch=('x86_64')
url="https://github.com/acedmicabhishek/KernelDrive"
license=('MIT')
depends=('gtk4' 'libadwaita' 'polkit')
optdepends=('opendoas: for doas elevation support' 'sudo: for sudo elevation support')
makedepends=('meson' 'git' 'gcc')
provides=('kernel-drive')
conflicts=('kernel-drive')
source=("git+https://github.com/acedmicabhishek/KernelDrive.git")
md5sums=('SKIP')

pkgver() {
    cd "$srcdir/KernelDrive"
    git describe --long --tags 2>/dev/null | sed 's/\([^-]*-g\)/r\1/;s/-/./g' ||
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    arch-meson KernelDrive build
    meson compile -C build
}

package() {
    meson install -C build --destdir "$pkgdir"
}
