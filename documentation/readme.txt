MinnaMicroZ80 v0.22 - The Z80 to Microdrive Snapshot Converter
--------------------------------------------------------------
Written by Robee Shepherd         http://www.robeesworld.com

This utility is licensed for non-commercial use only, you are
not permitted to distribute this software on any commercial
website, any commercially sold media.

This includes but is not limited to its inclusion on
CDs/DVDs/Blu-rayROM discs sold on ebay, or through a website.
It may also not appear on any website which contains advertising
or charges for access.

If you like this software, that's very cool! You can usually find me
on http://www.myreviewer.com, via the github repo or
http://www.worldofspectrum.org



About MinnaMicroZ80
-------------------

This is a Windows command line utility for converting Z80
snapshots so they can be restored on a real Spectrum from a
Microdrive cartridge.

It also includes a BASIC loader utility that you should install
to each cartridge, and that supports multiple games on a single
cartridge.


How to Use MinnaMicroZ80
------------------------

Load any game into an emulator such as Fuse or ZXSpin, save it
at any point you fancy as a z80 format snapshot. Failing that,
you can also often download z80 snapshots from various places
on the internet.

Note that restoring a game will corrupt part of the top third of
the screen, so the best place to save a game is right at the point
when it has loaded from tape, but before it wipes the loading
screen.

I might make a YouTube video on how to do this at some point.



Run the command line tool using the following arguments:

   MicroZ80.exe snapshot.z80 speccyname

This will generate a TZX tape file called "speccyname.tzx" and
containing one file for 48k snapshot or three for a 128k one.
Don't bother saving 48k games snapshotted on a 128k machine,
it will just create 3 files when you only needed 1.



Insert a microdrive cartridge into your Spectrum unit, and type:

   FORMAT "m";1;"Games"

Then copy the BASIC loader from GenericLoader.tzx, and the files
from the generated speccyname.tzx file to the microdrive
cartridge using whatever method you prefer.

Personally I use the excellent easy to use Copier program on
this site, written by Paul Farrow:
http://www.fruitcake.plus.com/Sinclair/Interface2/Cartridges/Interface2_RC_New_Copier.htm



Why the Name?
-------------

Minna was my last dog, she was a cute Finnish Lapphund who
reached the ripe old age of 17.5.

She was very cute and fluffy.


Special Thanks
--------------

This program was made possible by the MegaLZ C packing code, and
Z80 depacking code by mayHem:
http://lvd.nm.ru/MegaLZ/ (link no longer works :()

And the Microdrive machine code loading routines of Jim P, plus
he also showed me how to clear a BASIC program from MC too!:
http://www.worldofspectrum.org/forums/showthread.php?t=26055
