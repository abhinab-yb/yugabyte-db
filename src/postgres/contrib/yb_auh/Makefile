# contrib/yb_auh/Makefile

MODULE_big = yb_auh
OBJS = yb_auh.o $(WIN32RES)

EXTENSION = yb_auh
DATA =
PGFILEDESC = "yb_auh - Yugabyte Active Universe History"
SHLIB_LINK += -L$(YB_BUILD_ROOT)/lib -lyb_pggate

LDFLAGS_SL += $(filter -lm, $(LIBS))

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/yb_auh/yb_auh.conf
REGRESS = yb_auh

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/yb_auh
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
