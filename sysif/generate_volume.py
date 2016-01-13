import sys

nb_volumes = 10
debut = 0
fin = 255
maximum = 255
minimum = 0
volume_min = 110
volume_incr = volume_min/nb_volumes

sys.stdout.write("char volumes[" + str(fin-debut+1) +"][" + str(nb_volumes) + "] = {\n")
for actu in range(debut, fin + 1):
	sys.stdout.write("{")
	for i in range(0, nb_volumes):
		a_jouer = actu + (int)(((255 - volume_incr*i - maximum)*(actu - minimum) - (minimum - volume_incr*i)*(maximum - actu)) / (maximum - minimum))
		sys.stdout.write(str(a_jouer))
		if i is not (nb_volumes-1):
			sys.stdout.write(", ")
	sys.stdout.write("}")
	if actu is not (fin):
		sys.stdout.write(",")
	sys.stdout.write("\n")

sys.stdout.write("}\n")	