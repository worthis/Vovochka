magick icon.png -define icon:auto-resize=256,128,64,48,32,16 icon.ico
magick icon.png -background "#000000" -flatten -alpha off -strip -resize 256x256 -sampling-factor 2x2 -interlace none -quality 90 icon.jpg
pause