BINDIR=bin
INCLUDE=-I Sources
CFLAGS=-g0 -O3 -Wall -Wextra -Wno-write-strings -Wno-deprecated-declarations -std=c++0x
# Don't remove this - FIX the warning!
CFLAGS+=-Werror
LDFLAGS=
SOURCES=$(wildcard Sources/*.cpp)
OBJECTS=$(addprefix $(BINDIR)/, $(notdir $(SOURCES:.cpp=.o)))
EXECUTABLE=$(BINDIR)/XooTools
vpath %.cpp Sources

.PHONY: XooTools
all: $(EXECUTABLE)

$(BINDIR):
	mkdir -p $(BINDIR)

$(EXECUTABLE): $(BINDIR) $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

-include $(BINDIR)/$(OBJECTS:.o=.d)

$(BINDIR)/%.o: %.cpp
	$(CXX) $(INCLUDE) -c $(CFLAGS) $< -o $(BINDIR)/$*.o
	$(CXX) $(INCLUDE) -MM $(CFLAGS) $< > $(BINDIR)/$*.d
	@mv -f $(BINDIR)/$*.d $(BINDIR)/$*.d.tmp
	@sed -e 's|.*:|$(BINDIR)/$*.o:|' < $(BINDIR)/$*.d.tmp > $(BINDIR)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(BINDIR)/$*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $(BINDIR)/$*.d
	@rm -f $(BINDIR)/$*.d.tmp

clean:
	rm -fr $(BINDIR)/
