#ifndef ADMIN_ACTION_H
#define ADMIN_ACTION_H

#include <string>

// A snapshot of an item-related admin change. The controller will use this
// information later to reverse the change in the library database.
struct AdminAction {
    enum class Kind {
        ItemAdded,
        ItemDeleted
    };

    Kind kind;
    std::string itemId;
    std::string itemType;
    std::string title;
    int publicationYear;
    std::string creator; // Used for a Book's author; empty for other item types.
};

#endif
