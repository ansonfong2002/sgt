# SGT/setup.csh: SGT setup (03/12/24, AF)
# Configure paths as necessary

setenv SGT ~/project/sgt
setenv PYTHON /opt/pkg/python-3.10
setenv PYTHONPATH $SGT/lib/python
setenv SYSTEMC_HOME /opt/pkg/systemc-2.3.3
setenv RISCV /opt/pkg/riscv
setenv PATH $SGT/bin:$PYTHON/bin:$RISCV/bin:$PATH

echo
echo "SGT setup:"
echo
echo "C++ compiler:   " `which g++`
echo "Python:         " `which python3.10`
echo "SystemC library:" $SYSTEMC_HOME
echo