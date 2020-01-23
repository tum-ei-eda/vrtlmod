////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod.cpp
/// @brief main file for llvm-based VRTL-modifer tool
/// @details based on ftcv frontend
/// @date Created on Mon Jan 15 12:29:21 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "vrtlmod.hpp"

////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option category
static llvm::cl::OptionCategory UserCat("User Options");
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "regxml". Gets input Xml
static llvm::cl::opt<std::string> RegisterXmlFilename("regxml", llvm::cl::Required, llvm::cl::desc("Specify input register xml"),
		llvm::cl::value_desc("file name"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "out". Gets output directory path
static llvm::cl::opt<std::string> OUTdir("out", llvm::cl::Optional, llvm::cl::desc("Specify output directory"), llvm::cl::value_desc("path"),
		llvm::cl::cat(UserCat));

////////////////////////////////////////////////////////////////////////////////
/// \brief vrtlmod main()
int main(int argc, const char **argv) {
	// Consume arguments
	APIbuilder &tAPI = APIbuilder::_i();

	CommonOptionsParser op(argc, argv, UserCat);

	if (tAPI.init(RegisterXmlFilename.c_str(), OUTdir.c_str()) < 0)
		return (-1);

	std::vector<std::string> sources = op.getSourcePathList();

	// prepare *_vrtlmod.cpp files: create, de-macro, clean comments.
	for (size_t i = 0; i < sources.size(); i++) {
		std::stringstream tmp;
		std::string srcName;
		auto lSl = sources[i].rfind("/");
		auto dcpp = sources[i].rfind(".cpp");
		if (lSl != std::string::npos) {
			srcName = sources[i].substr(lSl + 1, dcpp - lSl - 1);
		} else {
			srcName = sources[i].substr(0, dcpp - 1);
		}
		tmp << tAPI.get_outputDir();
		tmp << "/";
		tmp << srcName << "_vrtlmod.cpp";
		system((std::string("cp \"") + sources[i] + "\" \"" + tmp.str() + "\"").c_str());
		sources[i] = tmp.str();
	}

	// create a new Clang Tool instance
	ClangTool ToolM(op.getCompilations(), sources);

	// run Clang // insert macros (needed for macro code rewrite)
	int err = ToolM.run(newFrontendActionFactory<ftcv::RewriteMacrosAction>().get()
			);

	for (size_t i = 0; i < sources.size(); i++) {
		ftcv::RewriteMacrosAction::cleanFile(sources[i]);
	}

	ClangTool ToolRw(op.getCompilations(), sources);

	err = ToolRw.run(newFrontendActionFactory<MyFrontendAction>().get());

	return (err);
}

