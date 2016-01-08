# coding: utf-8
import sys, getopt
import subprocess
import os

def main():
	args = sys.argv[1:]
	files = []
	output = ""
	try:
		opts, args = getopt.getopt(args,"o:f:",["output=", "files="])
	except getopt.GetoptError:
		print('$ dump_music_separated.py -o tune.o -f "tune.wav tune2.wav"')
		sys.exit(2)
	for opt, arg in opts:
		if opt in ('-o', '--output'):
			output = arg
		if opt in ('-f', '--files'):
			files = arg.split(' ')

	files_point_o = []
	for f in files:
		files_point_o.append(f+'.o')
		subprocess.check_output(['arm-none-eabi-ld', '-s', '-r', '-o', f+'.o', '-b', 'binary', f])

	final = ''
	for f in files_point_o:
		with open(f) as file_o:
			# on prend 126 comme caractère de séparation
			data = file_o.read()#.replace(chr(126), chr(125))
			final += data #+ chr(126)

	with open(output, 'w') as output_file:
		output_file.write(final)

if __name__ == "__main__":
	main()