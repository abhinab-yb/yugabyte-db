# contrib/babelfishpg_tds/Makefile
MODULE_big = babelfishpg_tds
EXTENSION = babelfishpg_tds
DATA = babelfishpg_tds--1.0.0.sql
PGFILEDESC = "babelfishpg_tds - TDS Listener Extension"

tds_top_dir = .
tds_backend = $(tds_top_dir)/src/backend
tds_include = $(tds_top_dir)/src/include
TSQL_SRC = ../babelfishpg_tsql

PG_CFLAGS += -fstack-protector-strong
PG_CFLAGS += -Wno-tautological-constant-out-of-range-compare
PG_CFLAGS += -Wno-constant-conversion
PG_CFLAGS += -Wno-switch
PG_CFLAGS += -Wno-format
PG_CFLAGS += -Wno-shift-overflow
PG_CFLAGS += -Wno-builtin-memcpy-chk-size
PG_CFLAGS += -Wno-unknown-warning-option
PG_CFLAGS += -Wno-implicit-function-declaration
PG_CFLAGS += -Wno-int-conversion
PG_CPPFLAGS += -I$(TSQL_SRC) -I$(PG_SRC) -I$(tds_top_dir)

SHLIB_LINK += -lxml2

# Exclude the following files from the build (sometimes these
# files are included in another c file)
tds_exclude_files = $(tds_backend)/tds/support_funcs.c \
					$(tds_backend)/tds/tds_data_map.c \
					$(tds_backend)/tds/tdsprinttup.c

tds_temp_srcs = $(shell find $(tds_top_dir) -name "*.c")
tds_srcs = $(filter-out $(tds_exclude_files), $(tds_temp_srcs))

OBJS = $(patsubst %.c, %.o, $(tds_srcs))
OBJS += $(WIN32RES)

$(tds_include)/error_mapping.h: error_mapping.txt generate_error_mapping.pl
	$(PERL) generate_error_mapping.pl $< > $@
$(tds_backend)/tds/err_handler.o: $(tds_include)/error_mapping.h

# Disable for now
#NO_PGXS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/babelfishpg_tds
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# Only execute TAP tests if sqlcmd is installed in the path
# ifneq (, $(shell which sqlcmd))
# SUBDIRS = test
# endif

#include ../Makefile.common

.DEFAULT_GOAL := all

$(recurse)
$(recurs_always)
