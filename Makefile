#  Makefile for Version 9.4 of Icon


#  configuration parameters
VERSION=9.4.0.b1
name=unspecified
dest=/must/specify/dest/


##################################################################
#
# Default targets.

default:	Icon Ilib Ibin

config/unix/$(name)/status src/h/define.h:
	:
	: To configure Icon, run either
	:
	:	make Configure name=xxxx     [for no graphics]
	: or	make X-Configure name=xxxx   [with X-Windows graphics]
	:
	: where xxxx is one of
	:
	@(cd config/unix; ls -d [a-z]*)
	:
	@exit 1


##################################################################
#
# Code configuration.


# Configure the code for a specific system.

Configure:	config/unix/$(name)/status
		make Pure >/dev/null
		cd config/unix; $(MAKE) Setup-NoGraphics name=$(name)

X-Configure:	config/unix/$(name)/status
		make Pure >/dev/null
		cd config/unix; $(MAKE) Setup-Graphics name=$(name)


# Get the status information for a specific system.

Status:
		@cat config/unix/$(name)/status


##################################################################
#
# Compilation.


# The interpreter: icont and iconx.

Icon Icon-icont bin/icont: Common
		cd src/icont;		$(MAKE)
		cd src/runtime;		$(MAKE) 


# The compiler: rtt, the run-time system, and iconc.
# (NO LONGER SUPPORTED OR MAINTAINED.)

Icon-iconc:	Common
		cd src/runtime;		$(MAKE) comp_all
		cd src/iconc;		$(MAKE)


# Common components.

Common:		src/h/define.h
		cd src/common;		$(MAKE)
		cd src/rtt;		$(MAKE)


# The Icon program library.

Ilib:		bin/icont
		cd ipl;			$(MAKE)

Ibin:		bin/icont
		cd ipl;			$(MAKE) Ibin


##################################################################
#
# Installation and packaging.


# Installation:  "make Install dest=new-parent-directory"

D=$(dest)
Install:
		test -d $D || mkdir $D
		test -d $D/bin || mkdir $D/bin
		test -d $D/lib || mkdir $D/lib
		test -d $D/doc || mkdir $D/doc
		test -d $D/man || mkdir $D/man
		test -d $D/man/man1 || mkdir $D/man/man1
		cp README $D
		cp bin/[a-qs-z]* $D/bin
		rm -f $D/bin/libXpm*
		cp lib/*.* $D/lib
		cp doc/*.* $D/doc
		cp man/man1/icont.1 $D/man/man1


# Bundle up for binary distribution.

DIR=icon.$(VERSION)
Package:
		rm -rf $(DIR)
		umask 002; $(MAKE) Install dest=$(DIR)
		tar cf - icon.$(VERSION) | gzip -9 >icon.$(VERSION).tgz
		rm -rf $(DIR)


##################################################################
#
# Tests.

Test    Test-icont:	; cd tests; $(MAKE) Test
Samples Samples-icont:	; cd tests; $(MAKE) Samples

Test-iconc:		; cd tests; $(MAKE) Test-iconc
Samples-iconc:		; cd tests; $(MAKE) Samples-iconc


#################################################################
#
# Run benchmarks.

Benchmark:
		$(MAKE) Benchmark-icont

Benchmark-iconc:
		cd tests/bench;		$(MAKE) benchmark-iconc

Benchmark-icont:
		cd tests/bench;		$(MAKE) benchmark-icont


##################################################################
#
# Cleanup.
#
# "make Clean" removes intermediate files, leaving executables and library.
# "make Pure"  also removes binaries, library, and configured files.

Clean:
		touch Makedefs
		rm -rf icon.*
		cd src;			$(MAKE) Clean
		cd ipl;			$(MAKE) Clean
		cd tests;		$(MAKE) Clean

Pure:
		touch Makedefs
		rm -rf icon.* bin/[a-z]* lib/[a-z]*
		cd ipl;			$(MAKE) Pure
		cd src;			$(MAKE) Pure
		cd tests;		$(MAKE) Pure
		cd config/unix; 	$(MAKE) Pure

Dist-Clean:
		rm -rf `find * -type d -name CVS`
		rm -f `find * -type f | xargs grep -l '<<ARIZONA-[O]NLY>>'`
