////////////////////////////////////////////////////////////////////////////////
/// @file generator.cpp
/// @date Created on Mon Jan 07 14:12:11 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/vapi/target.hpp"
#include "vrtlmod/util/system.hpp"
#include "vrtlmod/util/logging.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

namespace vapi {

VapiGenerator::VapiGenerator(void)
	:	outdir_()
	, systemc_(false) {
}

int VapiGenerator::init(const char *pTargetXmlFile, const char *pOutdir, bool systemc) {
	systemc_ = systemc;
	outdir_ = pOutdir;
	return (XmlHelper::init(pTargetXmlFile));
}

std::string VapiGenerator::getTDExternalDecl(void) {
	static bool once = false;
	std::stringstream ret;
	if (!once) {
		once = true;
		ret << mTopTypeName << "VRTLmodAPI& gTD = " << mTopTypeName << "VRTLmodAPI::i(); \t// global target dictionary (api)\n";
	} else {
		ret << "extern " << mTopTypeName << "VRTLmodAPI& gTD;\n";
	}
	return (ret.str());
}

std::string VapiGenerator::getInludeStrings(void) {
	std::stringstream ret;
	ret <<
"/* Includes for Target Injection API */ \n\
#include \"" << get_targetdictionary_relpath() << "\" \n\
#include \"" << API_DIRPREFIX << "/" << get_apiheader_filename() << "\"\n";
	return (ret.str());
}

std::string VapiGenerator::get_intermittenInjectionStmtString(Target &t) {
	std::stringstream ret;
	ret << "gTD.td_.at(\"" << t.get_hierarchyDedotted() << "\")->inject()";
	return (ret.str());
}

std::string VapiGenerator::get_sequentInjectionStmtString(Target &t, int word //expression
		) {
	std::stringstream ret;
	if (word < 0) { // insert simple variable access
		ret << "gTD.td_.at(\"" << t.get_hierarchyDedotted() << "\")->inject()";
	} else { // insert word accessed write
		ret << "gTD.td_.at(\"" << t.get_hierarchyDedotted() << "\")->inject(" << word << ")";
	}
	t.mSeqInjCnt++;
	return (ret.str());
}

std::string VapiGenerator::get_sequentInjectionStmtString(Target &t, const std::string& word //expression
		) {
	std::stringstream ret;
	ret << "gTD.td_.at(\"" << t.get_hierarchyDedotted() << "\")->inject(" << word << ")";
	//	ret << "SEQ_TARGET_INJECT_W(gTD.td_[\"" << t.get_hierarchyDedotted() << "\"], " << word;
	//	ret << ")";
	t.mSeqInjCnt++;
	return (ret.str());
}

int VapiGenerator::isExprTarget(const char *pExpr) {
	int ret = -1;
	ExprT x = ExprT(pExpr);
	std::string expre;
	if (x.prefix == "vlSymsp") {
		expre = x.object + std::string(".") + x.name;
	} else if (x.prefix == "vlTOPp") {
		expre = std::string("TOP.") + x.name;
	}
	for (auto const &it : mTargets) {
		ret++;
		std::string target = it->mElData.hierarchy;
		if (target == expre) {
			return ret;
		}
	}
	return (-1);
}

Target& VapiGenerator::getExprTarget(int idx) {
	return (*mTargets.at(idx));
}

int VapiGenerator::build_API(void) {
	std::string api_dir = outdir_ + std::string("/") + std::string( API_DIRPREFIX) + std::string("/");

	if( !fs::exists(fs::path(api_dir)) ) {
		fs::create_directory( fs::path(api_dir) );
	}
	vapisrc_.write( api_dir + get_apisource_filename());
	vapiheader_.write( api_dir + get_apiheader_filename());

	return (1);
}

std::vector<std::string> VapiGenerator::prepare_sources(const std::vector<std::string>& sources, bool overwrite) {
	/* clean file list */
	std::vector<std::string> nsources;
	for (size_t i = 0; i < sources.size(); ++i) {
		if(fs::is_regular_file(fs::path(sources[i])) == false) {
			util::logging::log(util::logging::WARNING, std::string("Input source ")+ sources[i] + std::string(" is not a file. Removed from input set."));
		} else {
			nsources.push_back(sources[i]);
		}
	}
	auto outdir = fs::path(get_outputDir());
	if(!fs::is_directory(outdir)){
		auto ret = create_directory(outdir);
		if(ret == false){
			util::logging::log(util::logging::ERROR, std::string("Failed to create output directory ")+ get_outputDir());
		}
	}
	if (overwrite == false) {		
		for (size_t i = 0; i < nsources.size(); ++i) {
			std::stringstream tmp;
			std::string srcName;
			auto lSl = sources[i].rfind("/");
			auto dcpp = sources[i].rfind(".cpp");
			if (lSl != std::string::npos) {
			 srcName = nsources[i].substr(lSl + 1, dcpp - lSl - 1);
			} else {
			 srcName = nsources[i].substr(0, dcpp - 1);
			}
			tmp << get_outputDir() << "/" << srcName << "_vrtlmod.cpp";
			fs::copy_file(fs::path(nsources[i]), fs::path(tmp.str()), fs::copy_option::overwrite_if_exists);
			nsources[i] = tmp.str();
		}
	}
	return nsources;
}

} // namespace vapi
