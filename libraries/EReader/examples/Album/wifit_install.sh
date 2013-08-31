#!/bin/sh

wifit=wifit.0.0.2
sudo apt-get install python
sudo apt-get install python-imaging
sudo apt-get install python-numpy
sudo rm -f $wifit.zip
wget http://wyolum.com/downloads/$wifit.zip
unzip -o $wifit.zip

