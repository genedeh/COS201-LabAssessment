#ifndef BOOK_H
#define BOOK_H

#include "LibraryItem.h"

class Book : public LibraryItem {
private:
    std::string author;

public:
    Book(const std::string& id,
         const std::string& title,
         int publicationYear,
         const std::string& author)
        : LibraryItem(id, title, publicationYear), author(author) {}

    const std::string& getAuthor() const { return author; }
    std::string itemType() const override { return "Book"; }
};

#endif
