Micro-controller code for an USB oscilloscope

I have built a 3 channel USB oscilloscope as a personal hobby project.  I designed the schematics and layout and populated all the electrical components myself.  I chose STM32F4 micro-controller from ST Micro for this project.

It uses an FTDI USB chip to communicate to the GUI program running on a PC.  It uses a DMA function for the A2D pins for measuring voltage signals.  Ideally the "main.c" should be split in several files, but as a hobby project, it was good enough for me.  

