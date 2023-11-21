#!/usr/bin/env python3

import os

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