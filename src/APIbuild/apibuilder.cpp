////////////////////////////////////////////////////////////////////////////////
/// @file apibuilder.cpp
/// @date Created on Mon Jan 07 14:12:11 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <APIbuild/apibuilder.hpp>
#include <APIbuild/target.hpp>
#include <string>
#include <sstream>

#include <fstream>
#include "ftcv/Misc.h"

#include <chrono>
#include <ctime>

APIbuilder::APIbuilder(void) : outdir() {

}

int APIbuilder::init(const char *pTargetXmlFile, const char *pOutdir) {
	outdir = pOutdir;
	return (XmlHelper::init(pTargetXmlFile));
}

std::string APIbuilder::getInludeStrings(void) {
	std::stringstream ret;
	ret << "/* Includes for Target Injection API */" << std::endl;
	ret << "#include \"" << "TD/" << API_TD_HEADER_NAME << "\"" << std::endl;
	ret << "#include \"InjAPI/injectapi.hpp\"" << std::endl;
	ret << "extern sTD_t gTD; \t// global target dictionary" << std::endl;

	return (ret.str());
}

std::string APIbuilder::get_intermittenInjectionStmtString(Target &t) {
	std::stringstream ret;
	if (t.mElData.vrtlCxxType.find("WData") == std::string::npos) {
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

std::string APIbuilder::get_sequentInjectionStmtString(Target &t, int word //expression
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

int APIbuilder::isExprTarget(const char *pExpr) {
	int ret = -1;
	ExprT x = ExprT(pExpr);
	std::string expre;
	if(x.prefix == "vlSymsp"){
		expre = //this->mTopName +
				x.object + std::string(".") + x.name;
	}else if(x.prefix == "vlTOPp"){
		expre = //this->mTopName +
				std::string("TOP.") + x.name;
	}
	for (auto const &it : mTargets) {
		ret++;
		std::string target = it->mElData.hierarchy; // = it->get_hierarchy(); //TOP.top.<name>
		if(target == expre){
			return ret;
		}
	}
	return (-1);
}

Target& APIbuilder::getExprTarget(int idx) {
	return (*mTargets.at(idx));
}

std::string APIbuilder::get_targetdictionaryTargetClassDeclName(Target &t) {
	std::string ret = "e_";
	ret += t.get_hierarchyDedotted();
	return (ret);
}

std::string APIbuilder::get_targetdictionaryTargetClassDefName(Target &t) {
	std::string ret = "TDentry_";
	ret += t.get_hierarchyDedotted();
	return (ret);
}

std::string APIbuilder::get_targetdictionaryEntryTypeDefString(Target &t) {
	std::stringstream ss;
	ss << "/* (TDentry-Id " << t.get_index() << "):" << t << " */" << std::endl;
	ss << "class " << get_targetdictionaryTargetClassDefName(t) << ": public TDentry {" << std::endl;
	ss << "\t" << "public:" << std::endl;
	ss << "\t\t" << "unsigned bits;" << std::endl;

	ss << "\t\t" << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << "* data;" << "\t// " << t.mElData.vrtlCxxType << std::endl;
	if(t.mElData.words <= 1){
		ss << "\t\t" << t.mElData.vrtlCxxType << " mask;" << std::endl;
		ss << "\t\t" << "void reset_mask(void){mask = 0;}" << std::endl;
		if (t.mElData.vrtlCxxType == "QData") {
			ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_QO(1, bit, mask, 0);}" << std::endl;
		} else {
			ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}" << std::endl;
		}
	}else{
		ss << "\t\t" << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << " mask[" << t.mElData.words << "];" << std::endl;
		ss << "\t\t" << "void reset_mask(void){" << std::endl;
		for (unsigned  i = 0; i < t.mElData.words; i++) {
			ss << "\t\t\t" << "mask[" << i << "] = 0;" << std::endl;
		}
		ss << "\t\t}" << std::endl;
		ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_WO(1, bit, mask, 1);}" << std::endl;
	}

	ss << "\t\t" << get_targetdictionaryTargetClassDefName(t) << "(const char* name, " << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << "* data) :" << std::endl;
	ss << "\t\t\t" << "TDentry(name, " << t.index << "), data(data), mask(), bits(" << t.mElData.nmbBits << ") {}" << std::endl;
	ss << "};" << std::endl;

	t.mTD_typedef = ss.str();

	return (t.mTD_typedef);
}

int APIbuilder::build_API(void) {
	std::stringstream tmp;
	tmp << "mkdir -p " << get_outputDir() << " && ";
	tmp << "mkdir -p " << get_outputDir() << "/" << API_TD_DIRPREFIX << " && ";
	tmp << "cp -r APItemplates/* " << get_outputDir();
	system(tmp.str().c_str());
	return (build_targetdictionary());
}

int APIbuilder::build_targetdictionary(void) {
	std::string tgtdir = outdir;
	tgtdir += "/";
	tgtdir += API_TD_DIRPREFIX;
	if (build_targetdictionary_HPP(tgtdir.c_str()) > 0) {
		if (build_targetdictionary_CPP(tgtdir.c_str()) <= 0) {
			return (-2);
		}
	} else {
		return (-1);
	}

	for(const auto & it: mTargets){
		if ((it->mSeqInjCnt / it->mElData.words) > 2){
			ftcv::log(ftcv::WARNING, std::string("More than 2 sequential injections for: ")+it->_self());
		}
	}
	return (1);
}

int APIbuilder::build_targetdictionary_HPP(const char *outputdir) {

	std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	std::string filepath = outputdir;
	filepath += "/";
	filepath += API_TD_HEADER_NAME;
	std::ofstream file;
	file.open(filepath);
	if (file.fail()) {
		return (-1);
	}

	file << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
	file << "/// @file " << API_TD_HEADER_NAME << std::endl;
	file << "/// @brief Injection Target Dictionary for injection-modified VRTL header" << std::endl;
	file << "/// @details Automatically generated from: " << mFilepath << std::endl;
	file << "/// @date Created on " << std::ctime(&timestamp);
	file << "/// @author APIbuilder version " << APIBUILDER_VERSION << std::endl;
	file << "////////////////////////////////////////////////////////////////////////////////" << std::endl;

	file << std::endl;
	file << "#ifndef INJECTION_TD_H" << std::endl;
	file << "#define INJECTION_TD_H" << std::endl;

	file << std::endl;
	file << "#include \"verilated.h\"" << "//v4.023" << std::endl;
	file << "#include \"" << "../InjAPI/injectapi.hpp\"" << std::endl;

	file << std::endl;

	for (auto const &it : mTargets) {
		file << std::endl;
		file << get_targetdictionaryEntryTypeDefString(*it);
	}

	file << std::endl;
	file << "typedef struct sTD {" << std::endl;
	for (auto const &it : mTargets) {
		file << "\t" << get_targetdictionaryTargetClassDefName(*it) << "& " << get_targetdictionaryTargetClassDeclName(*it) << ";" << std::endl;
	}

	file << "\tsTD(";
	bool first = true;
	for (auto const &it : mTargets) {
		if (first) {
			first = false;
		} else {
			file << ", " << std::endl;
		}
		file << "\t" << get_targetdictionaryTargetClassDefName(*it) << "& a" << it->index;
	}
	file << ") : " << std::endl;
	first = true;
	for (auto const &it : mTargets) {
		if (first) {
			first = false;
		} else {
			file << "," << std::endl;
		}
		file << "\t\t " << get_targetdictionaryTargetClassDeclName(*it) << "(a" << it->index << ")";
	}
	file << "{}" << std::endl;
	file << "} sTD_t;" << std::endl;

	file << std::endl;
	file << "#endif //INJECTION_TD_H" << std::endl;

	file.close();

	return (1);
}

int APIbuilder::build_targetdictionary_CPP(const char *outputdir) {
	std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

//	std::stringstream ss_file;
	std::string filepath = outputdir;
	filepath += "/";
	filepath += API_TD_SOURCE_NAME;

	std::ofstream file;
	file.open(filepath);
	if (file.fail()) {
		return (-1);
	}
	// write file head
	file << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
	file << "/// @file " << API_TD_SOURCE_NAME << std::endl;
	file << "/// @brief Injection Target Dictionary for injection-modified VRTL sources" << std::endl;
	file << "/// @details Automatically generated from: " << mFilepath << std::endl;
	file << "/// @date Created on " << std::ctime(&timestamp);
	file << "/// @author APIbuilder version " << APIBUILDER_VERSION << std::endl;
	file << "////////////////////////////////////////////////////////////////////////////////" << std::endl;

	file << std::endl;
	file << "#include \"verilated.h\"" << "// v4.023" << std::endl;
	file << "#include \"" << API_TD_HEADER_NAME << "\"" << std::endl;
	file << "#include \"" << "../" << mTopTypeName << ".h\"" << std::endl;
	file << "#include \"" << "../" << mTopTypeName << "__Syms.h\"" << std::endl;

	file << std::endl;
	file << mTopTypeName << " gTop;" << std::endl;

	file << std::endl;
	file << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
	file << "/// @brief Global target dictionary" << std::endl;
	file << "/// @details Initializer list is generated according to definition in " << API_TD_HEADER_NAME << std::endl;
	file << "sTD_t gTD ( " << std::endl;
	bool first = true;
	for (auto const &it : mTargets) {
		if (first) {
			first = false;
		} else {
			file << "," << std::endl;
		}
		file << "\t* new " << get_targetdictionaryTargetClassDefName(*it) << "(\"" << it->mElData.name << "\", ";
		std::string hier = it->get_hierarchy();
		auto fdot = hier.find(".");
		if(fdot != std::string::npos){
			if (it->mElData.vrtlCxxType.find("WData") != std::string::npos) {
				file << "(gTop.__VlSymsp->TOPp->" << hier.substr(0,fdot) << "->" << hier.substr(fdot +1) << "))";
			} else {
				file << "&(gTop.__VlSymsp->TOPp->" << hier.substr(0, hier.find(".")) << "->" << hier.substr(hier.find(".") +1) << "))";
			}
		}else{
			if (it->mElData.vrtlCxxType.find("WData") != std::string::npos) {
				file << "(gTop.__VlSymsp->TOPp->" << it->get_hierarchy() << "))";
			} else {
				file << "&(gTop.__VlSymsp->TOPp->" << it->get_hierarchy() << "))";
			}
		}

	}
	file << std::endl << ");" << std::endl;
	file.close();

	return (1);
}
