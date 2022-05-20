# RemotePlayWhatever
Tiny application that lets you force remote play together any game you have in your steam library including non-steam ones.

## Building
Make sure you have cmake and wxWidgets installed first 

#### Linux
1. git clone --recurse-submodules https://github.com/m4dEngi/RemotePlayWhatever.git
2. cd RemotePlayWhatever
3. mkdir build
4. cd build
5. cmake ../
6. make

Optional ond deb/ubuntu system you can run ``make package`` to build *.deb package

#### Windows
1. git clone --recurse-submodules https://github.com/m4dEngi/RemotePlayWhatever.git
2. cd RemotePlayWhatever
3. mkdir build
4. cd build
5. cmake -G "Visual Studio 16 2019" -A Win32 ../
6. cmake --build . --config Release

## Setup
Credit to [Jondycz](https://www.reddit.com/r/Steam/comments/jtm6zc/remote_play_whatever_together_a_tiny_app_that/gc6jnvf?utm_source=share&utm_medium=web2x&context=3) on Reddit.
1. Go to [releases](https://github.com/m4dEngi/RemotePlayWhatever/releases) and download the latest pre-compiled version.
2. Extract zip file to desktop
3. Open the app
4. Open any steam game
5. Alt+Tab: on your system tray, there's a steam/portal logo, click on it and select your friend to play with.
6. Alt+tab back to the game and enjoy
