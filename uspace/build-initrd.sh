#!/bin/bash

../src/tools/make_initrd initrd.img hello_world.elf hello_world.elf
mv initrd.img ../