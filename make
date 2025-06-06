#!/bin/sh

CC="${CC-cc}"
CFLAGS='-std=c99 -Iinclude -Iinclude/core -Wall -Wextra -Wpedantic -O3'
LDLIBS="$LDLIBS"
PACKAGES='x11 xrandr xcursor xft fontconfig'
PREFIX="${PREFIX-}"
SOURCES=src/utility/*.c\ src/*.c\ src/x11/*.c\ src/parse/*.c

usage() {
    echo "Usage: $0 TARGET..."
    echo 'Targets: fensterchef install uninstall'
    echo 'Variables: CC PREFIX'
    exit $1
} >&2

_command() {
    echo "$@" >&2 && "$@" || exit 1
}

fensterchef() {
    CFLAGS="$CFLAGS `pkg-config --cflags $PACKAGES`"
    LDLIBS="$LDLIBS `pkg-config --libs $PACKAGES`"
    _command "$CC" $LDFLAGS $CFLAGS $SOURCES -o fensterchef $LDLIBS
}

install() {
    [ -f fensterchef ] && echo 'fensterchef has been built already' >&2 || fensterchef
    _command mkdir -p "$PREFIX/usr/share/licenses/fensterchef"
    _command cp LICENSE.txt "$PREFIX/usr/share/licenses/fensterchef"
    _command mkdir -p "$PREFIX/usr/bin"
    _command mv fensterchef "$PREFIX/usr/bin"
    _command mkdir -p "$PREFIX/usr/share/man/man1"
    _command gzip --best -c doc/fensterchef.1 >"$PREFIX/usr/share/man/man1/fensterchef.1.gz"
    _command mkdir -p "$PREFIX/usr/share/man/man5"
    _command gzip --best -c doc/fensterchef.5 >"$PREFIX/usr/share/man/man5/fensterchef.5.gz"
    _command mkdir -p "$PREFIX/usr/share/fensterchef"
    _command cp wm "$PREFIX/usr/share/fensterchef"
}

uninstall() {
    for d in "$PREFIX/usr/share/licenses/fensterchef/" \
             "$PREFIX/usr/share/fensterchef" ; do
        [ -d "$d" ] && _command rm -rf "$d"
    done

    for f in "$PREFIX/usr/bin/fensterchef" \
             "$PREFIX/usr/share/man/man1/fensterchef.1.gz" \
             "$PREFIX/usr/share/man/man5/fensterchef.5.gz" ; do
        [ -f "$f" ] && _command rm -f "$f"
    done
}

[ $# -eq 0 ] && usage 0

while [ $# -ne 0 ] ; do
    case "$1" in
    # Targets
    fensterchef) fensterchef ;;
    install) install ;;
    uninstall) uninstall ;;
    *)
        echo "what is \"$1\"?"
        usage 1
        ;;
    esac
    shift
done
