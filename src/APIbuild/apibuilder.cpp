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
	return (ret.str());
}

std::string APIbuilder::get_intermittenInjectionStmtString(Target &t) {
	std::stringstream ret;
	if (t.mElData.vrtlCxxType.find("WData") == std::string::npos) {
		ret << "INT_TARGET_INJECT(gTD.TDe_" << t.get_hierarchyDedotted();
	} else { // insert word accessed write
		int words = -1;
		for (int i = t.mElData.nmbBits; i >= 0; i -= 32) {
			words++;
		}
		ret << "INT_TARGET_INJECT_W(gTD.TDe_" << t.get_hierarchyDedotted() << ", " << words; //word;
	}
	ret << ")";

	return (ret.str());
}

std::string APIbuilder::get_sequentInjectionStmtString(Target &t, int word //expression
		) {
	std::stringstream ret;
	if (word < 0) { // insert simple variable access
		ret << "SEQ_TARGET_INJECT(gTD.TDe_" << t.get_hierarchyDedotted();

	} else { // insert word accessed write
		ret << "SEQ_TARGET_INJECT_W(gTD.TDe_" << t.get_hierarchyDedotted() << ", " << word;
	}
	ret << ")";

	return (ret.str());
}

int APIbuilder::isExprTarget(const char *pExpr) {
	int ret = -1;
	std::string obj = pExpr;
	auto pos = obj.find("->");
	if (pos == std::string::npos){
		ftcv::log(ftcv::WARNING, std::string("Possible target has no symbol table prefix: ").append(pExpr));
		return (-1);
	}else{
		std::string target = obj.substr(pos +2);
		obj = obj.substr(0, pos);

		for (auto const &it : mTargets) {
			ret++;
			std::string tExpr = it->get_hierarchyDedotted();
	//		if (std::string(pExpr).find() != std::string::npos) {
			if(target == tExpr){
				return ret;
			}
		}
	}
	return (-1);
}

Target& APIbuilder::getExprTarget(int idx) {
	return (*mTargets.at(idx));
}

std::string APIbuilder::get_targetdictionarytypedef(Target &t) {
	std::string ret = "TDentry_";
	ret += t.get_hierarchyDedotted();
	return (ret);
}

std::string APIbuilder::get_targetdictionaryEntryTypeDefString(Target &t) {
	std::stringstream ss;
	ss << "/* (TDentry-Id " << t.get_index() << "):" << t << " */" << std::endl;
	ss << "class " << get_targetdictionarytypedef(t) << ": public TDentry {" << std::endl;
	ss << "\t" << "public:" << std::endl;
	ss << "\t\t" << "unsigned bits;" << std::endl;
	ss << "\t\t" << t.mElData.vrtlCxxType << "* data;" << std::endl;

	if ((t.mElData.vrtlCxxType == "CData") or (t.mElData.vrtlCxxType == "SData") or (t.mElData.vrtlCxxType == "IData") or (t.mElData.vrtlCxxType == "QData")) {
		ss << "\t\t" << t.mElData.vrtlCxxType << " mask;" << std::endl;

		ss << "\t\t" << "void reset_mask(void){mask = 0;}" << std::endl;
		if (t.mElData.vrtlCxxType == "QData") {
			ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_QO(1, bit, mask, 0);}" << std::endl;
		} else {
			ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}" << std::endl;
		}
	} else if (t.mElData.vrtlCxxType.find("WData") != std::string::npos) {
		int words = -1;
		for (int i = t.mElData.nmbBits; i >= 0; i -= 32) {
			words++;
		}
		if (words <= 0) {
			std::cout << "Critical error: WData with less than 32 bits";
		}
		ss << "\t\t" << t.mElData.vrtlCxxType << " mask[" << words << "];" << std::endl;

		ss << "\t\t" << "void reset_mask(void){" << std::endl;
		for (int i = 0; i < words; i++) {
			ss << "\t\t\t" << "mask[" << i << "] = 0;" << std::endl;
		}
		ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_WO(1, bit, mask, 1);}" << std::endl;
	}
	ss << "\t\t" << get_targetdictionarytypedef(t) << "(const char* name, " << t.mElData.vrtlCxxType << "* data) :" << std::endl;
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
		file << "\t" << get_targetdictionarytypedef(*it) << " TDe_" << it->get_hierarchyDedotted() << ";" << std::endl;
	}

	file << "\tsTD(";
	for (auto const &it : mTargets) {
		file << get_targetdictionarytypedef(*it) << " a" << it->index << ", ";
	}
	file << ") : " << std::endl;
	bool first = true;
	for (auto const &it : mTargets) {
		if (first) {
			first = false;
		} else {
			file << "," << std::endl;
		}
		file << "\t\tTDe_" << it->get_hierarchyDedotted() << "(a" << it->index << ")";
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
	file << "#include \"" << "../" << API_TD_HEADER_NAME << "\"" << std::endl;
	file << "#include \"" << "../" << mTopName << ".h\"" << std::endl;
	file << "#include \"" << "../" << mTopName << "__Syms.h\"" << std::endl;

	file << std::endl;
	file << mTopName << " gTop;" << std::endl;

	file << std::endl;
	file << "sTD_t gTD ( " << std::endl;
	bool first = true;
	for (auto const &it : mTargets) {
		if (first) {
			first = false;
		} else {
			file << "," << std::endl;
		}
		file << "\t" << get_targetdictionarytypedef(*it) << "(\"" << it->mElData.name << "\", ";
		if (it->mElData.vrtlCxxType == "WData") {
			file << "(gTop.__VlSymsp->TOPp->" << it->get_hierarchy() << "))";
		} else {
			file << "&(gTop.__VlSymsp->TOPp->" << it->get_hierarchy() << "))";
		}
	}
	file << std::endl << ");" << std::endl;
	file.close();

	return (1);
}
