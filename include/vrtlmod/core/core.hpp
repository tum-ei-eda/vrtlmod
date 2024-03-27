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
/// @file core.hpp
/// @date Created on Mon Feb 28 14:31:40 2022
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_CORE_CORE_HPP__
#define __VRTLMOD_CORE_CORE_HPP__

#include <memory>
#include <vector>
#include <set>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace pugi
{
class xml_node;
class xml_document;
class xml_tree_walker;
} // namespace pugi

namespace clang
{
class MemberExpr;
class FieldDecl;
class NamedDecl;

class CXXRecordDecl;
class ASTContext;
class Rewriter;
} // namespace clang

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
struct Filter;
namespace types
{
class Target;
class Module;
class Variable;
class Cell;
} // namespace types

namespace passes
{
class AnalyzePass;
class ElaboratePass;
class SignalDeclRewriter;
class InjectionRewriter;
} // namespace passes
namespace vapi
{
class VapiGenerator;
} // namespace vapi

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @class VrtlmodCore
class VrtlmodCore
{
    ///>>> TODO: Remove?
    friend passes::AnalyzePass;
    friend passes::ElaboratePass;
    friend passes::InjectionRewriter;
    friend passes::SignalDeclRewriter;
    friend vapi::VapiGenerator; ///<<<

  public:
    struct Context
    {
        std::unique_ptr<types::Cell> top_cell_;                       ///< TOP cell
        std::set<std::shared_ptr<types::Target>> signals_;            ///< Vector containing all parsed (xml) signals
        std::set<std::shared_ptr<types::Target>> injectable_targets_; ///< Vector containing all injectable targets
        std::set<std::shared_ptr<types::Target>>
            toinj_targets_;               ///< Vector containing all from injection targets (filtered signals)
        std::set<fs::path> parsed_files_; ///< parsed files
        std::set<std::unique_ptr<types::Module>> modules_;

        std::unique_ptr<pugi::xml_document> xml_doc_;
        std::unique_ptr<pugi::xml_node> xml_root_node_;
        std::unique_ptr<pugi::xml_node> xml_netlist_node_;
        std::unique_ptr<pugi::xml_node> xml_modules_node_;
        std::unique_ptr<pugi::xml_node> xml_files_node_;
    };
    std::unique_ptr<vapi::VapiGenerator> gen_{};

  protected:
    std::unique_ptr<Context> ctx_;
    fs::path out_dir_path_; ///< Specified path to output directory
    bool systemc_;          ///< vrtl input is systemc

  public: // public GETTERS and SETTERS
    const Context &get_ctx() const { return *ctx_; }
    std::set<fs::path> get_parsed_files(void) const { return ctx_->parsed_files_; }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns output directory
    fs::path get_output_dir(void) const { return (out_dir_path_); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns true if the API generator was configured for SystemC VRTL
    bool is_systemc(void) const { return (systemc_); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get extracted targets
    const types::Cell &get_top_cell(void) const { return *(ctx_->top_cell_); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief apply function to/with each signal
    void foreach_signal(const std::function<bool(const types::Target &t)> &func) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief apply function to/with each injection target
    void foreach_injection_target(const std::function<bool(const types::Target &t)> &func) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief apply function to/with each injectable target
    void foreach_injectable(const std::function<bool(const types::Target &t)> &func) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief apply function to/with each module
    void foreach_module(const std::function<bool(const types::Module &)> &func) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief apply function to/with each cell
    void foreach_cell(const std::function<bool(const types::Cell &)> &func) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Return the unique module (type of cell) for a given cell
    const types::Module *get_module_from_cell(const types::Cell &c) const;

  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief Prepare sources according to the API
    /// \param files file paths to files ( will sort out *.cpp/*.cc and ignore rest)
    /// \return A new vector of sources file paths (path is generated on call)
    /// \details Copies (or overwrites - corresponding to cmd line options) sources
    std::vector<std::string> prepare_sources(const std::vector<std::string> &files, bool overwrite);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Prepare headers according to the API
    /// \param files file paths to files ( will sort out *.h/*.hpp and ignore rest)
    /// \return A new vector of sources file paths (path is generated on call)
    /// \details Copies (or overwrites - corresponding to cmd line options) sources
    std::vector<std::string> prepare_headers(const std::vector<std::string> &files, bool overwrite);
    /// \brief Preprocess headers according to the API
    /// \param files file paths to files ( will sort out *.h/*.hpp and ignore rest)
    void preprocess_headers(const std::vector<std::string> &header_files);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Postprocess headers according to the API
    /// \param files file paths to files ( will sort out *.h/*.hpp and ignore rest)
    void postprocess_headers(const std::vector<std::string> &header_files);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get the header file name of VRTL top module
    std::string get_vrtltopheader_filename(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get the header file name of VRTL symbol table module
    std::string get_vrtltopsymsheader_filename(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Checks whether given Expr is a target in the Xml-Input
    /// \param pExpr Expression String
    /// \return -1 if not a target. >-1 as index in target list
    int is_expr_target(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent,
                       const clang::ASTContext &ctx) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Checks whether given Expr is a target in the Xml-Input
    /// \param target field declaration of target in question
    /// \return number of target instances matching the declaration
    int is_decl_target(const clang::FieldDecl *target) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns String containing the (external) declaration of target dictionary (once full decl)
    std::string getTDExternalDecl(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Return needed include strings
    std::string get_include_string() const { return "#include \"targetdictionary.hpp\"\n"; }

    VrtlmodCore(const char *out_dir_path, bool systemc);
    virtual ~VrtlmodCore(void);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Build VRTLFI XML from analysis and eleboration
    void build_xml(void);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Build VRTLFI API
    int build_api(void);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Print the TD to std::out
    void print_targetdictionary(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Applies the given xml file as a whitelist to injectable targets upodating to-inject internal target list
    int initialize_injection_targets(std::string file = "");

    std::string get_prefix(types::Cell const *c, std::string module_instance) const;
    std::string get_memberstr(types::Cell const *c, const types::Target &t, const std::string &prefix) const;
  protected: // only friends of VrtlmodCore or itself shall use these methods, bc. they return non-const reference to
    // context members or alter them
    std::vector<std::string> prepare_files(const std::vector<std::string> &files,
                                           const std::vector<std::string> &file_ext_matchers, bool overwrite = false);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get extracted targets
    std::set<std::shared_ptr<types::Target>> &get_signals(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get extracted targets
    std::set<std::shared_ptr<types::Target>> &get_inj_targets(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get extracted targets
    std::set<std::shared_ptr<types::Target>> &get_injectable_targets(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get a Target of target list by index
    /// \param idx Index
    /// \return Reference to Target with index idx
    types::Target &get_target_from_index(int idx) const;
    void add_parsed_file(fs::path fpath) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief add a injectable target
    void add_injectable_target(std::shared_ptr<types::Target> t) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief add a injection target
    void add_injection_target(std::shared_ptr<types::Target> t) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief add a injection target
    void add_injection_target(const types::Target &t) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief apply a passed target filter to injectables
    void apply_target_filter(std::unique_ptr<Filter> filter) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief add a signal
    void add_signal(std::shared_ptr<types::Target> sig) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get extracted targets
    void set_top_cell(const pugi::xml_node &node) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief register a cell as the top cell from AST, a top cell has no instance in the symboltable
    const types::Cell *set_top_cell(const clang::FieldDecl *cell, const clang::ASTContext &ctx) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief register a module (verilated module class) from AST
    const types::Module *add_module(const clang::CXXRecordDecl *module, const clang::ASTContext &ctx) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief register a module instance from AST
    const types::Module *add_module_instance(const clang::NamedDecl *instance_decl, const clang::CXXRecordDecl *module,
                                             const clang::ASTContext &ctx) const;
    const types::Module *add_module_instance(const clang::MemberExpr *instance, const clang::CXXRecordDecl *module,
                                             const clang::ASTContext &ctx) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief register a cell (reference to a verilated module class in a verilated module class) from AST
    const types::Cell *add_cell(const clang::FieldDecl *cell, const clang::ASTContext &ctx) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief register a variable (non-internal port or variable in a verilated module class) from AST
    const types::Variable *add_variable(const clang::FieldDecl *variable, const clang::ASTContext &ctx,
                                        const clang::Rewriter &rew) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief register a possible injection location with variable
    const types::Variable *add_injection_location(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent,
                                                  const clang::ASTContext &ctx) const;
};

} // namespace vrtlmod

#endif // __VRTLMOD_CORE_CORE_HPP__
