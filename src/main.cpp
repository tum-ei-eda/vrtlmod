////////////////////////////////////////////////////////////////////////////////
/// @file main.cpp
/// @brief main file for llvm-based VRTL-modifer tool
/// @details based on ftcv frontend by ?
/// @date Created on Mon Jan 15 12:29:21 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

#include "vrtlmod/vrtlmod.hpp"
#include "vrtlmod/util/system.hpp"

////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option category
static llvm::cl::OptionCategory UserCat("User Options");
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "systemc".
static llvm::cl::opt<bool> SystemC("systemc", llvm::cl::Optional, llvm::cl::desc("Input VRTL is SystemC code"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "regxml". Sets input Xml
static llvm::cl::opt<std::string> RegisterXmlFilename("regxml", llvm::cl::Required, llvm::cl::desc("Specify input register xml"),
		llvm::cl::value_desc("file name"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "out". Sets output directory path
static llvm::cl::opt<std::string> OUTdir("out", llvm::cl::Required, llvm::cl::desc("Specify output directory"), llvm::cl::value_desc("path"),
		llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "override".
static llvm::cl::opt<bool> Overwrite("overwrite", llvm::cl::Optional, llvm::cl::desc("Override source files"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "silent".
static llvm::cl::opt<bool> Silent("silent", llvm::cl::Optional, llvm::cl::desc("Execute without Warnings and Info"), llvm::cl::cat(UserCat));

static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp(vrtlmod::env::get_environmenthelp().c_str());

////////////////////////////////////////////////////////////////////////////////
/// \brief vrtlmod main()
int main(int argc, const char **argv) {
	// Consume arguments

	CommonOptionsParser op(argc, argv, UserCat);

	if(bool(Silent)){
		util::logging::log(util::logging::OBLIGAT, "Executing silently - Warnings and Infos disabled", true);
	}

	if(!fs::exists(OUTdir.c_str())){
		util::logging::abort(std::string("Output directory ") + std::string(OUTdir) + " doesn't exist!");
	}

	if(!fs::exists(RegisterXmlFilename.c_str())){
		util::logging::abort(std::string("XML file ") + std::string(RegisterXmlFilename) + " doesn't exist!");
	}

	vapi::VapiGenerator& tAPI = vapi::VapiGenerator::_i();
	if (tAPI.init(RegisterXmlFilename.c_str(), OUTdir.c_str(), SystemC) < 0){
		util::logging::abort(std::string("Vrtlmod API generator initialization failed"));
	}

	std::vector<std::string> sources = op.getSourcePathList();

	// prepare *_vrtlmod.cpp files: create, de-macro, clean comments.
	vrtlmod::prepare_sources(sources);

	// create a new Clang Tool instance
	ClangTool ToolM(op.getCompilations(), sources);

	// run Clang // insert macros (needed for macro code rewrite)
	int err = ToolM.run(newFrontendActionFactory<transform::rewrite::RewriteMacrosAction>().get());

	for (size_t i = 0; i < sources.size(); i++) {
		transform::rewrite::RewriteMacrosAction::cleanFile(sources[i]);
	}

	ClangTool ToolRw(op.getCompilations(), sources);

	err = ToolRw.run(newFrontendActionFactory<MyFrontendAction>().get());

	tAPI.build_API();

	return (0);
}

//-------------------------
void vrtlmod::prepare_sources(std::vector<std::string> &sources) {
	if (Overwrite == false) {
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
			tmp << vapi::VapiGenerator::_i().get_outputDir();
			tmp << "/";
			tmp << srcName << "_vrtlmod.cpp";
			fs::copy_file(fs::path(sources[i]), fs::path(tmp.str()), fs::copy_option::overwrite_if_exists);
			// system((std::string("cp \"") + sources[i] + "\" \"" + tmp.str() + "\"").c_str());
			sources[i] = tmp.str();
		}
	}
}



bool vrtlmod::env::check_environment(void) {
	// Replace with checks to verilator?
/*	std::string console;
	console = util::system::exec("[ -z \"$VRTLMOD_SRCDIR\" ] && echo \"Empty\"");
	if (console == "Empty") {
		console = util::system::exec("ls template");
		if (console.find("No such file or directory") != std::string::npos) {
			util::logging::log(util::logging::OBLIGAT, get_environmenthelp());
			return (false);
		}
	}
*/
	return (true);
}

std::string vrtlmod::env::get_environmenthelp(void) {
	return("Either execute vrtlmod from its source directory or set \"VRTLMOD_SRCDIR\" to vrtlmod's source directory.\nE.g.: \"export VRTLMOD_SRCDIR=<path-to-vrtlmod-srcs>\"");
}
