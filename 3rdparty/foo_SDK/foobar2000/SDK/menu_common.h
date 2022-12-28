#pragma once
#include "image.h"

class menu_flags {
public:
    enum {
        disabled = 1 << 0,
        checked = 1 << 1,
        radiochecked = 1 << 2,
        defaulthidden = 1 << 3,
        disclosure = 1 << 4, // mobile platform specific, indicates whether to show a disclosure mark next to a command
    };
};

//! Type referring to a combination of menu_flags::* values
typedef uint32_t menu_flags_t;

typedef fb2k::image menu_glyph;

//! \since 2.0
//! Menu manager's tree item object; see mainmenu_manager and contextmenu_manager.
class menu_tree_item : public service_base {
    FB2K_MAKE_SERVICE_INTERFACE(menu_tree_item, service_base);
public:
    enum type_t {
        itemSeparator = 0,
        itemCommand,
        itemSubmenu
    };


    virtual type_t type() = 0;
    virtual size_t childCount() = 0;
    virtual menu_tree_item::ptr childAt(size_t at) = 0;
    virtual uint32_t commandID() = 0;
    virtual const char* name() = 0;
    virtual menu_flags_t flags() = 0; // menu_flags::*
    virtual void execute(service_ptr ctx) = 0;
    virtual bool get_description(pfc::string_base& strOut) = 0;
    //! Refreshes the item's state. Returns true if either of the following properties has changed: flags, name, glyph, description. Currently does not allow the item's children to change.
    virtual bool refreshState() { return false; }

    //! Command GUID for registering yourself for menu refresh notifications. May be null GUID if this item has no valid command GUID.
    virtual GUID commandGuid() { return pfc::guid_null; }
    //! Sub-command GUID for dynamically created items. Typically null GUID as simple items do not have sub command GUIDs.
    virtual GUID subCommandGuid() { return pfc::guid_null; }

    virtual bool get_icon(fb2k::imageLocation_t& outLoc) { return false; }

    bool isSeparator() { return type() == itemSeparator; }
    bool isCommand() { return type() == itemCommand; }
    bool isSubmenu() { return type() == itemSubmenu; }
    bool isDisclosure() { return (flags() & menu_flags::disclosure) != 0; }
};


typedef menu_tree_item mainmenu_tree_item;

