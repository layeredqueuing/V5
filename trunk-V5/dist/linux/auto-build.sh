#!/bin/sh
export SHELL=/bin/sh
cd $HOME/Build/src-linux
if svn update | grep '^[UP] ' > /dev/null; then
  if make > make.out; then
    make install
  else
    mail -s"Nightly Build failed." greg < make.out
  fi
fi
