#
# SPDX-FileCopyrightText: (C) 2017-2025 Red Hat, Inc.
#
# SPDX-License-Identifier: GPL-2.0-or-later
#

SUBDIRS = base internal log iface resource daemon modules tools

uninstall-hook:
	test -d ${DESTDIR}${pkglibdir} && rmdir ${DESTDIR}${pkglibdir}
	test -d ${DESTDIR}${pkgincludedir} && rmdir ${DESTDIR}${pkgincludedir}
