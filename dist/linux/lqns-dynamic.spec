#
# LQNS RPM file.
# $Id: lqns-dynamic.spec 9267 2010-03-23 19:58:32Z greg $

%define product_name lqns
%define product_version 5.0
%define rpm_release 1
%define product_nameversion %{product_name}-%{product_version}
%define product_source_dir %{product_nameversion}
%define product_tarball %{product_nameversion}.tar.gz
%define product_man_section 1
%define product_url http://www.layeredqueues.org
%define product_vendor Carleton University
%define product_license Yes.
%define product_group Applications/Engineering
%define install_prefix /usr/local
%define share_dir share/%{product_name}

BuildRoot:  %{_tmppath}/%{product_name}-build-root
Release: %{rpm_release}
Summary: Layered Queueing Network Solvers
Name: %{product_name}
Version: %{product_version}
Group: %{product_group}
URL: %{product_url}
Vendor: %{product_vendor}
License: See http://www.sce.carleton.ca/rads/

Source0: %{product_tarball}
Patch0: lqns-patch.001.local-tweaks

%description
Layered queueing networks (LQNs) describe systems with software
servers and logical resources. A software server may itself act as a
customer.This format is easy to apply to modelling client-server
systems. The notion of software server has been generalized to include
locks, buffers, critical sections,task threads and other logical
resources, and the modeling concepts include asynchronous messaging,
and parallel execution.

%changelog
* Thu Oct 1 2009 Greg Franks <greg@sce.carleton.ca> 4.4
- Removed Copyright: The Department of Systems and Computer Engineeering, Carleton University.
* Thu May 13 2004 Greg Franks <greg@sce.carleton.ca> 3.2-1
- Initial version.


%prep
cd ${RPM_BUILD_DIR}
rm -rf ${RPM_BUILD_DIR}/%{product_nameversion}
zcat ${RPM_SOURCE_DIR}/%{product_tarball} | tar xvf -
cd %{product_source_dir}
./configure --enable-static=no --enable-shared --prefix=%{install_prefix} CFLAGS=-O3 CXXFLAGS=-O3 
%build
cd ${RPM_BUILD_DIR}/%{product_source_dir}
make


%install
rm -rf ${RPM_BUILD_ROOT}
cd ${RPM_BUILD_DIR}/%{product_source_dir}
make install DESTDIR="${RPM_BUILD_ROOT}"
mkdir -p ${RPM_BUILD_ROOT}%{install_prefix}/%{share_dir}
mkdir -p ${RPM_BUILD_ROOT}%{install_prefix}/%{share_dir}/examples

%post
cd %{install_prefix}/bin
ln -fs lqn2ps lqn2fig
ln -fs lqn2ps lqn2lqn
ln -fs lqn2ps lqn2jpeg
ln -fs lqn2ps lqn2png
ln -fs lqn2ps lqn2out
ln -fs lqn2ps lqn2xml
ln -fs lqsim parasrvn
cd %{install_prefix}/share/man/man%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2fig.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2lqn.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2jpeg.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2png.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2out.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2xml.%{product_man_section}
ln -fs lqsim.%{product_man_section} parasrvn.%{product_man_section}

%postun
cd %{install_prefix}/bin
rm -f lqn2fig
rm -f lqn2lqn
rm -f lqn2jpeg
rm -f lqn2png
rm -f lqn2out
rm -f lqn2xml
rm -f parasrvn
cd %{install_prefix}/share/man/man%{product_man_section}
rm -f lqn2ps.%{product_man_section} 
rm -f lqn2ps.%{product_man_section} 
rm -f lqn2ps.%{product_man_section} 
rm -f lqn2ps.%{product_man_section} 
rm -f lqn2ps.%{product_man_section} 
rm -f lqn2ps.%{product_man_section} 
rm -f lqsim.%{product_man_section}  

%files
%dir %attr( - , root , root ) %{install_prefix}
%dir %attr( - , root , root ) %{install_prefix}/bin
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2ps
%attr( 0755 , root , root ) %{install_prefix}/bin/lqngen
%attr( 0755 , root , root ) %{install_prefix}/bin/rep2flat
%attr( 0755 , root , root ) %{install_prefix}/bin/lqnx
%attr( 0755 , root , root ) %{install_prefix}/bin/lqsim
%attr( 0755 , root , root ) %{install_prefix}/bin/srvndiff
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2emf
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2fig
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2gif
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2jpeg
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2lqn
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2out
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2png
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2sxd
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2xml
%attr( 0755 , root , root ) %{install_prefix}/bin/lqx
%dir %attr( - , root , root ) %{install_prefix}/lib
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqio.la
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqio.so
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqio.so.0
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqio.so.0.0.1
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqx.la
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqx.la
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqx.so
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqx.so.0
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqx.so.0.0.1
%dir %attr( - , root , root ) %{install_prefix}/share
%dir %attr( - , root , root ) %{install_prefix}/share/man
%dir %attr( - , root , root ) %{install_prefix}/share/man/man%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2ps.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqngen.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqns.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqsim.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/srvndiff.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2emf.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2fig.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2gif.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2jpeg.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2lqn.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2out.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2png.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2sxd.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2xml.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/rep2flat.%{product_man_section}*
%dir %attr( 0755 , root , root ) %{install_prefix}/%{share_dir}
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/lqn-core.xsd
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/lqn-sub.xsd
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/lqn.xsd 
%dir %attr( 0755 , root , root ) %{install_prefix}/%{share_dir}/examples
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/cmpdesign.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/database.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/dbcase.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/dbcasea.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/dbcaseb.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/ex1-1.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/ex1-2.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/ex1-3.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/pipeline.lqn
%attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/examples/test.lqn
%doc %attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/tutorial.pdf
%doc %attr( 0444 , root , root ) %{install_prefix}/%{share_dir}/userman.pdf
