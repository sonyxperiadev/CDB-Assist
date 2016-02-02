/*
 * logowrapper.s
 *
 * Created: 2012-03-24 10:15:49
 *  Author: Werner Johansson, Unified Engineering of Sweden AB, wj@unifiedengineering.se
 */ 
 
	.section ".text"
	.global logobmp
	.global logobmpsize
logobmp:
	.incbin "bootlogo.bmp"
logobmpsize:
	.word .-logobmp
