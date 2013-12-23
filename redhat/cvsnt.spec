Summary: Concurrent version control system
Name: cvsnt
Version: 2.0.51c
Release: 1
Copyright: GPL
Group: Development/Tools
Source: http://www.cvsnt.org/cvsnt-%{version}.tar.gz
URL: http://www.cvsnt.org
Prereq: /sbin/install-info
Prefix: %{_prefix}
Buildroot: %{_tmppath}/%{name}-root

%description
CVSNT (Concurrent Version System) is a version control system which can
record the history of your files (usually, but not always, source
code). CVSNT only stores the differences between versions, instead of
every version of every file you've ever created. CVSNT also keeps a log
of who, when and why changes occurred.

CVSNT is very helpful for managing releases and controlling the
concurrent editing of source files among multiple authors. Instead of
providing version control for a collection of files in a single
directory, CVSNT provides version control for a hierarchical collection
of directories consisting of revision controlled files.  These
directories and files can then be combined together to form a software
release.

Install the cvsnt package if you need to use a version control system.

CVSNT is much improved from the older CVS and now has many modern
version control features.  See http://www.cvsnt.org

%prep
%setup -q

%build
%configure --mandir=%{_mandir}

make
make doc info

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
gzip $RPM_BUILD_ROOT%{_mandir}/man1/*
gzip $RPM_BUILD_ROOT%{_mandir}/man5/*
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/etc/pam.d
mkdir -p $RPM_BUILD_ROOT/etc/xinetd.d
install redhat/cvslockd $RPM_BUILD_ROOT/etc/init.d/cvslockd
install redhat/cvsnt-pam $RPM_BUILD_ROOT/etc/pam.d/cvs
install redhat/cvsnt-xinetd $RPM_BUILD_ROOT/etc/xinetd.d/cvsnt

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc FAQ README
%doc doc/RCSFILES
%{_bindir}
%{_mandir}
%{_prefix}/lib/cvsnt
%{_prefix}/share/cvsnt/contrib
%{_sysconfdir}/cvsnt
%{_sysconfdir}/init.d/cvslockd
%{_sysconfdir}/pam.d/cvs
%{_sysconfdir}/xinetd.d/cvsnt

%changelog
* Thu Jul 29 2005 Tony Hoyle <tmh@nodomain.org>
- Updated for cvsnt 2.0.51

* Wed Feb  4 2004 Alon Ziv <alonz@emblazesemi.com>
- Updated for cvsnt 2.0.24

* Sat Feb 23 2002 Tony Hoyle <tmh@magenta-netlogic.com>
- Modified for cvsnt

* Fri Aug 17 2001 Corey Minyard <minyard@acm.org>
- Rewrote the spec file to make it sane.

* Thu Apr 26 2001 Derek Price <dprice@collab.net>
- avoid picking up %{_infodir}/dir.
- remove krb5-configs from requirements since RedHat doesn't use it anymore.

* Wed Nov 29 2000 Derek Price <dprice@openavenue.com>
- Use _infodir consistently for info pages and _bindir for binaries.
- use more succinct file list

* Wed Oct 18 2000 Derek Price <dprice@openavenue.com>
- Make the Kerberos binary a subpackage.
- fix the info & man pages too

* Wed Sep 27 2000 Derek Price <dprice@openavenue.com>
- updated for cvs 1.11

* Wed Mar  1 2000 Nalin Dahyabhai <nalin@redhat.com>
- make kerberos support conditional at build-time

* Wed Mar  1 2000 Bill Nottingham <notting@redhat.com>
- integrate kerberos support into main tree

* Mon Feb 14 2000 Nalin Dahyabhai <nalin@redhat.com>
- build with gssapi auth (--with-gssapi, --with-encryption)
- apply patch to update libs to krb5 1.1.1

* Fri Feb 04 2000 Cristian Gafton <gafton@redhat.com>
- fix the damn info pages too while we're at it.
- fix description
- man pages are compressed
- make sure %post and %preun work okay

* Sun Jan 9 2000  Jim Kingdon <http://bugzilla.redhat.com/bugzilla>
- update to 1.10.7.

* Wed Jul 14 1999 Jim Kingdon <http://developer.redhat.com>
- add the patch to make 1.10.6 usable
  (http://www.cyclic.com/cvs/dev-known.html).

* Tue Jun  1 1999 Jeff Johnson <jbj@redhat.com>
- update to 1.10.6.

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 2)

* Mon Feb 22 1999 Jeff Johnson <jbj@redhat.com>
- updated text in spec file.

* Mon Feb 22 1999 Jeff Johnson <jbj@redhat.com>
- update to 1.10.5.

* Tue Feb  2 1999 Jeff Johnson <jbj@redhat.com>
- update to 1.10.4.

* Tue Oct 20 1998 Jeff Johnson <jbj@redhat.com>
- update to 1.10.3.

* Mon Sep 28 1998 Jeff Johnson <jbj@redhat.com>
- update to 1.10.2.

* Wed Sep 23 1998 Jeff Johnson <jbj@redhat.com>
- remove trailing characters from rcs2log mktemp args

* Thu Sep 10 1998 Jeff Johnson <jbj@redhat.com>
- update to 1.10.1

* Mon Aug 31 1998 Jeff Johnson <jbj@redhat.com>
- fix race conditions in cvsbug/rcs2log

* Sun Aug 16 1998 Jeff Johnson <jbj@redhat.com>
- update to 1.10.

* Wed Aug 12 1998 Jeff Johnson <jbj@redhat.com>
- update to 1.9.30.

* Mon Jun 08 1998 Prospector System <bugs@redhat.com>
- translations modified for de, fr

* Mon Jun  8 1998 Jeff Johnson <jbj@redhat.com>
- build root
- update to 1.9.28

* Mon Apr 27 1998 Prospector System <bugs@redhat.com>
- translations modified for de, fr, tr

* Wed Oct 29 1997 Otto Hammersmith <otto@redhat.com>
- added install-info stuff
- added changelog section
