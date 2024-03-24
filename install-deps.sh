#!/bin/sh

sudo dnf install -y openssl-devel

#downloading boost 1.84.0 from src

appRoot=$PWD

cd ~ & wget https://boostorg.jfrog.io/artifactory/main/release/1.84.0/source/boost_1_84_0.tar.gz
tar -xzf boost_*.tar.gz

cd boost_1_84_0
./bootstrap.sh && ./b2 install

sudo echo /usr/local/lib >> /etc/ld.so.conf && sudo ldconfig

cd $appRoot



