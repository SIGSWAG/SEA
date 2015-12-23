import Leap, sys, thread, time
import SEA

PORT = 0
BAUDRATE = 9400

def main():

	serialController = SEA.SerialController();
	serialController.port = 0
	serialController.baudrate = 9400
	# Create a sample listener and controller
	listener = SEA.LeapMotionForMusicListener(serialController)
	controller = Leap.Controller()

	# Have the sample listener receive events from the controller
	controller.add_listener(listener)

	# Keep this process running until Enter is pressed
	print "Press Enter to quit..."
	try:
		sys.stdin.readline()
	except KeyboardInterrupt:
		pass
	finally:
		# Remove the sample listener when done
		controller.remove_listener(listener)


if __name__ == "__main__":
	main()
