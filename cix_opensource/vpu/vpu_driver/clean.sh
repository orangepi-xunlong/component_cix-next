scons -c
find . -name "*.o" | xargs rm -fv
find . -name "*.o.cmd" | xargs rm -fv
find . -name "*.ko" | xargs rm -fv
find . -name "*.ko.cmd" | xargs rm -fv
find . -name "*.mod" | xargs rm -fv
find . -name "*.mod.c" | xargs rm -fv
find . -name "*.mod.cmd" | xargs rm -fv
find . -name ".tmp_versions" | xargs rm -rfv
find . -name "*Module.symvers*" | xargs rm -rfv
find . -name "*modules.order*" | xargs rm -rfv
rm -rfv include lib bin build
rm -rfv site_scons/site_tools/__pycache__
rm -fv .scons.vars .sconsign.dblite site_scons/site_tools/misc.pyc

