////////////////////////////////////////////////////////////////////////////////
/// @file ReWriteMacrosAction.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ftcv/rewriterHandler/RewriteMacrosAction.h"

namespace ftcv {

RewriteMacrosAction::RewriteMacrosAction() {

}

void RewriteMacrosAction::ExecuteAction() {
	clang::CompilerInstance &CI = getCompilerInstance();
	std::stringstream buffer;
	llvm::raw_os_ostream rout(buffer);
	RewriteMacrosInInput(CI.getPreprocessor(), &rout);
	rout.flush();
	std::ofstream out(getCurrentFile().str().c_str());
	out << buffer.str();
	out.flush();
	out.close();
}
bool RewriteMacrosAction::cleanFile(const std::string &file) {

	std::ifstream in(file.c_str());
	if (!in.is_open())
		return false;

	std::stringstream ss;

	while (in.good()) {
		char c;
		in.get(c);
		ss << c;
	}
	in.close();

	std::string data = ss.str();
	ss.str("");
	size_t pos = data.find("/*FTCVDL");
	while (pos != std::string::npos) {
		size_t end = data.find("FTCVDL*/", pos);
		if (end != std::string::npos) {
			data.replace(pos, end - pos + 8, "");
		} else {
			//ERROR
			return false;
		}
		pos = data.find("/*FTCVDL");
	}

	std::ofstream out(file.c_str());
	if (!out.is_open())
		return false;

	out << data;

	out.flush();
	out.close();

	return true;

}

} // namespace ftcv
