HydraTool
======

This repository contains host software (Linux/Windows) for HydraBus with HydraFW firmware, a project to
produce a low cost, open source multi-tool hardware for anyone interested in Learning/Developping/Debugging/Hacking/Penetration Testing 
for basic or advanced embedded hardware.

HydraBus: https://www.hydrabus.com & https://github.com/hydrabus

HydraFW: https://github.com/hydrabus/hydrafw

## How to build host software on Windows:

### Prerequisites Windows, GNU/Linux:

* Qt5.6.x or more (with QtCreator)
 * See Qt5.x Online Installer http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe
 * See also http://download.qt.io/official_releases/qt/
 * Tested with success on Qt5.6 & Qt5.9.1 (using Qt Static Build) on Ubuntu 16.x LTS & Windows7 SP1(MSVC2013)

### Build the project with QtCreator on Windows, GNU/Linux:

* Import hydratool.pro project in Qt Creator 
* Configure the Project for MinGW and/or MSVC2013 or more
* Build the project

