# Maintainer: ACE : acedmicabhishek@gmail.com

pkgname=kernel-drive
pkgver=0.1.1
pkgrel=3
pkgdesc="A high-performance modular system control app for Arch Linux"
arch=('x86_64')
url="https://github.com/acedmicabhishek/KernelDrive"
license=('MIT')
depends=('gtk4' 'libadwaita' 'polkit')
makedepends=('meson' 'ninja' 'git' 'gcc')
optdepends=('ryzenadj: Ryzen CPU Power Control'
            'stress-ng: CPU Stress Testing'
            'gpu-burn-git: GPU Stress Testing')
source=("git+$url")
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
