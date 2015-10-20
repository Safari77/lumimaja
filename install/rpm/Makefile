# Makefile to create an rpm file for wxWidgets version of lumimaja

# RELEASENAME should be provided by invoker, or grokked
# from src/ui/wxWidgets/version.in
RELEASENAME := 0.98

MKDIR = /bin/mkdir -p
CAT = /bin/cat
MV = /bin/mv
CP = /bin/cp
RM = /bin/rm -rf
SED = /bin/sed
ZIP = /usr/bin/zip
FIND = /usr/bin/find
CHMOD = /bin/chmod

DOCSDIR := ../../docs
XMLDIR  := ../../xml
HELPDIR := ../../help
MODIR   := ../../src/ui/wxWidgets/I18N/mos
# I'd prefer the rpm build to be done under this directory,
# but rpmbuild's default is ~/rpmbuild, and life's too short to find out
# where to change this.
RPMBUILDDIR := $(HOME)/rpmbuild
REDHATDIR := $(RPMBUILDDIR)/redhat

DOCS := ../../README.txt $(DOCSDIR)/ReleaseNotes.txt \
	$(DOCSDIR)/ChangeLog.txt
MANPAGE := $(DOCSDIR)/lumimaja.1
XMLFILES := $(wildcard $(XMLDIR)/*.*)
HELPFILES := $(HELPDIR)/helpEN.zip
PKG_BASE := lumimaja
SRC_TGZ := $(PKG_BASE)-$(RELEASENAME)-src.tgz
TARBALL := ../../Releases/$(SRC_TGZ)


.PHONY: all build clean tarball

all : build

build:	tarball
	@$(MKDIR) $(RPMBUILDDIR)/SPECS $(RPMBUILDDIR)/SOURCES
	@$(SED) s/@VERSION@/$(RELEASENAME)/g < lumimaja.spec.tmpl > $(RPMBUILDDIR)/SPECS/lumimaja.spec
	@$(CP) $(TARBALL) $(RPMBUILDDIR)/SOURCES
	@(cd $(RPMBUILDDIR); rpmbuild -ba -v SPECS/lumimaja.spec)
	@$(CP) $(RPMBUILDDIR)/RPMS/*/*.rpm ../../Releases
	@$(CP) $(RPMBUILDDIR)/SRPMS/*.rpm ../../Releases

tarball: $(TARBALL)

$(TARBALL):
	$(MAKE) -C ../.. tar PKG_BASE=$(PKG_BASE)

clean:
	@$(RM)  $(RPMBUILDDIR)
