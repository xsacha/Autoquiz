#include "accountinfo.h"
#include <QMessageBox>

AccountInfo::AccountInfo(QObject *parent)
    : QObject(parent)
{
    // Establish username from login
    char* user = nullptr;
#ifdef WINVER
    user = std::getenv("DISPLAYNAME"); // LAST, first
    if (user != nullptr) {
        QStringList parts = QString(user).toLower().split(", ");
        user = std::getenv("USERNAME"); // ffllln
        if (user != nullptr) {
            if (parts[1][0] != user[0])
            {
                // They do not match. Suspect tampering.
                if (QMessageBox::information(NULL, "Login mismatch", "Login information does not match.\n Either this is not a properly configured EQ machine, or the login has been tampered with.", QMessageBox::Ok))
                    exit(0);
            }
        }
        for(int i = 0; i < parts.length(); i++)
            parts[i][0] = parts[i][0].toUpper();
        if (parts.length() > 1)
            _user = parts.last() + " " + parts.first();
        else
            _user = parts.first();
    }
#else
    // We don't have a display name for *nix. Just use the username.
    user = std::getenv("USER");
    if (user != nullptr)
        _user = user;
#endif
}

