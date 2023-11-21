#!/usr/bin/env python3

import os

method = os.environ["REQUEST_METHOD"]
if method == "GET":
	value = os.environ["value"]
	def is_prime(n):
		if n < 2:
			return(False)
		i = 2
		while i * i <= n:
			if n % i == 0:
				return(False)
			i += 1
		return(True)

	if value:
		try:
			num = int(value)
			if is_prime(num):
				r = f"<output>{num} est un nombre premier :)</output>"
			else:
				r = f"<output>{num} n'est pas un nombre premier :)</output>"
		except ValueError:
			r = "Veuillez entrer un nombre entier valide."
	else:
		r = "Veuillez soumettre un formulaire avec une valeur."
elif method == "POST":
	data = os.environ["num1"]
	data_splited = data.split("&")
	try:
		num1 = float(data_splited[0])
		num2 = float(data_splited[1].split("=")[1])
		r = f"<output>{num1} + {num2} = {num1 + num2} :)</output>"
	except ValueError:
		r = "Veuillez entrer un formulaire valide, c'est à dire avec deux nombres."
	
	#r = os.environ


result = f"""
<!DOCTYPE html>
<html>
<head>
	<title>Résultat</title>
	<meta charset="UTF-8">
</head>
<body>
	<h1>Résultat du script CGI</h1>
	<p>{r}</p>
</body>
</html>
"""
print(result)