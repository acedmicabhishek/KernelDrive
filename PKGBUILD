
pkgname=kernel-drive
pkgver=1.0.0
pkgrel=1
pkgdesc="A tool for managing CPU and GPU performance on Arch Linux"
arch=('x86_64')
url="https://github.com/acedmicabhishek/KernelDrive"
license=('MIT')
depends=('sdl2' 'sdl2_ttf' 'gcc-libs' 'glibc')

source=("git+https://github.com/acedmicabhishek/KernelDrive.git" "ob.ttf")
md5sums=('SKIP' '83b8326522939aff37ae053681ca1e31') 

build() {
    cd "$srcdir/KernelDrive"
    
    mkdir -p build
    cd build
    
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make
}

package() {
    cd "$srcdir/KernelDrive/build"
    
    install -Dm755 KernelDrive "$pkgdir/usr/bin/KernelDrive"
    install -Dm644 "$srcdir/ob.ttf" "$pkgdir/usr/share/fonts/ob.ttf" 
}
