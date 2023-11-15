#!/usr/bin/env python3

def is_prime(n):
	if n < 2:
		return(False)
	i = 2
	while i * i <= n:
		if n % i == 0:
			return(False)
		i += 1
	return(True)

#import cgi cgitb

#cgitb.enable()

#data = cgi.FieldStorage()

#try:
#	num = int(data["value"].value)
#except:
#	print("You need to enter an integer")
#	raise SystemExit(1)

#if is_prime(num):
#	print("<output>{0} is prime :)</output>".format(num))
#else:
#	print("<output>{0} is not prime :)</output>".format(num))

print(is_prime(257))