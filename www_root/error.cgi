#!/usr/bin/env python3

import sys
import cgi, cgitb

form = cgi.FieldStorage()

arg1 = sys.argv[1]
arg2 = sys.argv[2]

#print("Content-Type:text/html\r\n\r\n")
print("<html>")
print("<head>")
print("<title>Error Page</title>")
print("</head>")
print("<body>")
print("<h2>%s %s</h2>" % (arg1, arg2))
print("</body></html>")

