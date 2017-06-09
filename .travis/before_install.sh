#!/bin/sh
set -e

cd $HOME

if [[ $TRAVIS_PYTHON_VERSION == 'pypy' ]]; then
    deactivate

    wget https://bitbucket.org/squeaky/portable-pypy/downloads/pypy-5.8-linux_x86_64-portable.tar.bz2
    tar -jxvf pypy-5.8-linux_x86_64-portable.tar.bz2
    cd pypy-5.8-linux_x86_64-portable

    echo 'Setting up aliases...'
    export PATH=$HOME/pypy-5.8-linux_x86_64-portable/bin/:$PATH
    ln -s ~/pypy-5.8-linux_x86_64-portable/bin/pypy bin/python

    echo 'Setting up pip...'
    bin/pypy -m ensurepip
fi

if [[ $TRAVIS_PYTHON_VERSION == 'pypy3' ]]; then
    deactivate

    wget https://bitbucket.org/squeaky/portable-pypy/downloads/pypy3.5-5.8-beta-linux_x86_64-portable.tar.bz2
    tar -jxvf pypy3.5-5.8-beta-linux_x86_64-portable.tar.bz2
    cd pypy3.5-5.8-beta-linux_x86_64-portable

    echo 'Setting up aliases...'
    export PATH=$HOME/pypy3.5-5.8-beta-linux_x86_64-portable/bin/:$PATH
    ln -s ~/pypy3.5-5.8-beta-linux_x86_64-portable/bin/pypy3.5 bin/python

    echo 'Setting up pip...'
    bin/pypy -m ensurepip
    ln -s ~/pypy3.5-5.8-beta-linux_x86_64-portable/bin/pip3.5 bin/pip
fi

cd $TRAVIS_BUILD_DIR
