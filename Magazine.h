#ifndef MAGAZINE_H
#define MAGAZINE_H

#include "LibraryItem.h"

class Magazine : public LibraryItem {
public:
    Magazine(const std::string& id,
             const std::string& title,
             int publicationYear)
        : LibraryItem(id, title, publicationYear) {}

    std::string itemType() const override { return "Magazine"; }
};

#endif
