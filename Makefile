# The name of the project
PNAME := ipm

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

.PHONY: clean

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

clean:
	@rm -vf $(DEPENDENCIES) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) cleaned"
