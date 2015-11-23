#include "accountinfo.h"

AccountInfo::AccountInfo(QObject *parent)
    : QObject(parent)
{
    // Establish username from login
    char* user = nullptr;
#ifdef WINVER
    //user = std::getenv("USERNAME"); // ffllln
    user = std::getenv("DISPLAYNAME"); // LAST, first
    if (user != nullptr) {
        QStringList parts = QString(user).toLower().split(", ");
        for(int i = 0; i < parts.length(); i++)
            parts[i][0] = parts[i][0].toUpper();
        if (parts.length() > 1)
            _user = parts.last() + " " + parts.first();
        else
            _user = parts.first();
    }
#else
    user = std::getenv("USER");
    if (user != nullptr)
        _user = user;
#endif
    if (user != nullptr)
        delete user;
}

