abs_top_srcdir=@abs_top_srcdir@
SRCDIR=..
VERSION = @PACKAGE_VERSION@
AM_CXXFLAGS = @CXXFLAGS@ -Wall 

LIBDIR=$(SRCDIR)/lib
LIBSTEVE=$(SRCDIR)/lib/libsteve.a

bin_PROGRAMS = smoldyn

OPENGLCFLAGS=@OPENGL_CFLAGS@
OPENGLLDFLAGS=@OPENGL_LDFLAGS@

LIBMOLECULIZERCFLAGS = @LIBMOLECULIZER_CFLAGS@
LIBMOLECULIZERLDFLAGS=@LIBMOLECULIZER_LDFLAGS@

PYTHONLDFLAGS=@PYTHON_LSPEC@

AM_CFLAGS= -Wall -I$(LIBDIR) $(OPENGLCFLAGS) $(LIBMOLECULIZERCFLAGS) 
AM_LDFLAGS=$(OPENGLLDFLAGS) $(LIBMOLECULIZERLDFLAGS) $(PYTHONLDFLAGS) $(LIBZ) -fexceptions

smoldyn_LDADD = $(LIBSTEVE) -lstdc++ 

if BUILD_LIBMOLECULIZER
smoldyn_LDADD += ../libmoleculizer-1.1.2/src/libmoleculizer/libmoleculizer-1.0.la \
		../libmoleculizer-1.1.2/src/libxml++-1.0.5/libxml++/libxmlpp.la
endif


smoldyn_SOURCES=\
libsmoldyn.c\
smolboxes.c\
smolcmd.c \
smolcomparts.c \
smoldyn.c \
smolgraphics.c \
smolmolec.c \
smolmoleculizer.c \
smolport.c \
smolreact.c \
smolsim.c \
smolsurface.c \
smolthread.c \
smolwall.c

EXTRA_DIST= \
libsmoldyn.h\
smoldyn.h
