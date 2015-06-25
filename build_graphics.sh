#!/bin/sh

#Scale all images 2x
cd graphs
cp background.png basepose.png stand.png impatient*.png ../resources/images/
cd ../resources/images
mogrify -scale 200% *.png
