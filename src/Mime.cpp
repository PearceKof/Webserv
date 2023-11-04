
#include "Mime.hpp"

Mime::Mime()
{
	_ext[".html"] = "text/html";
	_ext[".htm"] = "text/html";
    _ext[".css"] = "text/css";
    _ext[".ico"] = "image/x-icon";
    _ext[".avi"] = "video/x-msvideo";
    _ext[".bmp"] = "image/bmp";
    _ext[".doc"] = "application/msword";
    _ext[".gif"] = "image/gif";
    _ext[".gz"] = "application/x-gzip";
    _ext[".ico"] = "image/x-icon";
    _ext[".jpg"] = "image/jpeg";
    _ext[".jpeg"] = "image/jpeg";
    _ext[".png"] = "image/png";
    _ext[".txt"] = "text/plain";
    _ext[".mp3"] = "audio/mp3";
	_ext[".mp4"] = "audio/mp4";
    _ext[".pdf"] = "application/pdf";
    _ext["default"] = "text/html";
}

bool	Mime::is_a_file(std::string request)
{
	for ( std::map<std::string, std::string>::iterator it = _ext.begin() ; it != _ext.end() ; it++ )
		if ( request.find(it->first) != std::string::npos )
			return true ;
	
	return false ;
}