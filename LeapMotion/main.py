import Leap, sys, thread, time
import SEA

PORT = 0
BAUDRATE = 9400
SWIPE_MINLENGTH = 100.0		# mm
SWIPE_MINVELOCITy = 750		# mm/s

def main():
	serialController = SEA.SerialController(testing=True);
	serialController.port = PORT
	serialController.baudrate = BAUDRATE
	# Create a sample listener and controller
	listener = SEA.LeapMotionForMusicListener(serialController)
	controller = Leap.Controller()

	# Configure SwipeGesture detection
	controller.config.set("Gesture.Swipe.MinLength", SWIPE_MINLENGTH)
	controller.config.set("Gesture.Swipe.MinVelocity", SWIPE_MINVELOCITy)
	controller.config.save()

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
