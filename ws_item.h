#pragma once

/*
 * A WsItem is a record of a single item in the workspace.
 * It has a WsPath recording the location in the workspace.
 */

#include "ws_path.h"
#include <boost/optional.hpp>
#include <mongocxx/cursor.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/json.hpp>

class WsItem
{
public:
    WsItem();
//    WsItem(bsoncxx::document::view v, const std::string &type_override = "");
//    WsItem(bsoncxx::document::value v, const std::string &type_override = "");
    WsItem(bsoncxx::document::view_or_value v, const std::string &type_override = "");
    WsItem(WsItem &&other);
    ~WsItem();

    friend std::ostream &operator<<(std::ostream &out, WsItem &item);
    friend class WsItemIterator;

    void fill_data();

    /* Accessors defined by the P3 workspace specification. */
    std::string name() const { return view_["name"].get_utf8().value.to_string(); }
    std::string type() const { return type_override_.empty() ? view_["type"].get_utf8().value.to_string() : type_override_; }
    std::string path() const { return type_override_ == "workspace" ? "" : view_["path"].get_utf8().value.to_string(); }
    std::string creation_time() const { return view_["creation_date"].get_utf8().value.to_string(); }
    std::string object_id() const { return view_["uuid"].get_utf8().value.to_string(); }
    std::string object_owner() const { return view_["owner"].get_utf8().value.to_string(); }
    std::string shocknode() const { return view_["shocknode"].get_utf8().value.to_string(); }
    bool shock() const {
	auto v = view_["shock"];
	switch (v.type()) {
	case bsoncxx::type::k_bool:
	    return v.get_bool();
	case bsoncxx::type::k_int32:
	    return v.get_int32();
	case bsoncxx::type::k_int64:
	    return v.get_int64();
	case bsoncxx::type::k_undefined:
	    return false;
	case bsoncxx::type::k_utf8:
	    return !v.get_utf8().value.to_string().empty();
	default:
	    return false;
	}
    }	    

private:
    WsPath path_;
    bsoncxx::document::view_or_value view_or_value_;
    bsoncxx::document::view view_;
    std::string type_override_;

    std::string data_;
};


class WsItemSet;

class WsItemIterator
{
public:
    WsItemIterator(WsItemSet &item_set);
    WsItemIterator(WsItemSet &item_set, int);
    WsItemIterator(WsItemIterator &&);
    WsItemIterator(WsItemIterator &);

    WsItemIterator & operator++();
    WsItem operator*();

    bool operator==(WsItemIterator const &other) { return this->citerator_ == other.citerator_; }
    bool operator!=(WsItemIterator const &other) { return this->citerator_ != other.citerator_; }
//private:
    WsItemSet &item_set_;
    mongocxx::cursor::iterator citerator_;
};

class WsItemSet
{
public:
    WsItemSet(mongocxx::cursor cursor, const std::string &type_override = "");

    WsItemIterator begin();
    WsItemIterator end();

    mongocxx::cursor cursor_;
    WsItemIterator end_iter_;
    std::string type_override_;
};

