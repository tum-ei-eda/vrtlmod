////////////////////////////////////////////////////////////////////////////////
/// @file apibuilder.cpp
/// @date Created on Mon Jan 07 14:12:11 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <APIbuild/apibuilder.hpp>
#include <APIbuild/target.hpp>
#include <APIbuild/utils.hpp>

#include <string>
#include <sstream>

#include <fstream>
#include "ftcv/Misc.h"

#include <chrono>
#include <ctime>

APIbuilder::APIbuilder(void) :
		outdir() {
}

int APIbuilder::init(const char *pTargetXmlFile, const char *pOutdir) {
	outdir = pOutdir;
	return (XmlHelper::init(pTargetXmlFile));
}

std::string APIbuilder::getTDExternalDecl(void) {
	static bool once = false;
	if (!once) {
		once = true;
		return ("sTD_t& gTD = VRTLmodAPI::i().get_struct(); \t// global target dictionary\n");
	} else {
		return ("extern sTD_t& gTD;\n");
	}
}

std::string APIbuilder::getInludeStrings(void) {
	std::stringstream ret;
	ret << "/* Includes for Target Injection API */" << std::endl;
	ret << "#include \"" << "VRTLmodAPI/TD/" << API_TD_HEADER_NAME << "\"" << std::endl;
	ret << "#include \"VRTLmodAPI/vrtlmod_api.hpp\"" << std::endl;
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
//	ss << "\t\t" << "unsigned bits;" << std::endl;

	ss << "\t\t" << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << "* data;" << "\t// " << t.mElData.vrtlCxxType << std::endl;
	if (t.mElData.words <= 1) {
		ss << "\t\t" << t.mElData.vrtlCxxType << " mask;" << std::endl;
		ss << "\t\t" << "void reset_mask(void){mask = 0;}" << std::endl;
		if (t.mElData.vrtlCxxType == "QData") {
			ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_QO(1, bit, mask, 0);}" << std::endl;
		} else {
			ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}" << std::endl;
		}
	} else {
		ss << "\t\t" << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("[")) << " mask[" << t.mElData.words << "];" << std::endl;
		ss << "\t\t" << "void reset_mask(void){" << std::endl;
		for (unsigned i = 0; i < t.mElData.words; i++) {
			ss << "\t\t\t" << "mask[" << i << "] = 0;" << std::endl;
		}
		ss << "\t\t}" << std::endl;
		ss << "\t\t" << "void set_maskBit(unsigned bit){VL_ASSIGNBIT_WO(1, bit, mask, 1);}" << std::endl;
	}
	ss <<
			"void read_data(uint8_t* pData) { \n\
				unsigned byte = 0; \n\
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); \n\
				for(unsigned bit = 0; bit < bits; bit++){ \n\
					if((bit % 8)==0){ \n\
						pData[byte] = xData[byte]; \n\
						byte++; \n\
					} \n\
				} \n\
			}" << std::endl;

	ss << "\t\t" << get_targetdictionaryTargetClassDefName(t) << "(const char* name, " << t.mElData.vrtlCxxType.substr(0, t.mElData.vrtlCxxType.find("["))
			<< "* data) :" << std::endl;
	ss << "\t\t\t" << "TDentry(name, " << t.index << ", "<< t.mElData.nmbBits << "), data(data), mask() {}" << std::endl;
	ss << "};" << std::endl;

	t.mTD_typedef = ss.str();

	return (t.mTD_typedef);
}

int APIbuilder::build_API(void) {
	std::stringstream tmp;

	std::string ret = utils::system::exec("echo ${VRTLMOD_SRCDIR}");
	std::string vrtlmoddir = ret;

	utils::strhelp::replaceAll(vrtlmoddir, "\n", "");

	tmp << "cp -r " << vrtlmoddir << "/APItemplates/* " << get_outputDir() << " && " << "mkdir -p " << get_outputDir() << "/" << API_DIRPREFIX << "/"
		<< API_TD_DIRPREFIX;
	utils::system::exec(tmp.str().c_str());
	return (build_targetdictionary());
}

int APIbuilder::build_targetdictionary(void) {
	std::stringstream tgtdir;
	tgtdir << outdir << "/" << API_DIRPREFIX;
	if (build_targetdictionary_HPP((tgtdir.str() + "/TD").c_str()) > 0) {
		if (build_API_CPP((tgtdir.str()).c_str()) <= 0) {
			return (-2);
		}
		if (build_API_HPP((tgtdir.str()).c_str()) <= 0) {
			return (-2);
		}
	} else {
		return (-1);
	}

	for (const auto &it : mTargets) {
		if ((it->mSeqInjCnt / it->mElData.words) > 2) {
			ftcv::log(ftcv::WARNING, std::string("More than 2 sequential injections for: ") + it->_self());
		}
	}
	return (1);
}

int APIbuilder::build_targetdictionary_HPP(const char *outputdir) {
	std::string filepath = outputdir;
	filepath += "/";
	filepath += API_TD_HEADER_NAME;
	std::ifstream ifile(filepath);
	std::stringstream filetemplate;
	std::ofstream file;

	if (ifile.is_open()) {
		std::string tmp;
		while (std::getline(ifile, tmp)) {
			if (tmp.find("//<INSERT_HEADER_COMMMENT>") != std::string::npos) {
				filetemplate << get_fileheader(API_TD_HEADER_NAME);
			} else if (tmp.find("//<INSERT_TD_CLASSES>") != std::string::npos) {
				for (auto const &it : mTargets) {
					filetemplate << std::endl << get_targetdictionaryEntryTypeDefString(*it);
				}
				filetemplate << std::endl << "typedef struct sTD {" << std::endl;
				for (auto const &it : mTargets) {
					filetemplate << "\t" << get_targetdictionaryTargetClassDefName(*it) << "& " << get_targetdictionaryTargetClassDeclName(*it) << ";"
							<< std::endl;
				}
				filetemplate << "\tsTD(" << std::endl;
				bool first = true;
				for (auto const &it : mTargets) {
					if (first) {
						first = false;
					} else {
						filetemplate << ", " << std::endl;
					}
					filetemplate << "\t\t" << get_targetdictionaryTargetClassDefName(*it) << "& a" << it->index;
				}
				filetemplate << ") : " << std::endl;
				first = true;
				for (auto const &it : mTargets) {
					if (first) {
						first = false;
					} else {
						filetemplate << "," << std::endl;
					}
					filetemplate << "\t\t\t " << get_targetdictionaryTargetClassDeclName(*it) << "(a" << it->index << ")";
				}
				filetemplate << "{}" << std::endl;
				filetemplate << "} sTD_t;" << std::endl;

			} else {
				utils::strhelp::replace(tmp, "//<INSERT_TOP_INCLUDE>", std::string("#include \"") + mTopTypeName + ".h\"");
				utils::strhelp::replace(tmp, "<INSERT_VTOPTYPE>", mTopTypeName);
				filetemplate << tmp << std::endl;
			}
		}
		ifile.close();
	}

	file.open(filepath);
	if (file.fail()) {
		return (-1);
	}
	file << filetemplate.str();

	file.close();

	return (1);
}

int APIbuilder::build_API_HPP(const char *outputdir) {
	std::ofstream file;
	std::string filepath = outputdir;
	filepath += "/";
	filepath += API_TD_API_HEADER_NAME;
	std::ifstream ifile(filepath);
	std::stringstream filetemplate;


	if (ifile.is_open()) {
		std::string tmp;
		while (std::getline(ifile, tmp)) {
			if (tmp.find("//<INSERT_HEADER_COMMMENT>") != std::string::npos) {
				filetemplate << get_fileheader(API_TD_API_HEADER_NAME);
			} else {
				utils::strhelp::replace(tmp, "//<INSERT_TOP_INCLUDE>", std::string("#include \"") + mTopTypeName + ".h\"");
				utils::strhelp::replace(tmp, "<INSERT_VTOPTYPE>", mTopTypeName);
				filetemplate << tmp << std::endl;
			}
		}
		ifile.close();
	}

	file.open(filepath);
	if (file.fail()) {
		return (-1);
	}
	file << filetemplate.str();
	file.close();

	return (1);
}

int APIbuilder::build_API_CPP(const char *outputdir) {
	std::string filepath = outputdir;
	filepath += "/";
	filepath += API_TD_SOURCE_NAME;
	std::ifstream ifile(filepath);
	std::stringstream filetemplate;
	std::ofstream file;

	if (ifile.is_open()) {
		std::string tmp;
		while (std::getline(ifile, tmp)) {
			filetemplate << tmp << std::endl;
		}
		ifile.close();
	}

	file.open(filepath);
	if (file.fail()) {
		return (-1);
	}
	file << get_fileheader(API_TD_SOURCE_NAME);

	file << std::endl;
	file << "// Vrtl-specific includes:" << std::endl;
	file << "#include \"" << "../" << mTopTypeName << ".h\"" << std::endl;
	file << "#include \"" << "../" << mTopTypeName << "__Syms.h\"" << std::endl;
	file << "// General API includes:" << std::endl;
	file << filetemplate.str();
	file << std::endl;

	file << std::endl;
	file << "VRTLmodAPI::VRTLmodAPI(void) :" << std::endl << "\tmVRTL(* new " << mTopTypeName << ")," << std::endl << "TD_API()" << std::endl
			<< "{TD_API::init(mVRTL);}";
	file << std::endl;

	file << "void TD_API::init(" << mTopTypeName << "& pVRTL){" << std::endl;
	file << "mTD = new sTD(" << std::endl;
	std::string top = "pVRTL";

	bool first = true;
	for (auto const &it : mTargets) {
		if (first) {
			first = false;
		} else {
			file << "," << std::endl;
		}
		file << "\t\t* new " << get_targetdictionaryTargetClassDefName(*it) << "(\"" << it->mElData.hierarchy << "\", ";
		std::string hier = it->get_hierarchy();
		auto fdot = hier.find(".");
		if (fdot != std::string::npos) {
			if (it->mElData.words > 1) {
				file << top << ".__VlSymsp->TOPp->" << hier.substr(0, fdot) << "->" << hier.substr(fdot + 1);
			} else {
				file << "&(" << top << ".__VlSymsp->TOPp->" << hier.substr(0, hier.find(".")) << "->" << hier.substr(hier.find(".") + 1) << ")";
			}
		} else {
			if (it->mElData.words > 1) {
				file << top << ".__VlSymsp->TOPp->" << hier;
			} else {
				file << "&(" << top << ".__VlSymsp->TOPp->" << hier << ")";
			}
		}
		file << ")";
	}
	file << std::endl << "\t);" << std::endl;
	file << std::endl;
	for (auto const &it : mTargets) {
		file << "\tmEntryList.push_back(&(mTD->" << get_targetdictionaryTargetClassDeclName(*it) << "));" << std::endl;
	}
	file << "}" << std::endl;

	file.close();

	return (1);
}

std::string APIbuilder::get_fileheader(const char *filename) {
	std::stringstream x;
	std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	x << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
	x << "/// @file " << API_TD_API_HEADER_NAME << std::endl;
	x << "/// @brief Modified VRTL-API main header" << std::endl;
	x << "/// @details Automatically generated from: " << mFilepath << std::endl;
	x << "/// @date Created on " << std::ctime(&timestamp);
	x << "/// @author APIbuilder version " << APIBUILDER_VERSION << std::endl;
	x << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
	return (x.str());
}
