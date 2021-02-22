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
		ret << mTopTypeName << "_TD& gTD = " << mTopTypeName << "VRTLmodAPI::i().get_struct(); \t// global target dictionary\n";
	} else {
		ret << "extern " << mTopTypeName << "_TD& gTD;\n";
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
	if (t.mElData.vrtlCxxType.find("[") == std::string::npos) { //(t.mElData.vrtlCxxType.find("WData") == std::string::npos) {
		ret << "INT_TARGET_INJECT(gTD." << get_targetdictionaryTargetClassDeclName(t);
	} else { // insert word accessed write
		int words = -1;
		for (int i = t.mElData.nmbBits; i >= 0; i -= 32) {
			words++;
		}
		ret << "INT_TARGET_INJECT_W(gTD." << get_targetdictionaryTargetClassDeclName(t) << ", " << words; //word;
	}
	ret << ")";

	return (ret.str());
}

std::string VapiGenerator::get_sequentInjectionStmtString(Target &t, int word //expression
		) {
	std::stringstream ret;
	if (word < 0) { // insert simple variable access
		ret << "SEQ_TARGET_INJECT(gTD." << get_targetdictionaryTargetClassDeclName(t);

	} else { // insert word accessed write
		ret << "SEQ_TARGET_INJECT_W(gTD." << get_targetdictionaryTargetClassDeclName(t) << ", " << word;
	}
	ret << ")";
	t.mSeqInjCnt++;
	return (ret.str());
}

std::string VapiGenerator::get_sequentInjectionStmtString(Target &t, const std::string& word //expression
		) {
	std::stringstream ret;
	ret << "SEQ_TARGET_INJECT_W(gTD." << get_targetdictionaryTargetClassDeclName(t) << ", " << word;
	ret << ")";
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

std::string VapiGenerator::get_targetdictionaryTargetClassDeclName(Target &t) {
	std::string ret = "e_";
	ret += t.get_hierarchyDedotted();
	return (ret);
}

std::string VapiGenerator::get_targetdictionaryTargetClassDefName(Target &t) {
	std::string ret = "TDentry_";
	ret += t.get_hierarchyDedotted();
	return (ret);
}

std::string VapiGenerator::get_targetdictionaryEntryTypeDefString(Target &t) {
	std::stringstream ss;
	ss << "/* (TDentry-Id " << t.get_index() << "):" << t << " */" << std::endl;
	ss << "class " << get_targetdictionaryTargetClassDefName(t) << ": public TDentry {" << std::endl;
	ss << "public:" << std::endl;
//	ss << "\t\t" << "unsigned bits;" << std::endl;

	ss << "\t" << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << "* data;" << "\t// " << t.mElData.vrtlCxxType << std::endl;
	if (t.mElData.words <= 1) {
		ss << "\t" << t.mElData.vrtlCxxType << " mask;" << std::endl;
		ss << "\t" << "void reset_mask(void){mask = 0;}" << std::endl;
		if (t.mElData.vrtlCxxType == "QData") {
			ss << "\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_QO(1, bit, mask, 0);}" << std::endl;
		} else {
			ss << "\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}" << std::endl;
		}
	} else {
		std::string basetype = t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find(" ["));
		ss << "\t" << basetype << " mask[" << t.mElData.words << "];" << std::endl;
		ss << "\t" << "void reset_mask(void){" << std::endl;
		ss << "\t\t for (size_t word = 0; word < " << t.mElData.words << " ; ++word) { " << std::endl;
		ss << "\t\t\t" << "mask[word] = 0;" << std::endl;
		ss << "\t\t}" << std::endl;
		ss << "\t}" << std::endl;
		ss << "\t" << "void set_maskBit(unsigned bit) {" << std::endl;
		if(basetype == "WData"){
			ss << "\t\tVL_ASSIGNBIT_WO" << "(1, bit, mask, 1);"  << std::endl;
		} else if (basetype == "QData") {
			ss << "\t\tVL_ASSIGNBIT_QO" << "(1, bit, mask[bit / " << t.mElData.words << "], 1);"  << std::endl;
		} else {
			util::logging::log(util::logging::ERROR, std::string("CType of injection target not supported for mask method: ") + t.mElData.vrtlCxxType);
		}
		ss << "\t}" << std::endl;
		
	}
	ss <<
"	void read_data(uint8_t* pData) { \n\
		unsigned byte = 0; \n\
		uint8_t* xData = reinterpret_cast<uint8_t*>(data); \n\
		for(unsigned bit = 0; bit < bits; bit++){ \n\
			if((bit % 8)==0){ \n\
				pData[byte] = xData[byte]; \n\
				byte++; \n\
			} \n\
		} \n\
	}" << std::endl;

	ss <<
"	" << get_targetdictionaryTargetClassDefName(t) << "(const char* name, " << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << "* data) \n\
		: TDentry(name, " << t.index << ", " << t.mElData.nmbBits << ") \n\
		, data(data) \n\
		, mask() {} \n\
};" << std::endl;

	t.mTD_typedef = ss.str();

	return (t.mTD_typedef);
}

int VapiGenerator::build_API(void) {
	std::string api_dir = outdir_ + std::string("/") + std::string( API_DIRPREFIX) + std::string("/");

	if( !fs::exists(fs::path(api_dir)) ) {
		fs::create_directory( fs::path(api_dir) );
	}
	vapisrc_.write( api_dir + get_apisource_filename());
	vapiheader_.write( api_dir + get_apiheader_filename());

	for (const auto &it : mTargets) {
		if ((it->mSeqInjCnt / it->mElData.words) > 2) {
			util::logging::log(util::logging::WARNING, std::string("More than 2 sequential injections for: ") + it->_self());
		}
	}
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
