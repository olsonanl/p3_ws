#include "ws_item.h"
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/json.hpp>

WsItemSet::WsItemSet(mongocxx::cursor cursor, const std::string &type_override) :
    cursor_(std::move(cursor)),
    end_iter_(*this, 0),
    type_override_(type_override)
{
}

WsItemIterator WsItemSet::begin()
{
    return WsItemIterator(*this);
}

WsItemIterator WsItemSet::end()
{
    return end_iter_;
}

WsItem::WsItem()
{
    std::cerr << "wsitem defualt\n";
}

WsItem::~WsItem()
{
    std::cerr << "wsitem destruct " << view_or_value_.is_owning() << "\n";
}


WsItem::WsItem(bsoncxx::document::view_or_value vv, const std::string &type_override) :
    view_or_value_(vv),
    view_(vv.view()),
    type_override_(type_override)
{
}

WsItem::WsItem(WsItem &&other) :
    path_(other.path_),
    view_or_value_(std::move(other.view_or_value_)),
    view_(view_or_value_.view()),
    type_override_(other.type_override_)
{
    std::cerr << "move construct\n";
}

std::ostream &operator<<(std::ostream &out, WsItem &item)
{
    out << "Item: " << bsoncxx::to_json(item.view_) << "\n";
    return out;
}

WsItemIterator::WsItemIterator(WsItemSet &item_set) :
    item_set_(item_set),
    citerator_(item_set_.cursor_.begin())
{
    std::cerr << "init begin iter " << this << "\n";
}

WsItemIterator::WsItemIterator(WsItemSet &item_set, int x) :
    item_set_(item_set),
    citerator_(item_set_.cursor_.end())
{
    std::cerr << "init end iter " << this << "\n";
}

WsItemIterator::WsItemIterator(WsItemIterator &&other) :
    item_set_(other.item_set_),
    citerator_(other.citerator_)
{
    std::cerr << "move construct " << this << " from " << &other << "\n";
}

WsItemIterator::WsItemIterator(WsItemIterator &other) :
    item_set_(other.item_set_),
    citerator_(other.citerator_)
{
    std::cerr << "copy construct " << this << " from " << &other << "\n";
}

WsItemIterator &WsItemIterator::operator++()
{
    ++citerator_;
    return *this;
}

WsItem WsItemIterator::operator*()
{
//    const bsoncxx::document::view & data = *citerator_;
//    WsItem i(data);
//    return i;
    return WsItem(*citerator_, item_set_.type_override_);
}
    
