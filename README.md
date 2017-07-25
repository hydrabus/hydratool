HydraTool
======

This repository contains host software (Linux/Windows) for HydraBus with HydraFW firmware, a project to
produce a low cost, open source multi-tool hardware for anyone interested in Learning/Developping/Debugging/Hacking/Penetration Testing 
for basic or advanced embedded hardware.

HydraBus: https://www.hydrabus.com & https://github.com/hydrabus

HydraFW: https://github.com/hydrabus/hydrafw

For more details on HydraTool see the [HydraTool Wiki](https://github.com/hydrabus/hydratool/wiki)

## How to build host software on Windows, GNU/LINUX:

### Prerequisites Windows, GNU/Linux:

* Qt5.6.x or more (with QtCreator)
 * See Qt5.x Online Installer http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe
 * See also http://download.qt.io/official_releases/qt/
 * Tested with success on Qt5.6/5.7/5.9.1 (using Qt Static Build) on Ubuntu 16.x LTS & Windows7 SP1(MSVC2013)

### Build the project with QtCreator on Windows, GNU/Linux:

* Import hydratool.pro project in Qt Creator 
* Configure the Project for MinGW and/or MSVC2013 or more
* Build the project

## How to build Qt5.x as static build on Windows, GNU/LINUX:
http://wohlsoft.ru/pgewiki/Building_static_Qt_5

### How to build Qt5.x as static build on Windows MSCV2013:
```
http://doc.qt.io/qt-5/windows-building.html

Download and extract jom from https://download.qt.io/official_releases/jom/
copy jom.exe to D:\Qt\Qt5_7-msvc2013_static_build\qt-everywhere-opensource-src-5.7.0

goto D:\Qt\Qt5_7-msvc2013_static_build\qt-everywhere-opensource-src-5.7.0\qtbase\mkspecs\common
and change msvc-desktop.conf like this (change all MD to MT to remove dependency on msvc dlls)
	initial values:

	QMAKE_CFLAGS_RELEASE = -O2 -MD
	QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += -O2 -MD -Zi
	QMAKE_CFLAGS_DEBUG = -Zi -MDd
	should be changed to:

	QMAKE_CFLAGS_RELEASE = -O2 -MT
	QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += -O2 -MT -Zi
	QMAKE_CFLAGS_DEBUG = -Zi -MTd

cd D:
d:

REM Set up \Microsoft Visual Studio 2013, where <arch> is \c amd64, \c x86, etc.
CALL "D:\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
SET _ROOT=D:\Qt\Qt5_7-msvc2013_static_build\qt-everywhere-opensource-src-5.7.0
SET PATH=%_ROOT%\qtbase\bin;%_ROOT%\gnuwin32\bin;%PATH%
REM Uncomment the below line when using a git checkout of the source repository
REM SET PATH=%_ROOT%\qtrepotools\bin;%PATH%
SET QMAKESPEC=win32-msvc2013
SET _ROOT=

cd D:\Qt\Qt5_7-msvc2013_static_build\qt-everywhere-opensource-src-5.7.0

configure.bat -static -debug-and-release -prefix "D:\Qt\Qt5.7.0_msvc2013_static" -platform win32-msvc2013 -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -opengl desktop -qt-sql-sqlite -qt-sql-odbc -no-openssl -opensource -confirm-license -make libs -nomake tools -nomake examples -nomake tests

jom

jom install
```

### How to build Qt5.9 as static build on GNU/LINUX:
```
cd ~
wget http://download.qt.io/official_releases/qt/5.9/5.9.0/single/qt-everywhere-opensource-src-5.9.0.tar.xz
tar --xz -xvf qt-everywhere-opensource-src-5.9.0.tar.xz
cd qt-everywhere-opensource-src-5.9.0
sudo ./configure -static -release -prefix ~/Qt590static -opensource -confirm-license -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -qt-xcb -opengl desktop -make libs -nomake tools -nomake examples -nomake tests -skip wayland -skip purchasing -skip serialbus -skip script -skip scxml -skip speech -skip qtwebengine
sudo make -r -j 4
sudo make install
```
