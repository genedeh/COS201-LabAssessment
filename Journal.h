#ifndef JOURNAL_H
#define JOURNAL_H

#include "LibraryItem.h"

class Journal : public LibraryItem {
public:
    Journal(const std::string& id,
            const std::string& title,
            int publicationYear)
        : LibraryItem(id, title, publicationYear) {}

    std::string itemType() const override { return "Journal"; }
};

#endif
