#!/bin/bash

make -j32
if [ $? -eq 0 ]; then
	sudo make modules_install && sudo make install
else
	echo "Compilation failed"
fi
