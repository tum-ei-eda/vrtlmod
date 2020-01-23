////////////////////////////////////////////////////////////////////////////////
/// @file Misc.h
////////////////////////////////////////////////////////////////////////////////

#ifndef FTCV_MISC_H
#define FTCV_MISC_H

/** \brief Handles Logging of ftcv. And supports a abort() function.
  */

#include <string>

namespace ftcv
{

 enum LEVEL {
 	INFO,
	WARNING,
	ERROR

 };

const char * toString(LEVEL level);

void log(LEVEL level,const std::string & msg);

template <typename T>
std::string toLogString(const T & obj){
	return "{UNKNOWN OBJECT}";
}

template <>
inline std::string toLogString<std::string>(const std::string & obj){
	return std::string("{\"") + obj + "\"}";
}

template <typename T,typename ... OT>
void log(LEVEL level,const std::string & msg,const T & o,const OT & ...   objects)
{
	log(level,msg+"\n\t"+toLogString(o),objects...);
}

void abort();
void abort(const std::string & msg);
template <typename T,typename ... OT>
void abort(const std::string & msg,const T & o,const OT & ...   objects)
{
	abort(msg+"\n\t"+toLogString(o),objects...);
}

} // namespace ftcv

#endif // FTCV_LOGGER_H
