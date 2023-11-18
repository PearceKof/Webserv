#!/usr/bin/env python3

import os

print("Content-type: text/html\n")

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
			result = f"<output>{num} est un nombre premier :)</output>"
		else:
			result = f"<output>{num} n'est pas un nombre premier :)</output>"
	except ValueError:
		result = "Veuillez entrer un nombre entier valide."
else:
	result = "Veuillez soumettre un formulaire avec une valeur."

print(f"""
<!DOCTYPE html>
<html>
<head>
	<title>Résultat</title>
</head>
<body>
	<h1>Résultat du script CGI</h1>
	<p>{result}</p>
</body>
</html>
""")