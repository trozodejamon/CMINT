@echo off
rem Build settings for STC15F2K60S2
echo Building MINT
sdcc -pmcs51 --iram-size 256 --xram-size 32768 --code-size 61440 --model-large -c mint.c
sdcc -pmcs51 --iram-size 256 --xram-size 32768 --code-size 61440 --model-large main.c mint.rel
move main.ihx mint.ihx
packihx mint.ihx >mint.hex
echo Done