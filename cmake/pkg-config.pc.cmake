prefix=${CMAKE_INSTALL_PREFIX}
includedir=${includedir}
libdir=${libdir}

Name: cargo
Description: ${PROJECT_DESCRIPTION}
Version: ${CARGO_VERSION}
Libs: -L${PKG_CONFIG_LIBDIR} -lcargo
Cflags: -I${PKG_CONFIG_INCLUDEDIR}
