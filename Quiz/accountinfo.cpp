#include "accountinfo.h"

AccountInfo::AccountInfo(QObject *parent)
    : QObject(parent), _loggedin(false)
{
    // Establish username from login
    char* user = nullptr;
#ifdef WINVER
    user = std::getenv("USERNAME");
#else
    user = std::getenv("USER");
#endif
    if (user != nullptr)
        _user = user;

    client = new Client(_user);
}

