# The name of the project
PNAME := ipm

# we need to define some basic variables
CPP := c++

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
HEADERS := $(wildcard $(HDRDIR)/*.hpp)
SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.cpp=.o)))
DEPENDENCIES := $(addprefix $(DEPDIR)/,$(notdir $(SOURCES:.cpp=.d)))

.PHONY: clean

# First and the default target

all: $(DEPENDENCIES) $(OBJDIR) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) has been made"

$(DEPENDENCIES): $(DEPDIR)/%.d: $(SRCDIR)/%.cpp
	@echo "DEP $@"
	@$(CPP) -MM -MT \
		'$@ $(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(notdir $<)))' \
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
