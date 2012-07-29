# Copyright 2012 Peter Bašista
#
# This file is part of the IPM Utility
#
# The IPM Utility is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# The name of the project
PNAME := ipm

APREFIX := $(PNAME)

ARCHIVE_NC := $(APREFIX).tar
ARCHIVE_GZ := $(ARCHIVE_NC).gz
ARCHIVE_XZ := $(ARCHIVE_NC).xz

# we need to define some basic variables
CPP := c++

# File extensions
HDREXT := .hpp
SRCEXT := .cpp
OBJEXT := .o
DEPEXT := .d

HDRDIR := h
SRCDIR := src
OBJDIR := obj
DEPDIR := d
ENAME := $(PNAME)
CFLAGS := -I$(HDRDIR)

# The version of the tar program present in the current system.
TAR_VERSION := $(shell tar --version | head -n 1 | cut -d ' ' -f 1)

# A flag indicating whether the xz compression utility is not available
XZ_UNAVAILABLE := $(shell hash xz 2>/dev/null || echo "COMMAND_UNAVAILABLE")

# Kernel name as returned by "uname -s"
KNAME := $(shell uname -s)

# If we are on the Mac OS, we would like to link with the iconv
ifeq ($(KNAME),Darwin)
LIBS := -liconv
else
LIBS :=
endif

AFLAGS := -O3 -Wall -Wextra -Wconversion -pedantic -g
HEADERS := $(wildcard $(HDRDIR)/*$(HDREXT))
SOURCES := $(wildcard $(SRCDIR)/*$(SRCEXT))
OBJECTS := $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:$(SRCEXT)=$(OBJEXT))))
DEPENDENCIES := $(addprefix $(DEPDIR)/,\
	$(notdir $(SOURCES:$(SRCEXT)=$(DEPEXT))))
OTHERFILES := COPYING Makefile README

.PHONY: clean dist distgz distxz

# First and the default target

all: $(DEPENDENCIES) $(OBJDIR) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) has been made"

$(DEPENDENCIES): $(DEPDIR)/%$(DEPEXT): $(SRCDIR)/%$(SRCEXT)
	@echo "DEP $@"
	@$(CPP) -MM -MT \
		'$@ $(addprefix $(OBJDIR)/,\
		$(subst $(SRCEXT),$(OBJEXT),$(notdir $<)))' \
		$(CFLAGS) $(AFLAGS) $< -o $@

include $(DEPENDENCIES)

$(OBJDIR):
	@echo "creating object directory for $(PNAME)"
	@mkdir $(OBJDIR)

$(OBJECTS):
	@echo "CPP $<"
	@$(CPP) -c $(CFLAGS) $(AFLAGS) $< -o $@

$(ENAME): $(OBJECTS)
	@echo "LD $(ENAME)"
	@$(CPP) $(LIBS) $(AFLAGS) $(OBJECTS) -o $(ENAME)

# If the available tar version is bsd tar, we have to change
# the syntax of the transformation command
ifeq ($(TAR_VERSION),bsdtar)
$(ARCHIVE_NC):
	@rm -rvf $(DEPDIR).tmp
	@mv -v $(DEPDIR) $(DEPDIR).tmp
	@mkdir -vp $(DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar -s '|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(HEADERS) $(SOURCES) $(DEPDIR) $(OTHERFILES)
	@rmdir $(DEPDIR)
	@mv -v $(DEPDIR).tmp $(DEPDIR)
else
$(ARCHIVE_NC):
	@rm -rvf $(DEPDIR).tmp
	@mv -v $(DEPDIR) $(DEPDIR).tmp
	@mkdir -vp $(DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar --transform 's|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(HEADERS) $(SOURCES) $(DEPDIR) $(OTHERFILES)
	@rmdir $(DEPDIR)
	@mv -v $(DEPDIR).tmp $(DEPDIR)
endif

$(ARCHIVE_GZ): $(ARCHIVE_NC)
	@echo "compressing the archive"
	@gzip -v '$(ARCHIVE_NC)'

$(ARCHIVE_XZ): $(ARCHIVE_NC)
	@echo "compressing the archive"
	@xz -v '$(ARCHIVE_NC)'

distgz: $(ARCHIVE_GZ)
	@echo "archive $(ARCHIVE_GZ) created"

distxz: $(ARCHIVE_XZ)
	@echo "archive $(ARCHIVE_XZ) created"

# If we do not have the xz compression utility,
# we would like to use the gzip instead
ifeq ($(XZ_UNAVAILABLE),COMMAND_UNAVAILABLE)
dist: distgz
else
dist: distxz
endif

clean:
	@rm -vf $(DEPENDENCIES) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) cleaned"

distclean:
	@rm -vf $(ARCHIVE_NC) $(ARCHIVE_GZ)
	@echo "distribution archives cleaned"
