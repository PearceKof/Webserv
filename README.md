## Webserv

Webserv is a web server implemented in C++ as part of a project at 19 (42 Network). The objective is to create a fully functional HTTP/1.1 web server that is non-blocking, handles multiple ports, and complies with C++ 98 standards.

# Table of Contents

- Project Overview
- Features
- Requirements
- Installation
- Usage
- Configuration
- Dependencies

# Project Overview

The Webserv project aims to deepen our understanding of network programming, HTTP protocols, and server-client communication. The server is designed to handle HTTP requests, serve static files, and manage dynamic content, while implementing features like non-blocking I/O and proper error handling.

# Features

- Fully compliant with HTTP/1.1
- Non-blocking server operations using epoll()
- Configurable server settings via a configuration file
- Multi-port listening capabilities
- Support for GET, POST, and DELETE methods
- Static file serving and directory listing
- Client file uploads
- Accurate HTTP response status codes
- Default error pages if custom ones are not provided
- Stress-tested for stability

# Requirements

- Language: C++ 98 standard (must compile with -std=c++98)
- Memory Management: The server must handle memory efficiently and should not crash even if it runs out of memory.
- Makefile: The project must include a Makefile with at least the following rules: all, clean, fclean, re, and $(NAME)
- Compilation: Use c++ with the flags -Wall -Wextra -Werror
- Forbidden: External libraries and Boost libraries are not allowed.
- System Calls Allowed: execve, dup, dup2, pipe, fork, poll, select, socket, bind, listen, accept, and many others as specified in the project requirements.

# Installation

To build and run Webserv, you need a C++ compiler that supports the C++ 98 standard and make.

1. Clone the repository:
   git clone https://github.com/yourusername/Webserv.git
   cd Webserv

2. Build the project:
   make

3. The Makefile includes the following targets:
   - all: Compiles the project
   - clean: Removes object files
   - fclean: Removes object files and the executable
   - re: Rebuilds the project

# Usage

After building the server, you can start it by running:
./webserv [config_file]

- config_file is a parameter where you can specify a configuration file for the server.

# Configuration

Webserv can be customized using a configuration file similar to NGINX's format. Here is a basic example you can find in the config directory:
{
server {
	"listen" localhost:80 localhost:81

	"server_name" example.com

	"error_page" 400 www/400.html
	"error_page" 403 www/403.html
	"error_page" 404 www/404.html
	"error_page" 405 www/405.html
	"error_page" 409 www/409.html
	"error_page" 413 www/413.html
	"error_page" 415 www/415.html
	"error_page" 500 www/500.html
	"error_page" 501 www/501.html
	"error_page" 505 www/505.html

	"root" www
	"client_max_body_size" 1000000

	"location" / {
		"index" /home.html
		"allow_methods" GET
	}

	"location" /upload {
		"index" /upload.html
		"allow_methods" GET POST
		"upload_path" www/upload
		"upload" on
	}

	"location" /delete {
		"root" www/upload
		"index" /delete.html
		"allow_methods" GET DELETE
	}

	"location" /assets {
		"root" www/assets
		"autoindex" on
		"allow_methods" GET
	}

	"location" /cgi {
		"root" www
		"index" /cgi.html
		"allow_methods" GET POST
		"cgi_path" /cgi/isprime.py /cgi/addition.py
		"cgi_extension" py
	}

	"location" /redirection {
		"allow_methods" GET
		"redirect" https://www.youtube.com/watch?v=wFw-8AGBpYs
	}

	"location" /download {
		"index" /download.html
		"allow_methods" GET
	}
}
}

- listen: Specifies the port and IP address the server will listen on.
- server_name: Defines the domain name of the server.
- error_page: Configures the error page for specific HTTP errors.
- client_max_body_size: Limits the size of the client body.
- root: Defines the root of the server. The root is the starting point where the server will search the files to respond with.
- location: Sets the paths and the files defined with "index" that'll be sended on request and the accepted HTTP methods.
- upload and upload_path: If Upload is set to on and Post is an allowed method, then it is possible to upload files to the server to the path defined in upload_path

# Dependencies

Webserv relies solely on the C++ standard library (C++ 98) and system calls. It does not require any external libraries.
