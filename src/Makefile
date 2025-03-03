CXX := g++
CFLAGS := -O3 -g -std=gnu++20 -Werror -Wall -Wextra -Wpedantic -Wcast-align=strict -Wformat-security
#CFLAGS_DEBUG := -O3 -g --coverage -std=gnu++20 -Werror -Wall -Wextra -Wpedantic -Wcast-align=strict -Wformat-security
LDFLAGS := -lncurses -lze_loader
SOURCES := ze-monitor.cpp helpers.cpp args.cpp engine.cpp process.cpp temperature.cpp device.cpp power_domain.cpp
OBJECTS := $(SOURCES:.cpp=.o)
TARGET := ze-monitor
DESTDIR := /usr/bin

# Version handling
VERSION_FILE := version.txt
VERSION := $(shell cat $(VERSION_FILE))
BUILD_DIR := build/$(VERSION)

# Extract components from version
VERSION_NUMS := $(shell echo $(VERSION) | cut -d'-' -f1)
RELEASE := $(shell echo $(VERSION) | cut -d'-' -f2)

# Targets
all: $(BUILD_DIR)/$(TARGET)

check-pkg:
	@if [ "$(PKG_CHECK)" != "yes" ]; then \
		echo "libze-dev or ncurses-dev not found. Please install both."; \
		exit 1; \
	fi

$(BUILD_DIR)/%.o: %.cpp %.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TARGET): $(addprefix $(BUILD_DIR)/,$(OBJECTS))
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: $(BUILD_DIR)/$(TARGET)
	mkdir -p $(DESTDIR)
	sudo cp $(BUILD_DIR)/$(TARGET) $(DESTDIR)
	sudo chmod +x $(DESTDIR)/$(TARGET)

deb:
	sed -e "s/VERSION/$(VERSION_NUMS)/g" \
	    -e "s/RELEASE/$(RELEASE)/g" \
	    debian/control.template > debian/control
	sed -e "s/VERSION/$(VERSION_NUMS)/g" \
	    -e "s/RELEASE/$(RELEASE)/g" \
	    debian/changelog.template > debian/changelog
	dpkg-buildpackage -us -uc -b
	mkdir -p $(BUILD_DIR)/packages
	mv ../$(TARGET)_$(VERSION_NUMS)-$(RELEASE)*.deb $(BUILD_DIR)/packages/

rpm:
	mkdir -p $(CURDIR)/rpmbuild/SOURCES
	mkdir -p $(CURDIR)/rpmbuild/SPECS
	mkdir -p $(CURDIR)/rpmbuild/BUILD
	mkdir -p $(CURDIR)/rpmbuild/RPMS
	mkdir -p $(CURDIR)/rpmbuild/SRPMS
	tar czf $(CURDIR)/rpmbuild/SOURCES/$(TARGET)-$(VERSION_NUMS).tar.gz \
		--transform 's,^,$(TARGET)-$(VERSION_NUMS)/,' \
		--exclude=rpmbuild \
		*
	sed -e "s/VERSION/$(VERSION_NUMS)/g" \
	    -e "s/RELEASE/$(RELEASE)/g" \
	    rpm/$(TARGET).spec.template > $(CURDIR)/rpmbuild/SPECS/$(TARGET).spec
	rpmbuild -ba --nodeps $(CURDIR)/rpmbuild/SPECS/$(TARGET).spec \
		--define "_topdir $(CURDIR)/rpmbuild"		
	mkdir -p $(BUILD_DIR)/packages
	find $(CURDIR)/rpmbuild/RPMS -name "$(TARGET)-$(VERSION_NUMS)-$(RELEASE)*.rpm" -exec cp {} $(BUILD_DIR)/packages/ \;

clean:
	rm -rf build/

.PHONY: all check-pkg install clean deb rpm
