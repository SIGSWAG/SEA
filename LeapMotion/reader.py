import serial
import io
import sys

ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM5'
ser.open()

print "Connected : "+str(ser)

while 1:
	msg = ser.read()
	if msg == '\0':
		print msg
	else:
		sys.stdout.write(msg)
