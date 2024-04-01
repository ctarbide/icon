#!/bin/sh
set -eu
die(){ ev=$1; shift; for msg in "$@"; do echo "${msg}"; done; exit "${ev}"; }

# settings
prefix=${HOME}/local
distdir=icon-package

# sanity check
test -d "${distdir}" || die 1 "Error, directory \"${distdir}\" not found."

# create dirs
install -m 755 -d "${prefix}/bin"
install -m 755 -d "${prefix}/lib/icon"
install -m 755 -d "${prefix}/share/icon/doc"
install -m 755 -d "${prefix}/share/man/man1"

# share/icon
install -m 644 "${distdir}/README" "${prefix}/share/icon"
install -m 644 "${distdir}/doc"/* "${prefix}/share/icon/doc"

# share/man
install -m 644 "${distdir}/man/man1/icon.1" "${prefix}/share/man/man1"
install -m 644 "${distdir}/man/man1/icont.1" "${prefix}/share/man/man1"

# lib/icon
install -m 644 "${distdir}/lib/icon"/*.h "${prefix}/lib/icon"
install -m 644 "${distdir}/lib/icon"/*.icn "${prefix}/lib/icon"
install -m 644 "${distdir}/lib/icon"/*.u1 "${prefix}/lib/icon"
install -m 644 "${distdir}/lib/icon"/*.u2 "${prefix}/lib/icon"

# bin
install -m 755 "${distdir}/bin/icon" "${prefix}/bin/icon"
install -m 755 "${distdir}/bin/icont" "${prefix}/bin/icont"
install -m 755 "${distdir}/bin/iconx" "${prefix}/bin/iconx"
install -m 755 "${distdir}/bin/libcfunc.so" "${prefix}/bin/libcfunc.so"

echo 'all done'
