AUTOSCAN program.

Preconditions:
1. Captain Obviousness wants to warn you that in order to use this program you should have scanner to work with this program :)
2. This program was tested with Windows 10


The overall architecture: the scanning program uses WIA - Window Image Acquisition library using COM objects alongside with GDI+.

This sample automatically scans documents with Flatbed type of scanners (but not tested with feeder type of scanners when there are separated individual pages in the scanner tray, for this type of program you need to set timeout value to zero, there is no need to turn the page manually)

But you can also scan lists non-automatical mode, just when you are ready to scan.
This program can
1) automaticall increment and setup new page counter.
Usefull when you start a new book or want to rescan a certain page
2) you may set up a timeout between scans, the time required to take the book and turn the page and put it back to the scanner
The most skilled guy requires 6 seconds.
3) the program can save files in various file formats jpg , png
4) after saving the the file program
4.1) will turn the page to 90 degrees clockwise
5) each the page will be cropped so there is only the page content, the cropping point can be customized
6) for each scanned  page , there are three  files: original one, rotated one, and rotated+cropped. , those manipulation are implemented using WinApi GDI+ library

7) You can also control the whole process: stop it, resume it, switch between auto mode, change timeout value using console application via command line. 
Also Ctrl+Break command will gracefully stop the scanning process , but not immediately, it will wait while the current page will be scanned so no data will be lost.

Have fun.
so you don't have to manually rename the file, rotate and crop files , just turn  the page each N seconds.



CopyLeft. IT-Cluster "Reactor" Luhanshchyna. 2021
Nusrat Nuriyev
________________________
Another useful links
https://stackoverflow.com/questions/5345803/does-gdi-have-standard-image-encoder-clsids
https://docs.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-cropping-and-scaling-images-about


The time spent:
6 hours proof of concept
+9 hour to make it suitable for personal needs

