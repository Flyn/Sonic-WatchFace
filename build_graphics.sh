#!/bin/sh

#Scale all images 2x
cd graphs
cp background.png clearnight.png cloudy.png cloudynight.png rainy.png rainynight.png rain1.png rain2.png rain3.png basepose.png sunny.png stand.png lookingup.png impatient*.png ../resources/images/
cd ../resources/images
mogrify -scale 200% *.png
