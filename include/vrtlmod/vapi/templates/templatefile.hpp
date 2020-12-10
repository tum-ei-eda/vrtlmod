////////////////////////////////////////////////////////////////////////////////
/// @file templatefile.hpp
/// @date Created on Wed Dec 09 16:33:42 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__
#define __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__

#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>

#include "vrtlmod/util/logging.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all API building functionalities
namespace vapi {

////////////////////////////////////////////////////////////////////////////////
/// @class TemplateFile
/// @brief Self-contained base class for file generation without template files
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class TemplateFile {
	///////////////////////////////////////////////////////////////////////
	/// \brief Specified path to output directory
	std::string filepath_;
	std::string filename_;
protected:
	std::string header_;
	std::string body_;

	virtual const std::string get_brief(void){return (std::string(""));}
	virtual const std::string get_details(void){return (std::string(""));}
	virtual const std::string get_date(void){
		std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		return (std::ctime(&timestamp));
	}
	virtual const std::string get_author(void){return (std::string(""));}

	virtual void generate_header(void){
		std::stringstream x;
		header_ = "";
		x << "////////////////////////////////////////////////////////////////////////////////" << std::endl
			<< "/// @file " 		<< filename_ 			<< std::endl
			<< "/// @date " 		<< get_date() 		<< std::endl
			<< "/// @author " 	<< get_author() 	<< std::endl
			<< "/// @brief " 		<< get_brief() 		<< std::endl
			<< "/// @details " 	<< get_details() 	<< std::endl
			<< "////////////////////////////////////////////////////////////////////////////////" << std::endl;
	};
	virtual void generate_body(void) = 0;

public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns output directory
	const std::string get_filepath(void) const{
		return (filepath_);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns file name
	const std::string get_filename(void) const{
		return (filename_);
	}

	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	TemplateFile(void) { }
	virtual ~TemplateFile(void){};

	void write(const std::string filepath){
		filepath_ = filepath;
		filename_ = (filepath_.rfind("/") != std::string::npos) ?
			 filepath_.substr(filepath_.rfind("/"))
			: (filepath_.rfind("\\") != std::string::npos) ? filepath_.substr(filepath_.rfind("\\")) : filepath_;

		generate_header();
		generate_body();

		std::ofstream out;
		util::logging::log(util::logging::INFO, std::string("TemplateFile file path: ") + filepath_ + std::string(" open"));
		out.open(filepath_);
		if (out.fail()) {
			util::logging::abort(std::string("TemplateFile file path: ") + filepath_ + std::string(" invalid."));
		}
		out << header_;
		out << body_;
		out.close();
		util::logging::log(util::logging::INFO, std::string("TemplateFile file path: ") + filepath_ + std::string(" written"));
	}
};

} // namespace vapi

#endif /* __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__ */
