import os

data = os.environ["num1"]
data_splited = data.split("&")
try:
	num1 = float(data_splited[0])
	num2 = float(data_splited[1].split("=")[1])
	r = f"<output>{num1} + {num2} = {num1 + num2} :)</output>"
except ValueError:
	r = "Veuillez entrer un formulaire valide, c'est à dire avec deux nombres."

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