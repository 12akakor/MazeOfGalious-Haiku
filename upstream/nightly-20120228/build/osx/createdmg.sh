#!/bin/bash

if [ -z $1 ]; then
	echo USAGE: $0 [version number]
	exit 1
fi

echo Creating .dmg file...

echo Creating compressed dmg file...
hdiutil create -srcfolder Maze\ of\ Galious/build/Release/ -format UDZO -imagekey zlib-level=9 -volname Maze\ of\ Galious Maze\ of\ Galious-$1.dmg

echo
echo Done! 
echo The .dmg file is located in the current directory.

