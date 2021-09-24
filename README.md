# Linjat

A puzzle game, which started as a substrate for experimenting with procedural puzzle generation but ended up kind of fun to play.

Playable version at https://linjat.snellman.net/


# Setup 

## Requirements

- Cmake
- Gflags
- GCC
- Perl
- cpanm/JSON

## Setup instructions

Download CMAKE and follow the instructions on how to install for the command line
https://cmake.org/install/

How to install CMAKE - Essentially, download the program, run it, then go to `Tools->How to install for Command Line Use`. It'll tell you to run `sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install`

Install GCC
`brew install gcc --HEAD`

Install cpanm (to get JSON)
`brew install cpanm`

Install JSON dependency
`cpanm install JSON`

Install GFlags
`brew install gflags`

Now you should be able to run
`./gen.sh`

After you give it execute access


