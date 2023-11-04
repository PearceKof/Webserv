
#ifndef MIME_HPP
# define MIME_HPP

# include <string>
# include <map>

class Mime
{
private:
	std::map<std::string ,std::string> _ext;
public:
	Mime();
	std::string	get_content_type(std::string ext) { return _ext[ext] ;};
	bool	is_a_file(std::string request);
};

#endif