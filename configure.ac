AC_PREREQ(2.61)
AC_INIT([zmqpbexample], [0.1], [joshrotenberg@gmail.com])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([ZMQ], [libzmq >= 2.1.7])
AC_SUBST(ZMQ_CFLAGS)
AC_SUBST(ZMQ_LIBS)

AC_PATH_PROG(PROTOC, protoc, no, $PATH:/usr/local/bin:/usr/bin)
if test "x$PROTOC" = "xno"; then
   AC_MSG_ERROR([protoc is required; please install or specify the path])
fi

PKG_CHECK_MODULES([PROTOBUF], [protobuf >= 2.3.0])
AC_SUBST(PROTOBUF_CFLAGS)
AC_SUBST(PROTOBUF_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_SRCDIR(src/zmqpbexample.cc)
AC_OUTPUT