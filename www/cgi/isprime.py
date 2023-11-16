#!/usr/bin/env python3

import cgi
print("Content-type: text/html\n")
data = cgi.FieldStorage()

# def is_prime(n):
# 	if n < 2:
# 		return(False)
# 	i = 2
# 	while i * i <= n:
# 		if n % i == 0:
# 			return(False)
# 		i += 1
# 	return(True)

if "value" in data:
	num = data["value"].value
	try:
		num = int(num)
		result = f"La valeur entrée par l'utilisateur est : {num}"
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



# if is_prime(num):
# 	content = f"<output>{num} is prime :)</output>"
# else:
# 	content = f"<output>{num} is not prime :)</output>"

# print("Content-Type: text/html\n")
# print(f"Content-Length: {len(content)}")
# print()
# print(content)





# print("Content-type:text/html\r\n\r\n")
# print('<html>')
# print('<head>')
# print('<title>Hello Word - First CGI Program</title>')
# print('</head>')
# print('<body>')
# print('<h2>Hello Word! This is my first CGI program</h2>')
# print('</body>')
# print('</html>')