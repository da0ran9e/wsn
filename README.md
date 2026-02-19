# wsn
Place this repo folder into ns-3-dev-git/src
Script: 
```bash
sudo apt install -y gcc g++ python3 python3-pip python3-setuptools git \
    mercurial qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    gdb valgrind gsl-bin libgsl-dev flex bison \
    libsqlite3-dev sqlite3 \
    libgtk-3-dev \
    libsctp-dev \
    libxml2 libxml2-dev \
    cmake

./ns3 configure --enable-examples --enable-modules=wsn
./ns3 build
```