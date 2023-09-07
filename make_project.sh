#!/bin/bash
echo "-- Checking for Brew installation"
which -s brew
if [[ $? != 0 ]] ; then
  echo "-- Brew was not found. Installing via Ruby"
  ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
else
  echo "-- Found Brew"   
fi

if brew ls --versions SDL2 > /dev/null; then
  echo "-- Found SDL2 installation with Brew"
else
  echo "-- SDL2 was not found in the system. Installing via Brew"
  brew install SDL2
fi

echo "-- Running CMAKE"
mkdir project_files
cd project_files
cmake -G Xcode ../source/

