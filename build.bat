cd gba
make clean
make
cd ..
mkdir data
mv -f gba/gba_mb.gba data/gba_mb.gba
make -f Makefile.gc clean
make -f Makefile.gc
make -f Makefile.wii clean
make -f Makefile.wii
pause
