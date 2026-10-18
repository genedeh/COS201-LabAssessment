#ifndef LIBRARY_ITEM_H
#define LIBRARY_ITEM_H

#include <string>

// Abstract base class for every kind of item stored by the library.
class LibraryItem {
private:
    std::string id;
    std::string title;
    int publicationYear;
    bool available;

public:
    LibraryItem(const std::string& id,
                const std::string& title,
                int publicationYear)
        : id(id), title(title), publicationYear(publicationYear), available(true) {}

    // A virtual destructor makes deleting child objects through a LibraryItem
    // pointer safe. The = 0 makes this class abstract.
    virtual ~LibraryItem() = default;

    const std::string& getId() const { return id; }
    const std::string& getTitle() const { return title; }
    int getPublicationYear() const { return publicationYear; }
    bool isAvailable() const { return available; }

    void setAvailable(bool value) { available = value; }

    // Every concrete child must say what type of item it represents.
    virtual std::string itemType() const = 0;
};

#endif
