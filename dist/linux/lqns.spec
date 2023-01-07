# LQNS RPM file.
# ------------------------------------------------------------------------
# $Id: lqns.spec 16291 2023-01-06 21:03:21Z greg $
# ------------------------------------------------------------------------

%define product_name lqns
%define product_version VERSION
%define rpm_release 4
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

%prep
cd ${RPM_BUILD_DIR}
rm -rf ${RPM_BUILD_DIR}/%{product_nameversion}
zcat ${RPM_SOURCE_DIR}/%{product_tarball} | tar xvf -
cd %{product_source_dir}
./configure --disable-shared --prefix=%{install_prefix} CFLAGS=-O3 CXXFLAGS=-O3 
%build
cd ${RPM_BUILD_DIR}/%{product_source_dir}
make


%install
rm -rf ${RPM_BUILD_ROOT}
cd ${RPM_BUILD_DIR}/%{product_source_dir}
make install DESTDIR="${RPM_BUILD_ROOT}"
mkdir -p ${RPM_BUILD_ROOT}%{install_prefix}/%{share_dir}
mkdir -p ${RPM_BUILD_ROOT}%{install_prefix}/%{share_dir}/examples
mkdir -p ${RPM_BUILD_ROOT}%{install_prefix}/share/man
mkdir -p ${RPM_BUILD_ROOT}%{install_prefix}/share/man/man%{product_man_section}

%post
cd %{install_prefix}/bin
ln -fs lqn2ps lqn2fig
ln -fs lqn2ps lqn2lqn
ln -fs lqn2ps lqn2jpeg
ln -fs lqn2ps lqn2png
ln -fs lqn2ps lqn2out
ln -fs lqn2ps lqn2xml
ln -fs lqngen lqn2lqx
cd %{install_prefix}/share/man/man%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2fig.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2lqn.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2jpeg.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2png.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2out.%{product_man_section}
ln -fs lqn2ps.%{product_man_section} lqn2xml.%{product_man_section}

%postun
cd %{install_prefix}/bin
rm -f lqn2fig
rm -f lqn2lqn
rm -f lqn2jpeg
rm -f lqn2png
rm -f lqn2out
rm -f lqn2xml
cd %{install_prefix}/share/man/man%{product_man_section}
rm -f lqn2fig.%{product_man_section} 
rm -f lqn2lqn.%{product_man_section} 
rm -f lqn2jpeg.%{product_man_section} 
rm -f lqn2png.%{product_man_section} 
rm -f lqn2pout.%{product_man_section} 
rm -f lqn2xml.%{product_man_section} 

%files
%dir %attr( - , root , root ) %{install_prefix}
%dir %attr( - , root , root ) %{install_prefix}/bin
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2csv
%attr( 0755 , root , root ) %{install_prefix}/bin/lqn2ps
%attr( 0755 , root , root ) %{install_prefix}/bin/lqngen
%attr( 0755 , root , root ) %{install_prefix}/bin/lqns
%attr( 0755 , root , root ) %{install_prefix}/bin/lqsim
%attr( 0755 , root , root ) %{install_prefix}/bin/lqx
%attr( 0755 , root , root ) %{install_prefix}/bin/petrisrvn
%attr( 0755 , root , root ) %{install_prefix}/bin/qnsolver
%attr( 0755 , root , root ) %{install_prefix}/bin/rep2flat
%attr( 0755 , root , root ) %{install_prefix}/bin/srvndiff
%dir %attr( - , root , root ) %{install_prefix}/lib
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqio.a
%attr( 0755 , root , root ) %{install_prefix}/lib/liblqx.a
%attr( 0755 , root , root ) %{install_prefix}/lib/libmva.a
%dir %attr( - , root , root ) %{install_prefix}/share
%dir %attr( - , root , root ) %{install_prefix}/share/man/man%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2csv.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqn2ps.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqngen.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqns.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/lqsim.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/petrisrvn.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/qnsolver.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/rep2flat.%{product_man_section}*
%attr( 0444 , root , root ) %{install_prefix}/share/man/man%{product_man_section}/srvndiff.%{product_man_section}*
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
