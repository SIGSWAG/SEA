import Leap, sys, thread, time, serial
from Leap import SwipeGesture

class SerialController(serial.Serial):

    def __init__(self, testing = False):
        super(SerialController, self).__init__()
        self.testing = testing

    def open(self):
        if self.testing:
            print "Testing : "
        else:
            super(SerialController, self).open()
        print "Serial communication oppened at port %s with baudrate at %d" % (self.portstr, self.baudrate)

    def close(self):
        if self.testing:
            print "Testing : "
        else:
            super(SerialController, self).open()
        print "Serial communication closed"

    def write(self, string):
        if self.testing:
            print "Testing : "
        else:
            super(SerialController, self).write(string)
        print "Message '%s' has been sent" % string

class LeapMotionForMusicListener(Leap.Listener):
    finger_names = ['Thumb', 'Index', 'Middle', 'Ring', 'Pinky']
    bone_names = ['Metacarpal', 'Proximal', 'Intermediate', 'Distal']
    state_names = ['STATE_INVALID', 'STATE_START', 'STATE_UPDATE', 'STATE_END']
    messages_code = {'left':'L', 'right':'R', 'up':'U', 'down':'D', 'forward':'F', 'backward':'B'}

    @property
    def app_width(self):
        return self._app_width
    
    @property
    def app_height(self):
        return self._app_height
    
    @property
    def app_depth(self):
        return self._app_depth
    
    @property
    def detection_percentage(self):
        return self._detection_percentage
    
    @property
    def detection_width(self):
        return self._detection_percentage * self.app_width

    @property
    def detection_height(self):
        return self._detection_percentage * self.app_height

    @property
    def detection_depth(self):
        return self._detection_percentage * self.app_depth    



    def __init__(self, serial):
        super(LeapMotionForMusicListener, self).__init__()
        self._serial = serial
        # measures in mm
        self._app_width = 600
        self._app_height = 400
        self._app_depth = 400
        self._detection_percentage = 0.5
        self._temporisation = 1 * 1000000 # microseconds
        self._last_messages = []
        self._last_frame_timestamp = 0

    def on_init(self, controller):
        if not self._serial.isOpen():
            self._serial.open()
        print "Initialized"

    def on_connect(self, controller):
        print "Connected"

        # Enable gestures
        controller.enable_gesture(Leap.Gesture.TYPE_SWIPE);

    def on_disconnect(self, controller):
        # Note: not dispatched when running in a debugger.
        print "Disconnected"

    def on_exit(self, controller):
        self._serial.close()
        print "Exited"

    def on_frame(self, controller):
        # Get the most recent frame and report some basic information
        frame = controller.frame()
        messages = []

        print "Frame id: %d, timestamp: %d, hands: %d, fingers: %d, tools: %d, gestures: %d" % (
              frame.id, frame.timestamp, len(frame.hands), len(frame.fingers), len(frame.tools), len(frame.gestures()))

        # Interaction box
        i_box = frame.interaction_box
        
        # exit if there is more than one hand
        if len(frame.hands) != 1:
            return

        # Get hands
        for hand in frame.hands:

            handType = "Left hand" if hand.is_left else "Right hand"

            print "  %s, id %d, position: %s" % (
                handType, hand.id, hand.palm_position)

            normalized_palm_position = i_box.normalize_point(hand.palm_position)
            # Normalized position is between [0,1]
            # Scale and center
            position_x = (self.app_width  * normalized_palm_position.x) - (self.app_width / 2)
            position_y = (self.app_height * (1 - normalized_palm_position.y)) - (self.app_height / 2)
            position_z = (self.app_depth * normalized_palm_position.z) - (self.app_depth / 2)

            # Get the hand's normal vector and direction
            normal = hand.palm_normal
            direction = hand.direction

            diff_x = _outsideX(position_x)
            diff_y = _outsideY(position_y)
            diff_z = _outsideZ(position_z)

            if diff_x < 0:
                messages.append(self.messages_code["left"])
            elif diff_x > 0:
                messages.append(self.messages_code["right"])
            if diff_y < 0:
                messages.append(self.messages_code["forward"])
            elif diff_y > 0:
                messages.append(self.messages_code["backward"])
            if diff_z < 0:
                messages.append(self.messages_code["up"])
            elif diff_z > 0:
                messages.append(self.messages_code["down"])
            

            # Calculate the hand's pitch, roll, and yaw angles
            print "  pitch: %f degrees, roll: %f degrees, yaw: %f degrees" % (
                direction.pitch * Leap.RAD_TO_DEG,
                normal.roll * Leap.RAD_TO_DEG,
                direction.yaw * Leap.RAD_TO_DEG)

        # Get gestures
        for gesture in frame.gestures():
            if gesture.type == Leap.Gesture.TYPE_SWIPE:
                swipe = SwipeGesture(gesture)
                print "  Swipe id: %d, state: %s, position: %s, direction: %s, speed: %f" % (
                        gesture.id, self.state_names[gesture.state],
                        swipe.position, swipe.direction, swipe.speed)

        if not (frame.hands.is_empty and frame.gestures().is_empty):
            print ""
        
        if len(messages):
            for message in messages:
                if message in self._last_messages: # if message was previously sent
                    if (frame.timestamp - self._last_frame_timestamp) > self._temporisation: # if delay is more than tempo
                        self._serial.write(message)
                else:
                    self._serial.write(message)
        self._last_frame_timestamp = frame.timestamp
        self._last_messages = messages
    
    def _outsideX(self, x):
        limit = self.detection_width / 2
        if x <= -limit :
            return x + limit
        elif x >= limit :
            return x - limit
        return 0
    
    def _outsideY(self, y):
        limit = self.detection_height / 2
        if y <= -limit :
            return y + limit
        elif y >= limit :
            return y - limit
        return 0

    def _outsideZ(self, z):
        limit = self.detection_depth / 2
        if z <= -limit :
            return z + limit
        elif z >= limit :
            return z - limit
        return 0

    def state_string(self, state):
        if state == Leap.Gesture.STATE_START:
            return "STATE_START"

        if state == Leap.Gesture.STATE_UPDATE:
            return "STATE_UPDATE"

        if state == Leap.Gesture.STATE_STOP:
            return "STATE_STOP"

        if state == Leap.Gesture.STATE_INVALID:
            return "STATE_INVALID"