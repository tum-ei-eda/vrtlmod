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
/// @file types.hpp
/// @date Created on Mon Feb 28 15:56:11 2022
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_CORE_TYPES_HPP__
#define __VRTLMOD_CORE_TYPES_HPP__

#include <set>
#include <string>
#include <sstream>
#include <algorithm>

#include <pugixml.hpp>

#include <boost/lexical_cast.hpp>

#include "vrtlmod/core/filecontext.hpp"

#include "vrtlmod/util/logging.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{

class VapiGenerator;
class VrtlmodCore;

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core types
namespace types
{

struct NNode : public pugi::xml_node
{
    virtual std::string get_id(void) const { return pugi::xml_node::attribute("id").value(); }
    virtual std::string get_name(void) const { return pugi::xml_node::name(); }

    virtual void set_id(const std::string &id)
    {
        pugi::xml_node::remove_attribute("id");
        pugi::xml_node::append_attribute("id") = id.c_str();
    }
    NNode(const pugi::xml_node &xml_node) : pugi::xml_node(xml_node) {}
    virtual ~NNode() {}
};

#define LOCATABLE_GET_FILENAME_FROM_CLANG(LOC, SM) SM.getFileEntryForID(SM.getFileID(LOC))->getName()
#define LOCATABLE_GET_LINE_FROM_CLANG(LOC, SM) SM.getLineNumber(SM.getFileID(LOC), SM.getFileOffset(LOC))
#define LOCATABLE_GET_COL_FROM_CLANG(LOC, SM) SM.getColumnNumber(SM.getFileID(LOC), SM.getFileOffset(LOC))
#define LOCATABLE_INITIALIZER(LOC, SM)                                                  \
    LOCATABLE_GET_FILENAME_FROM_CLANG(LOC, SM), LOCATABLE_GET_LINE_FROM_CLANG(LOC, SM), \
        LOCATABLE_GET_COL_FROM_CLANG(LOC, SM)

struct Locatable : public NNode
{
    std::unique_ptr<FileLocator> decl_loc_{};              ///< found declaration location
    std::vector<std::unique_ptr<FileLocator>> sea_locs_{}; ///< found sequential assignment locations

    virtual std::string get_decl_loc(void) const { return pugi::xml_node::attribute("decl_loc").value(); }
    virtual std::string get_inj_loc(void) const { return pugi::xml_node::attribute("inj_loc").value(); }

    virtual void set_decl_loc(const std::string &id, unsigned line, unsigned column)
    {
        pugi::xml_node::remove_attribute("decl_loc");
        pugi::xml_node::append_attribute("decl_loc") =
            util::concat(id, ":l", std::to_string(line), ":c", std::to_string(column)).c_str();
    }

    virtual void set_inj_loc(const std::string &id, unsigned line, unsigned column)
    {
        pugi::xml_node::remove_attribute("inj_loc");
        pugi::xml_node::append_attribute("inj_loc") =
            util::concat(id, ":l", std::to_string(line), ":c", std::to_string(column)).c_str();
    }

    virtual void append_inj_loc(const std::string &id, unsigned line, unsigned column)
    {
        std::string old = get_inj_loc();
        if (old != "")
        {
            util::strhelp::replaceAll(old, "]", "");
            util::strhelp::replaceAll(old, "[", "");
            old += ", ";
        }
        pugi::xml_node::remove_attribute("inj_loc");
        pugi::xml_node::append_attribute("inj_loc") =
            util::concat("[", old, id, ":l", std::to_string(line), ":c", std::to_string(column), "]").c_str();
    }

    void add_decl_loc(const std::string &file, int line, int column)
    {
        auto loc = std::make_unique<FileLocator>(file, line, column);
        LOG_VERBOSE("decl file: [", loc->get_id(), "]", file);
        set_decl_loc(loc->get_id(), loc->get_line(), loc->get_column());
        decl_loc_ = std::move(loc);
    }
    void add_decl_loc(const llvm::StringRef &file, int line, int column)
    {
        add_decl_loc(std::string(file), line, column);
    }
    void add_inj_loc(const std::string &file, int line, int column, bool indirect = false)
    {
        auto loc = std::make_unique<FileLocator>(file, line, column);
        LOG_VERBOSE("inj file: [", loc->get_id(), "]", file);
        append_inj_loc(loc->get_id(), loc->get_line(), loc->get_column());
        sea_locs_.push_back(std::move(loc));
    }
    void add_inj_loc(const llvm::StringRef &file, int line, int column, bool indirect = false)
    {
        add_inj_loc(std::string(file), line, column, indirect);
    }
    Locatable(const pugi::xml_node &xml_node) : NNode(xml_node) {}
    virtual ~Locatable() {}
};

struct Cell : public Locatable //, public Ancestral<Cell>
{
    virtual std::string get_type(void) const { return pugi::xml_node::attribute("type").value(); }
    virtual std::string get_hierarchy(void) const { return pugi::xml_node::attribute("vHier").value(); }

    Cell(const pugi::xml_node &xml_node) : Locatable(xml_node) //, Ancestral<Cell>()
    {
        std::string name = get_name();
        if (name != "cell")
        {
            LOG_ERROR("Cell `<", name, "> ", NNode::get_id(), "[", get_type(), "]` is not a <cell>!");
        }
    }
    Cell(const Cell &) = delete;
    Cell(const Cell &&) = delete;
};

struct Variable;

struct Module final : public Locatable
{
    std::set<std::string> symboltable_instances_;

    std::set<std::unique_ptr<types::Variable>> variables_;
    std::set<std::unique_ptr<types::Cell>> cells_;

    void add_variable(std::unique_ptr<types::Variable> var) { variables_.insert(std::move(var)); }
    void add_cell(std::unique_ptr<types::Cell> cell) { cells_.insert(std::move(cell)); }

    void add_instance(std::string instance) { symboltable_instances_.insert(instance); }

    Module(const pugi::xml_node &xml_node) : Locatable(xml_node)
    {
        std::string name = get_name();
        if (name != "module")
        {
            LOG_ERROR("Module `<", name, "> ", NNode::get_id(), "` is not a <module>!");
        }
    }

    virtual bool operator==(const Module &rhs) const
    {
        auto d = get_decl_loc();
        auto _d = rhs.get_decl_loc();
        auto i = get_id();
        auto _i = rhs.get_id();
        bool ret = (d == _d) && (i == _i);
        return ret;
    }

    Module(const Module &) = delete;
};

struct Variable : public Locatable //, public Ancestral<Module>
{
    enum class Type
    {
        IN,
        OUT,
        INOUT,
        VAR
    };

    virtual std::string get_class(void) const { return get_inj_loc() != "" ? "undef" : "reg"; }
    virtual int get_bits(void) const { return boost::lexical_cast<int>(pugi::xml_node::attribute("bits").value()); }
    virtual std::string get_bases(void) const { return pugi::xml_node::attribute("bases").value(); }
    virtual std::string get_dimensions(void) const { return pugi::xml_node::attribute("dim").value(); }
    virtual std::string get_cxx_type(void) const { return pugi::xml_node::attribute("cxx_type").value(); }
    virtual std::string get_type(void) const { return get_name(); }

    Variable(const pugi::xml_node &xml_node) : Locatable(xml_node) //, Ancestral<Module>(parent)
    {
        std::string name = get_name();
        if (name != "in" && name != "out" && name != "var" && name != "inout")
        {
            LOG_ERROR("Element `<", name, "> ", NNode::get_id(), "[", get_cxx_type(), "]` is not a <in|out|var>!");
        }
    }
    virtual ~Variable() {}

    virtual bool operator==(const Variable &rhs) const
    {
        bool ret = (this->get_decl_loc() == rhs.get_decl_loc()) && (this->get_id() == rhs.get_id()) &&
                   (this->get_class() == rhs.get_class()) && (this->get_bits() == rhs.get_bits()) &&
                   (this->get_bases() == rhs.get_bases()) && (this->get_dimensions() == rhs.get_dimensions()) &&
                   (this->get_cxx_type() == rhs.get_cxx_type()) && (this->get_type() == rhs.get_type());
        return ret;
    }
};

////////////////////////////////////////////////////////////////////////////////
/// @class Target
/// @brief Corresponds to RegPicker-Xml element information
class Target : public Variable
{
    typedef enum SIG_CLASS
    {
        REG = 1,
        CONST = 2,
        WIRE = 3,
        UNDEF = 0
    } signalClass_t;

  protected:
    bool found_decl_{ false };
    bool found_assignment_{ false };

    const Module &parent_;

    std::pair<std::vector<std::string>, std::vector<int>> get_cxx_dimensions(void) const;
    std::vector<int> get_dimension_lengths(void) const;

  public:
    bool decl_rewritten_{ false };
    ///////////////////////////////////////////////////////////////////////
    /// \brief One-dimensional, i.e., element, number of bits
    int get_one_dim_bits(void) const { return get_dimension_lengths().back(); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Data types for unpacked types
    std::vector<std::string> get_cxx_dimension_types(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Dimensions of multi-dimensional types, e.g., CData[1][5]
    std::vector<int> get_cxx_dimension_lengths(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get parent (module declaring the signal/target)
    const Module &get_parent() const { return parent_; }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns the targets hierarchy
    std::string get_hierarchy(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns the targets undotted hierarchy ("__DOT__"s instead of "."s)
    std::string get_hierarchyDedotted(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns zero dim Most and Least significant bit
    std::pair<int, int> get_element_msb_lsb_pair(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Return the elements mask
    unsigned long get_element_mask(std::initializer_list<size_t> subscripts) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns the number of fault injection positions
    unsigned int get_seq_assignment_count(void) const { return sea_locs_.size(); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Self representation of this target
    std::string _self(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    Target(const pugi::xml_node &xml_node, const Module &parent) : types::Variable(xml_node), parent_(parent){};
    ///////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    virtual ~Target(void) {}

    ///////////////////////////////////////////////////////////////////////
    /// \brief Overloaded operator for stream access of class (reference)
    friend std::ostream &operator<<(std::ostream &os, const Target &obj)
    {
        os << obj._self();
        return os;
    }

    bool is_declared_here(const clang::FieldDecl *decl) const;
    bool is_assigned_here(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent = nullptr) const;
    bool has_found_decl(void) { return found_decl_; }
    bool has_found_assignment(void) { return found_assignment_; }
    bool has_rewritten_decl(void) { return decl_rewritten_; }
    bool check_declared_here(const clang::FieldDecl *decl);
    bool check_assigned_here(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent = nullptr);

    // oops
    virtual bool operator==(const Target &rhs)
    {
        // return (this->get_index() == rhs.get_index()); // TODO: could be problematic...
        bool ret = (this->get_parent() == rhs.get_parent()) && Variable::operator==(rhs);
        return ret;
    }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    Target(const pugi::xml_node &xml_node, const pugi::xml_node &parent) = delete;
    Target(const Target &) = delete;
};

} // namespace types
} // namespace vrtlmod

#endif // __VRTLMOD_CORE_TYPES_HPP__
