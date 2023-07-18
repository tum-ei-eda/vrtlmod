/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

////////////////////////////////////////////////////////////////////////////////
/// @file core.cpp
/// @date Created on Mon Feb 28 12:29:21 2022
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"
#include "vrtlmod/passes/pass.hpp"
#include "vrtlmod/util/logging.hpp"

#include "vrtlmod/vapi/generator.hpp"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/TextNodeDumper.h"

#include <pugixml.hpp>
#include <regex>
#include <algorithm>

#include <boost/lexical_cast.hpp>

namespace vrtlmod
{

VrtlmodCore::VrtlmodCore(const char *out_dir_path, bool systemc)
    : out_dir_path_(out_dir_path)
    , systemc_(systemc)
    , ctx_(std::make_unique<Context>())
    , gen_(std::make_unique<vapi::VapiGenerator>(*this))
{
    // build up xml document
    ctx_->xml_doc_ = std::make_unique<pugi::xml_document>();
    auto root = ctx_->xml_doc_->append_child("vrtlmod_xml");
    ctx_->xml_root_node_ = std::make_unique<pugi::xml_node>(root);
    ctx_->xml_root_node_->append_attribute("version") = VRTLMOD_VERSION;

    // ctx_->xml_root_node_ = std::make_unique<pugi::xml_node>(std::move(ctx_->xml_doc_->append_child("vrtlmod_xml")));
    auto netlist = ctx_->xml_root_node_->append_child("top");
    ctx_->xml_netlist_node_ = std::make_unique<pugi::xml_node>(netlist);

    auto modules = ctx_->xml_root_node_->append_child("modules");
    ctx_->xml_modules_node_ = std::make_unique<pugi::xml_node>(modules);

    auto files = ctx_->xml_root_node_->append_child("files");
    ctx_->xml_files_node_ = std::make_unique<pugi::xml_node>(files);
}

VrtlmodCore::~VrtlmodCore(void) {}

void VrtlmodCore::print_targetdictionary(void) const
{
    std::cout << gen_->get_td_string();
}

std::vector<std::string> VrtlmodCore::prepare_files(const std::vector<std::string> &files,
                                                    const std::vector<std::string> &file_ext_matchers, bool overwrite)
{
    // clean file list
    std::vector<std::string> nfiles;
    for (const auto &file : files)
    {
        if (fs::is_regular_file(fs::path(file)) == false)
        {
            LOG_WARNING("Input [", file, "] is not a file. Remove from input set.");
        }
        else
        {
            for (const auto &ext_matcher : file_ext_matchers)
            {
                auto dext = file.rfind(ext_matcher);
                if (dext != std::string::npos)
                {
                    nfiles.push_back(file);
                    break;
                }
            }
        }
    }
    auto outdir = get_output_dir();
    if (!fs::is_directory(outdir))
    {
        LOG_INFO("Creating directory [", outdir.string(), "]");
        auto ret = create_directory(outdir);
        if (ret == false)
        {
            LOG_ERROR("Failed to create output directory [", outdir.string(), "]");
        }
    }
    if (overwrite == false)
    {
        for (auto &nfile : nfiles)
        {

            std::string srcName;
            auto lSl = nfile.rfind("/");
            auto dext = std::string::npos;
            std::string file_ext = "";
            fs::path tmp = outdir / nfile.substr(lSl); // << file_ext_matchers[0];
            LOG_INFO("Accepted file [", tmp.string(), "]");
            fs::copy_file(fs::path(nfile), tmp, fs::copy_option::overwrite_if_exists);
            nfile = tmp.string();
        }
    }
    return nfiles;
}

void clean_file(const std::string &file_path)
{
    LOG_VERBOSE("> cleaning file", file_path);

    std::ifstream in(file_path.c_str());
    if (!in.is_open())
    {
        LOG_FATAL("Clean File could not open file path [", file_path, "]");
        return;
    }

    std::stringstream ss;

    while (in.good())
    {
        char c;
        in.get(c);
        ss << c;
    }
    in.close();

    std::string data = ss.str();
    ss.str("");
    size_t pos = 0;
    do
    {
        pos = data.find("/*verilator_public*/", pos); // TODO: what are these again?
        if (pos != std::string::npos)
            data.replace(pos, 20, "//verilator_public");

    } while (pos != std::string::npos);
    pos = 0;
    do
    {
        pos = data.find("struct {", pos); // TODO: what are these again?
        if (pos != std::string::npos)
        {
            LOG_VERBOSE(">>> replace anonymous struct.");
            data.replace(pos, 8, "");
            auto closepos = data.find("};", pos);
            data.replace(closepos, 2, "");
        }

    } while (pos != std::string::npos);

    std::ofstream out(file_path.c_str());
    if (!out.is_open())
    {
        LOG_FATAL("Clean File could not open file path [", file_path, "]");
        return;
    }

    out << data;

    out.flush();
    out.close();
}

std::vector<std::string> VrtlmodCore::prepare_sources(const std::vector<std::string> &files, bool overwrite)
{
    return prepare_files(files, { ".cpp", ".cc" }, overwrite);
}
std::vector<std::string> VrtlmodCore::prepare_headers(const std::vector<std::string> &files, bool overwrite)
{
    auto headers = prepare_files(files, { ".h", ".hpp" }, overwrite);
    for (const auto &it : headers)
    {
        clean_file(it);
    }
    gen_->build_targetdictionary(); // write the common target dictionary header to the output dir

    return headers;
}

std::string VrtlmodCore::get_vrtltopheader_filename(void) const
{
    std::string top_name = get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202

#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    return (top_name + ".h");
}
std::string VrtlmodCore::get_vrtltopsymsheader_filename(void) const
{
    std::string top_name = get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202

#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    return (top_name + "__Syms.h");
}

std::string VrtlmodCore::getTDExternalDecl(void) const
{
    static bool once = false;
    std::stringstream ret;
    if (!once)
    {
        once = true;
        ret << ctx_->top_cell_->get_type() << "VRTLmodAPI& gTD = " << ctx_->top_cell_->get_type()
            << "VRTLmodAPI::i(); \t// global target dictionary (api)\n";
    }
    else
    {
        ret << "extern " << ctx_->top_cell_->get_type() << "VRTLmodAPI& gTD;\n";
    }
    return ret.str();
}

int VrtlmodCore::is_decl_target(const clang::FieldDecl *target) const
{
    int ret = -1;
    std::string t_type = target->getType().getAsString();
    std::string t_name = target->getName().str();
    std::string t_a_type = target->getParent()->getTypeForDecl()->getTypeClassName();
    std::string t_a_name = target->getParent()->getName().str();

    LOG_VERBOSE("t_a_name: ", t_a_name, " of type ", t_a_type);
    LOG_VERBOSE("t_name: ", t_name, " of type ", t_type);

    bool found = false;
    auto func = [&](const types::Target &it)
    {
        ++ret;
        std::string target_name = it.get_id();
        std::string target_parent = types::Module(it.parent()).get_id();

        LOG_VERBOSE(">> target:", target_name, " of parent ", target_parent, " of type ", it.parent().name());
        if (t_a_name == target_parent)
        {
            LOG_VERBOSE(">>> same ancestor:", t_a_name, " of type ", t_a_type);

            if (t_name == target_name)
            {
                found = true;
                LOG_VERBOSE(">>> same target:", t_name, " of type ", t_type);
                return false;
            }
        }
        return true;
    };
    foreach_injection_target(func);

    return found ? ret : -1;
}

types::Target &VrtlmodCore::get_target_from_index(int idx) const
{
    return (**std::next(ctx_->toinj_targets_.begin(), idx));
}

const types::Module *VrtlmodCore::add_module(const clang::CXXRecordDecl *module, const clang::ASTContext &ctx) const
{
    std::string id = module->getNameAsString();
    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    mod_iter =
        std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(), [id](const auto &it) { return id == it->get_id(); });
    if (mod_iter == ctx_->modules_.end())
    {
        auto xml_node = ctx_->xml_modules_node_->append_child("module");
        xml_node.append_attribute("id") = id.c_str();
        auto mod_inst = std::make_unique<types::Module>(xml_node);

        mod_inst->add_decl_loc(LOCATABLE_INITIALIZER(module->getLocation(), ctx.getSourceManager()));
        const types::Module *ret = mod_inst.get();

        ctx_->modules_.insert(std::move(mod_inst));
        return ret;
    }
    else
    {
        LOG_VERBOSE("{module}: [", id, "] already declared.");
        return nullptr;
    }
}

const types::Module *VrtlmodCore::add_module_instance(const clang::NamedDecl *instance_decl,
                                                      const clang::CXXRecordDecl *module,
                                                      const clang::ASTContext &ctx) const
{
    std::string id = module->getNameAsString();
    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    mod_iter =
        std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(), [id](const auto &it) { return id == it->get_id(); });

    if (mod_iter != ctx_->modules_.end())
    {
        (*mod_iter)->add_instance(instance_decl->getNameAsString());
        return (*mod_iter).get();
    }
    return nullptr;
}

const types::Module *VrtlmodCore::add_module_instance(const clang::MemberExpr *instance,
                                                      const clang::CXXRecordDecl *module,
                                                      const clang::ASTContext &ctx) const
{
    return add_module_instance(instance->getMemberDecl(), module, ctx);
}

const types::Cell *VrtlmodCore::add_cell(const clang::FieldDecl *cell, const clang::ASTContext &ctx) const
{

    std::string id = cell->getNameAsString();
    std::string cell_type = cell->getType().getAsString();
    util::strhelp::replaceAll(cell_type, "class ", "");
    util::strhelp::replaceAll(cell_type, "*", "");
    util::strhelp::replaceAll(cell_type, " ", "");
    std::string module_id = cell->getParent()->getName().str();

    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    mod_iter = std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(),
                            [module_id](const auto &it) { return module_id == it->get_id(); });
    if (mod_iter == ctx_->modules_.end())
    {
        LOG_VERBOSE("{cell}: [", id, "] of type [", cell_type, "] no matchting parent [", module_id, "] found");
        return nullptr;
    }

    std::set<std::unique_ptr<types::Cell>>::iterator cell_iter;
    cell_iter = std::find_if((*mod_iter)->cells_.begin(), (*mod_iter)->cells_.end(),
                             [id](const auto &it) { return id == it->get_id(); });
    if (cell_iter != (*mod_iter)->cells_.end())
    {
        LOG_VERBOSE("{cell}: [", id, "] of type [", cell_type, "]  already a member of parent [", module_id, "]");
        return nullptr;
    }

    auto xml_node = (*mod_iter)->append_child("cell");
    xml_node.append_attribute("id") = id.c_str();
    xml_node.append_attribute("type") = cell_type.c_str();

    auto cell_instance = std::make_unique<types::Cell>(xml_node); //, **mod_iter);

    cell_instance->add_decl_loc(LOCATABLE_INITIALIZER(cell->getLocation(), ctx.getSourceManager()));
    const types::Cell *ret = cell_instance.get();

    (*mod_iter)->add_cell(std::move(cell_instance));

    return ret;
}

const types::Variable *VrtlmodCore::add_variable(const clang::FieldDecl *variable, const clang::ASTContext &ctx,
                                                 const clang::Rewriter &rew) const
{
    auto &srcmgr = ctx.getSourceManager();
    auto &lang_opts = ctx.getLangOpts();

    std::string id = variable->getNameAsString();
    std::string type = variable->getType().getAsString();
    std::string module_id = variable->getParent()->getName().str();

    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    mod_iter = std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(),
                            [module_id](const auto &it) { return module_id == it->get_id(); });
    if (mod_iter == ctx_->modules_.end())
    {
        LOG_VERBOSE("{variable}: [", id, "] of parent [", module_id, "] no matching parent module found.");
        return nullptr;
    }

    std::set<std::unique_ptr<types::Variable>>::iterator var_iter;
    var_iter = std::find_if((*mod_iter)->variables_.begin(), (*mod_iter)->variables_.end(),
                            [id](const auto &it) { return id == it->get_id(); });
    if (var_iter != (*mod_iter)->variables_.end())
    {
        LOG_VERBOSE("{variable}: [", id, "] of type [", type, "]  already a member of parent [", module_id, "]");
        return nullptr;
    }

    auto next_token = clang::Lexer::findNextToken(variable->getSourceRange().getEnd(), srcmgr, lang_opts);
    if (!next_token)
    {
        return nullptr;
    }

    auto next_token_loc = next_token->getLocation();
    auto decl_complete_range = clang::SourceRange(variable->getSourceRange().getBegin(), next_token_loc);
    auto decl_source_code_text = rew.getRewrittenText(decl_complete_range);
    LOG_VERBOSE("{decl_source_range}:", decl_complete_range.printToString(srcmgr));
    LOG_VERBOSE("{decl_source_code_text}:", decl_source_code_text);

    std::regex ctype_portdecl("/\\*VL_IN[1-9]*\\*/|/\\*VL_OUT[1-9]*\\*/|/\\*VL_INOUT[1-9]*\\*/");
    std::regex wctype_portdecl("/\\*VL_INW*\\*/|/\\*VL_OUTW*\\*/|/\\*VL_INOUTW*\\*/");
    std::regex sctype_portdecl("sc_in<|sc_out<");

    std::string var_type = "undef";
    std::string msb = "msb";
    std::string lsb = "lsb";
    std::vector<std::pair<std::string, std::string>> unpacked;

    auto unpack = [&unpacked](std::string &packed_str, const std::regex &break_regex)
    {
        auto open_templ = packed_str.find("<");
        auto close_templ = packed_str.rfind(">");
        auto comma = packed_str.rfind(",");
        std::string outer_type = packed_str.substr(0, open_templ);
        std::string outer_dim = packed_str.substr(comma + 1, close_templ - (comma + 1));
        auto base = packed_str.substr(0, open_templ);
        packed_str = packed_str.substr(open_templ + 1, comma - (open_templ + 1));
        unpacked.push_back(std::make_pair<>(std::string(base), std::string(outer_dim)));
        bool cont = std::regex_search(packed_str, break_regex);
        if (!cont)
        {
            unpacked.push_back(std::make_pair<>(std::string(packed_str), std::string(outer_dim)));
        }
        return cont;
    };

    if (std::regex_search(decl_source_code_text, ctype_portdecl))
    {
        auto first_col = decl_source_code_text.rfind(id) + id.length() + 1;
        auto second_col = decl_source_code_text.find(",", first_col + 1);
        msb = decl_source_code_text.substr(first_col, second_col - first_col);
        auto last_brace = decl_source_code_text.rfind(")*/;");
        ++second_col;

        lsb = decl_source_code_text.substr(second_col, last_brace - second_col);
        var_type = std::regex_search(decl_source_code_text, std::regex("/\\*VL_IN[1-9]*\\*/")) ? "in" : "out";
        auto cxx_base_type_str = decl_source_code_text.substr(0, decl_source_code_text.find(id) - 1);
        unpacked.push_back(std::make_pair<>(std::string(cxx_base_type_str), std::string("0")));
        LOG_VERBOSE(">>>: is a ctype ", var_type, " from ", msb, " to ", lsb);
    }
    else if (std::regex_search(decl_source_code_text, wctype_portdecl))
    {
        auto first_col = decl_source_code_text.rfind(id) + id.length() + 1;
        auto second_col = decl_source_code_text.find(",", first_col + 1);
        auto third_col = decl_source_code_text.find(",", second_col + 1);
        msb = decl_source_code_text.substr(first_col, second_col - first_col);
        auto last_brace = decl_source_code_text.rfind(")*/;");
        ++second_col;
        lsb = decl_source_code_text.substr(second_col, third_col - second_col);
        var_type = std::regex_search(decl_source_code_text, std::regex("/\\*VL_INW*\\*/")) ? "in" : "out";
        auto cxx_base_type_str = decl_source_code_text.substr(0, decl_source_code_text.find(id) - 1);
#if VRTLMOD_VERILATOR_VERSION <= 4202
        auto openbrace = decl_source_code_text.find('[');
        auto closebrace = decl_source_code_text.find(']');
        cxx_base_type_str += decl_source_code_text.substr(openbrace, closebrace - openbrace + 1);
#else // VRTLMOD_VERILATOR_VERSION <= 4228
#endif
        unpacked.push_back(std::make_pair<>(std::string(cxx_base_type_str), std::string("0")));
        LOG_VERBOSE(">>>: is a wctype ", var_type, " from ", msb, " to ", lsb);
    }
    else if (std::regex_search(decl_source_code_text, sctype_portdecl))
    {
        var_type = std::regex_search(decl_source_code_text, std::regex("sc_in<")) ? "in" : "out";
        auto cxx_base_type_str = decl_source_code_text.substr(0, decl_source_code_text.find(id));

        bool still_going = false;
        std::string search = decl_source_code_text; // var_type;

        auto open_templ = search.find("<");
        auto close_templ = search.rfind(">");
        auto outer_str = search.substr(0, open_templ);
        auto inner_str = search.substr(open_templ + 1, close_templ - (open_templ + 1));

        if (std::regex_search(inner_str, std::regex("^sc_bv<")))
        {
            auto open_templ = inner_str.find("<");
            auto close_templ = inner_str.rfind(">");
            msb = std::to_string(std::stoi(inner_str.substr(open_templ + 1, close_templ - (open_templ + 1))) - 1);
            lsb = "0";
        }
        else if (std::regex_search(inner_str, std::regex("int64_")))
        {
            msb = "63";
            lsb = "0";
        }
        else if (std::regex_search(inner_str, std::regex("int32_")))
        {
            msb = "31";
            lsb = "0";
        }
        else if (std::regex_search(inner_str, std::regex("int16_")))
        {
            msb = "15";
            lsb = "0";
        }
        else if (std::regex_search(inner_str, std::regex("int8_")))
        {
            msb = "7";
            lsb = "0";
        }
        else if (std::regex_search(inner_str, std::regex("^bool|^Bool_|^Bool")))
        {
            msb = "0";
            lsb = "0";
        }
        else
        {
            msb = "undef";
            lsb = "undef";
        }

        unpacked.push_back(std::make_pair<>(std::string(cxx_base_type_str), std::string("0")));
        LOG_VERBOSE(">>>: is a sctype port ", outer_str, " - ", inner_str, "!!!");

        // TODO: Support bitfield extraction for systemc type ports (only top level is systemc, but only
        // sc_{in,out}<stdint> is supported)
    }
    else // something else aka must be a signal
    {
        if (std::regex_search(type, std::regex("^VlUnpacked<")))
        {
            bool still_going = false;
            std::string search = decl_source_code_text; // type;
            do
            {
                still_going = unpack(search, std::regex("^VlUnpacked<"));
            } while (still_going);
        }
        else // packed signal
        {
            std::string type_str;
            auto close_space = decl_source_code_text.find(" ");
            type_str = decl_source_code_text.substr(0, close_space);

            unpacked.push_back(std::make_pair<>(std::string(type_str), std::string("0")));
        }

        std::string zerodim_typestr = unpacked.back().first;
        auto open_comm = zerodim_typestr.find("/*");
        auto colon_comm = zerodim_typestr.find(":");
        auto close_comm = zerodim_typestr.rfind("*/");
        msb = zerodim_typestr.substr(open_comm + 2, colon_comm - (open_comm + 2));
        lsb = zerodim_typestr.substr(colon_comm + 1, close_comm - (colon_comm + 1));
        var_type = "var";
    }

    unpacked.back().second = util::concat(msb, ":", lsb);
    unpacked.back().first = unpacked.back().first.substr(0, unpacked.back().first.find("/*"));
    std::string bases, dim;
    long bits = 1;
    bool first_base = true;
    for (auto &pair_ : unpacked)
    {
        // now we can clean whitespaces
        pair_.first.erase(
            std::remove_if(pair_.first.begin(), pair_.first.end(), [](unsigned char x) { return std::isspace(x); }),
            pair_.first.end());
        pair_.second.erase(
            std::remove_if(pair_.second.begin(), pair_.second.end(), [](unsigned char x) { return std::isspace(x); }),
            pair_.second.end());

        if (first_base)
        {
            first_base = false;
            bases = pair_.first;
            dim = pair_.second;
        }
        else
        {
            bases = util::concat(bases, ", ", pair_.first);
            dim = util::concat(dim, ", ", pair_.second);
        }

        int new_dim_bits = 0;
        try
        {
            bits *= (pair_.second.find(":") == std::string::npos)
                        ? boost::lexical_cast<int>(pair_.second)
                        : ((boost::lexical_cast<int>(msb) + 1) - boost::lexical_cast<int>(lsb));
        }
        catch (boost::bad_lexical_cast)
        {
            bits *= 0; // force total bit length to zero to trigger invalidation.
        }
    }

    if (bits <= 0)
    {
        // FIXME: normally we should not have any string types here, since AST matchers should be configured to filter
        // them out before add_variable is called
        if (type.find("string") != std::string::npos)
        {
            LOG_WARNING("Variable is of type string (can be ignored) - Failed to extract total bit length of signal [",
                        id, "] of module [", module_id, "]. Extracted value:", std::to_string(bits));
        }
        else
        {
            // not of type string, we should stop here by throwing a fatal log+except.
            LOG_FATAL("Failed to extract total bit length of signal [", id, "] of module [", module_id,
                      "]. Extracted value:", std::to_string(bits));
        }
        return nullptr;
    }

    auto xml_node = (*mod_iter)->append_child(var_type.c_str());
    xml_node.append_attribute("id") = id.c_str();
    xml_node.append_attribute("bases") = util::concat("[", bases, "]").c_str();
    xml_node.append_attribute("dim") = util::concat("[", dim, "]").c_str();
    xml_node.append_attribute("cxx_type") = type.c_str();
    // xml_node.append_attribute("decl") = decl_source_code_text.c_str();
    xml_node.append_attribute("bits") = bits;

    auto var_inst = std::make_unique<types::Variable>(xml_node); //, **it);

    var_inst->add_decl_loc(LOCATABLE_INITIALIZER(variable->getLocation(), ctx.getSourceManager()));
    const types::Variable *ret = var_inst.get();

    (*mod_iter)->add_variable(std::move(var_inst));
    return ret;
}

const types::Cell *VrtlmodCore::set_top_cell(const clang::FieldDecl *cell, const clang::ASTContext &ctx) const
{
    std::string id = cell->getNameAsString();
    std::string cell_type = cell->getType().getAsString();
    util::strhelp::replaceAll(cell_type, "struct", "");
    util::strhelp::replaceAll(cell_type, "class ", "");
    util::strhelp::replaceAll(cell_type, "*", "");
    util::strhelp::replaceAll(cell_type, " ", "");

    std::string parent_id = cell->getParent()->getName().str();

    if (ctx_->top_cell_.get())
    {
        LOG_VERBOSE("Top cell already set to [", ctx_->top_cell_->get_id(), "] of type [", ctx_->top_cell_->get_type(),
                    "]");
        return nullptr;
    }
    else
    {
        LOG_VERBOSE("{top cell}: Set top cell instance to [", id, "] of type [", cell_type, "]");
        auto xml_node = ctx_->xml_netlist_node_->append_child("cell");
        xml_node.append_attribute("id") = id.c_str();
        xml_node.append_attribute("type") = cell_type.c_str();

        ctx_->top_cell_ = std::make_unique<types::Cell>(xml_node);

        ctx_->top_cell_->add_decl_loc(LOCATABLE_INITIALIZER(cell->getLocation(), ctx.getSourceManager()));

        return ctx_->top_cell_.get();
    }
}

int VrtlmodCore::is_expr_target(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent,
                                const clang::ASTContext &ctx) const
{
    int ret = -1;
    bool found = false;

    std::string var_id = assignee->getMemberNameInfo().getAsString(); // assignee->getNameAsString();
    std::string var_type = assignee->getType().getAsString();
    std::string module_id = parent->getName().str();
    std::string module_type = parent->getTypeForDecl()->getTypeClassName();

    LOG_VERBOSE(">>>>> INJREW: {variable}: [", var_id, "] of parent [", module_id, "]");
    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    mod_iter = std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(),
                            [module_id](const auto &it) { return module_id == it->get_id(); });
    if (mod_iter == ctx_->modules_.end())
    {
        LOG_WARNING(">>>>> INJREW: {variable}: [", var_id, "] of parent [", module_id, "] no matching module found.");
        return -1;
    }

    std::set<std::unique_ptr<types::Variable>>::iterator var_iter;
    var_iter = std::find_if((*mod_iter)->variables_.begin(), (*mod_iter)->variables_.end(),
                            [var_id](const auto &it) { return var_id == it->get_id(); });
    if (var_iter == (*mod_iter)->variables_.end())
    {
        LOG_WARNING(">>>>> INJREW: {variable}: [", var_id, "] of parent [", module_id,
                    "] no matching variable found in module.");
        return -1;
    }
    else
    {
        LOG_VERBOSE(">>>>> INJREW: {variable}: [", (*var_iter)->get_id(), "] of parent [", module_id,
                    "] found injection location at ", "<TODO>", " of set [", (*var_iter)->get_inj_loc(), "]");

        auto func = [&](const types::Target &it)
        {
            ++ret;
            if (it.is_assigned_here(assignee, parent))
            {
                found = true;
                LOG_VERBOSE(">>>>> INJREW: target is assigned here", it.get_id(), " of type ", it.get_type(),
                            " of parent ", types::Module(it.parent()).get_id());
                return false;
            }
            return true;
        };
        foreach_injection_target(func);
    }

    return found ? ret : -1;
}

const types::Variable *VrtlmodCore::add_injection_location(const clang::MemberExpr *assignee,
                                                           const clang::CXXRecordDecl *parent,
                                                           const clang::ASTContext &ctx) const
{
    std::string var_id = assignee->getMemberNameInfo().getAsString(); // assignee->getNameAsString();
    std::string var_type = assignee->getType().getAsString();
    std::string module_id = parent->getName().str();
    std::string module_type = parent->getTypeForDecl()->getTypeClassName();

    LOG_VERBOSE("{variable}: [", var_id, "] of parent [", module_id, "]");
    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    mod_iter = std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(),
                            [module_id](const auto &it) { return module_id == it->get_id(); });
    if (mod_iter == ctx_->modules_.end())
    {
        LOG_VERBOSE("{variable}: [", var_id, "] of parent [", module_id, "] no matching module found.");
        return nullptr;
    }

    std::set<std::unique_ptr<types::Variable>>::iterator var_iter;
    var_iter = std::find_if((*mod_iter)->variables_.begin(), (*mod_iter)->variables_.end(),
                            [var_id](const auto &it) { return var_id == it->get_id(); });
    if (var_iter == (*mod_iter)->variables_.end())
    {
        LOG_VERBOSE("{variable}: [", var_id, "] of parent [", module_id, "] no matching variable found in module.");
        return nullptr;
    }
    else
    {
        if ((*var_iter)->get_type() == "in")
        {
            LOG_VERBOSE("{variable}: [", (*var_iter)->get_id(), "] of parent [", module_id,
                        "] found injection location at[", (*var_iter)->get_inj_loc(), "] - skip variable is input!");
            return nullptr;
        }
        LOG_VERBOSE("{variable}: [", (*var_iter)->get_id(), "] of parent [", module_id,
                    "] found injection location at[", (*var_iter)->get_inj_loc(), "]");
        (*var_iter)->add_inj_loc(LOCATABLE_INITIALIZER(assignee->getExprLoc(), ctx.getSourceManager()));

        return var_iter->get();
    }
}

void VrtlmodCore::build_xml()
{
    FileLocator::foreach_relevant_file(
        [&](const auto &it)
        {
            pugi::xml_node file_node = ctx_->xml_files_node_->append_child("file");
            file_node.append_attribute("id") = util::concat("f", std::to_string(it.first)).c_str();
            file_node.append_attribute("path") = it.second.string().c_str();
        });

    std::string top_name = get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202

#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    auto outfile = out_dir_path_ / top_name;
    outfile += "-vrtlmod.xml";

    LOG_INFO("Writing VRTL Analysis XML: ", outfile.string());
    ctx_->xml_doc_->save_file(outfile.string().c_str());

    struct ListWalker : public pugi::xml_tree_walker
    {
        virtual bool for_each(pugi::xml_node &node) override
        {
            std::string name = node.name();
            LOG_VERBOSE("node: ", name);
            if (name == "")
            {
                return false;
            }
            else if ((name == "var") || (name == "out") || (name == "inout"))
            {
                std::string parent = node.parent().name();
                if (parent == "module")
                {
                    types::Module m{ node.parent() };

                    const types::Module *unique_module{ nullptr };
                    auto func = [&m, &unique_module](const types::Module &it) -> bool
                    {
                        if (m == it)
                        {
                            unique_module = &it;
                            return false;
                        }
                        return true;
                    };
                    core_.foreach_module(func);

                    if (unique_module != nullptr)
                        core_.add_injectable_target(std::make_shared<types::Target>(node, *unique_module));
                }
            }
            return true;
        }
        VrtlmodCore &core_;
        ListWalker(VrtlmodCore &core) : core_(core) {}
    } walker{ *this };
    ctx_->xml_doc_->traverse(walker);
}

int VrtlmodCore::build_api(void)
{
    return gen_->build_api();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class Filter
/// \brief Base class for filters generating injcetion targets for injectable targets
struct Filter
{
    virtual std::set<const types::Target *> apply(const VrtlmodCore *core);

    std::set<const types::Target *> targets_{};
    Filter(void) {}
    virtual ~Filter(void) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class WhiteListFilter
/// \brief Filters injcetion targets for injectable targets by whitlist XML
/// \details To use pass a VRTL analyze/elaborate XML file (VrtlmodCore::build_xml), with optionally deleted nodes
struct WhiteListFilter : public Filter, public pugi::xml_tree_walker
{
    bool for_each(pugi::xml_node &node) override;
    std::set<const types::Target *> apply(const VrtlmodCore *core) override;
    const VrtlmodCore *core_{ nullptr };
    fs::path fpath_;
    WhiteListFilter(fs::path fpath) : fpath_(fpath) {}
    virtual ~WhiteListFilter(void) {}
};

int VrtlmodCore::initialize_injection_targets(std::string file)
{
    std::unique_ptr<Filter> filter;
    // check for valid file
    auto fpath = fs::path(file);
    if (file == "" || util::check_file(fpath)) // no filter application
    {
        if (file != "")
            LOG_WARNING("Passed Filter is not a valid file path. Ignoring Filters.");
        filter = std::make_unique<Filter>();
    }
    else
    {
        // we've got a whitelist file to filter our injectables with:
        filter = std::make_unique<WhiteListFilter>(fpath);
    }

    apply_target_filter(std::move(filter));

    LOG_INFO("Remaining Injection Targets:");
    for (const auto &t : ctx_->toinj_targets_)
    {
        LOG_INFO("\\->", t->_self());
    }

    std::set<const types::Target *> removed{};
    for (const auto &injectable : ctx_->injectable_targets_)
    {
        bool rem = true;
        for (const auto &injecting : ctx_->toinj_targets_)
        {
            if (injecting == injectable)
            {
                rem = false;
                break;
            }
        }
        if (rem)
            removed.insert(injectable.get());
    }

    if (removed.size() > 0)
    {
        LOG_WARNING("Removed Injectable Targets:");
        for (const auto &t : removed)
        {
            LOG_INFO("\\->", t->_self());
        }
    }

    return 0;
}

void VrtlmodCore::apply_target_filter(std::unique_ptr<Filter> filter) const
{
    auto targets = filter->apply(this);
    auto func = [&](const types::Target &it)
    {
        for (const auto &v : targets)
        {
            types::Variable _v(static_cast<pugi::xml_node>(*v));
            if (_v == it)
            {
                add_injection_target(it);
                break; // target found can leave current it-eration
            }
        }
        return true; // continue loop thorugh injectables
    };
    foreach_injectable(func);
}

void VrtlmodCore::add_signal(std::shared_ptr<types::Target> sig) const
{
    ctx_->signals_.insert(sig);
}

void VrtlmodCore::set_top_cell(const pugi::xml_node &node) const
{
    ctx_->top_cell_ = std::make_unique<types::Cell>(node);
}

std::set<const types::Target *> Filter::apply(const VrtlmodCore *core)
{
    LOG_INFO("No Filter found. Making all injectables to injection targets.");
    auto func = [&](const types::Target &t)
    {
        if (((t.get_name() == "out") || (t.get_name() == "inout")) && core->is_systemc())
        {
            const types::Module *top_module = core->get_module_from_cell(core->get_top_cell());
            const types::Module &parent = t.get_parent();
            if (parent == *top_module)
            {
                LOG_WARNING("Skip injection for SystemC Top-level Outputs: ", t._self());
                return true; // FIXME: skip output ports of top-level systemc modules.
                             // Currently no supported for injections
            }
        }
        targets_.insert(&t);
        LOG_VERBOSE(">> adding [", t._self(), "] to injection targets.");
        return true;
    };
    core->foreach_injectable(func);

    return targets_;
}

bool WhiteListFilter::for_each(pugi::xml_node &node)
{
    std::string name = node.name();
    LOG_VERBOSE("> whitelist node: ", name);
    if (name == "" || core_ == nullptr)
    {
        return false;
    }
    else if ((name == "var") || (name == "out") || (name == "inout"))
    {
        std::string parent = node.parent().name();

        if (parent == "module")
        {
            if (((name == "out") || (name == "inout")) && core_->is_systemc())
            {
                const types::Module *top_module = core_->get_module_from_cell(core_->get_top_cell());
                types::Module parent(node.parent());
                if (parent == *top_module)
                {
                    types::Target t(node, parent);
                    LOG_WARNING("Skip injection for SystemC Top-level Outputs: ", t._self());
                    return true; // FIXME: skip output ports of top-level systemc modules.
                                 // Currently no supported for injections
                }
            }

            types::Variable v(node);
            auto func = [&](const types::Target &t)
            {
                if (v == t)
                {
                    LOG_VERBOSE(">> adding [", t._self(), "] to injection targets.");
                    targets_.insert(&t);
                    return false;
                }
                return true;
            };
            core_->foreach_injectable(func);
        }
    }
    return true;
}

std::set<const types::Target *> WhiteListFilter::apply(const VrtlmodCore *core)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(fpath_.c_str());
    if (!result)
    {
        LOG_ERROR("XML [", fpath_.string(), "] parsed with errors, vrtlmod version: [",
                  doc.child("vrtlmod_xml").attribute("version").value(), "]\n",
                  "\tError description: ", result.description(), "\n",
                  "\tError offset: ", std::to_string(result.offset), " (error at [...", fpath_.string(), ":",
                  std::to_string(result.offset));
    }
    else
    {
        LOG_INFO("Filter found. Filtering injectables by whitelisting.");
        core_ = core;
        doc.traverse(*this);
    }

    return targets_;
}

void VrtlmodCore::foreach_signal(const std::function<bool(const types::Target &t)> &func) const
{
    for (const auto &it : ctx_->signals_)
    {
        if (!func(*it))
            break;
    }
}

void VrtlmodCore::foreach_injection_target(const std::function<bool(const types::Target &t)> &func) const
{
    for (const auto &it : ctx_->toinj_targets_)
    {
        if (!func(*it))
            break;
    }
}

void VrtlmodCore::foreach_injectable(const std::function<bool(const types::Target &t)> &func) const
{
    for (const auto &it : ctx_->injectable_targets_)
    {
        if (!func(*it))
            break;
    }
}

void VrtlmodCore::foreach_module(const std::function<bool(const types::Module &)> &func) const
{
    for (const auto &m : ctx_->modules_)
    {
        if (!func(*m))
            break;
    }
}

void VrtlmodCore::foreach_cell(const std::function<bool(const types::Cell &)> &func) const
{
    if (!func(get_top_cell()))
        return;

    for (const auto &m : ctx_->modules_)
        for (const auto &c : m->cells_)
        {
            if (!func(*c))
                break;
        }
}

const types::Module *VrtlmodCore::get_module_from_cell(const types::Cell &c) const
{
    std::set<std::unique_ptr<types::Module>>::iterator mod_iter;
    auto cell_type = c.get_type();
    mod_iter = std::find_if(ctx_->modules_.begin(), ctx_->modules_.end(),
                            [cell_type](const auto &it) { return cell_type == it->get_id(); });
    if (mod_iter != ctx_->modules_.end())
    {
        return (*mod_iter).get();
    }
    return nullptr;
}

std::set<std::shared_ptr<types::Target>> &VrtlmodCore::get_signals(void) const
{
    return (ctx_->signals_);
}

std::set<std::shared_ptr<types::Target>> &VrtlmodCore::get_inj_targets(void) const
{
    return (ctx_->toinj_targets_);
}

std::set<std::shared_ptr<types::Target>> &VrtlmodCore::get_injectable_targets(void) const
{
    return (ctx_->injectable_targets_);
}

void VrtlmodCore::add_parsed_file(fs::path fpath) const
{
    ctx_->parsed_files_.insert(fpath);
}

void VrtlmodCore::add_injectable_target(std::shared_ptr<types::Target> t) const
{
    ctx_->injectable_targets_.insert(t);
}

void VrtlmodCore::add_injection_target(std::shared_ptr<types::Target> t) const
{
    ctx_->toinj_targets_.insert(t);
}

void VrtlmodCore::add_injection_target(const types::Target &t) const
{
    for (auto &tptr : get_injectable_targets())
    {
        if (*tptr == t)
        {
            add_injection_target(tptr);
            break;
        }
    }
}

} // namespace vrtlmod
