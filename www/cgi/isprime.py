#!/usr/bin/python3

# import cgi cgitb

# cgitb.enable()

# data = cgi.FieldStorage()

# def is_prime(n):
# 	if n < 2:
# 		return(False)
# 	i = 2
# 	while i * i <= n:
# 		if n % i == 0:
# 			return(False)
# 		i += 1
# 	return(True)

# try:
# 	num = int(data["value"].value)
# except:
# 	print("You need to enter an integer")
# 	raise SystemExit(1)

# if is_prime(num):
# 	content = f"<output>{num} is prime :)</output>"
# else:
# 	content = f"<output>{num} is not prime :)</output>"

# print("Content-Type: text/html\n")
# print(f"Content-Length: {len(content)}")
# print()
# print(content)

print "Content-type:text/html\r\n\r\n"
print '<html>'
print '<head>'
print '<title>Hello Word - First CGI Program</title>'
print '</head>'
print '<body>'
print '<h2>Hello Word! This is my first CGI program</h2>'
print '</body>'
print '</html>'