#include <algorithm>
#include <cstdlib>
#include <conio.h>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <regex>
#include <string>
#include <vector>
#include <windows.h>
#include "colors.h"

using namespace std;

const string USERS = "users.txt";
const string ACCOUNTS_LIST = "accounts.txt";
const string LOCKED_STATUS = "locked.txt";
const string XOR_KEY = "CyberTrexSecretKey";

struct User
{
    string userID;
    string username;
    size_t masterPassword;
};

struct Account
{
    string userID;
    string category;
    string username;
    string password;
};

struct LockoutStatus
{
    bool isLocked = false;
    time_t lockoutTime;
};

// Main management flow
void createUser(map<string, User>& usersMap);
void logInUser(const map<string, User>& usersMap, string& userInSession, LockoutStatus& lockoutStatus);
void passwordManagement(const map<string, User> usersMap, const string& username, string& userInSession);
void addAccountToUser(const string& userID, map<string, vector<Account>>& accountsMap);
void showAccounts(const string& userID, const map<string, vector<Account>>& accountsMap);
void updateAccount(const string& userID, map<string, vector<Account>>& accountsMap);
void deleteAccount(const string& userID, map<string, vector<Account>>& accountsMap);
void searchAccounts(const string& userID, const map<string, vector<Account>>& accountsMap);

// file loading functions
void loadUsers(map<string, User>& usersMap);
void loadAccounts(map<string, vector<Account>>& accountsMap);
void saveUserToFile(const string& filename, const map<string, User>& usersMap);
void saveAccounts(const map<string, vector<Account>>& accountsMap);

// Helper functions
int generate_unique_id();
size_t create_password();
bool comparePasswords(string passInput, size_t hashedPassword);
int get_int(const string& prompt);
string toLower(const string& str);
string xorEncrypt(const string& input, const string& key);
string xorDecrypt(const string& input, const string& key);

// UI
void displayStartScreen();

// Lockout status
LockoutStatus loadLockoutStatus();
void saveLockoutStatus(const LockoutStatus& status);


int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitle("CyberTrex Password Manager");
    map<string, User> usersMap;
    LockoutStatus lockoutStatus = loadLockoutStatus();
    string userInSession = "";

    loadUsers(usersMap);

    displayStartScreen();

    while (userInSession == "")
    {
        system("cls");

        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║       CyberTrex Password Manager         ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n';

        cout << BLUE << "┌──────────────────────────────────────────┐" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "1. Sign Up                               " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "2. Log In                                " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "3. Exit                                  " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "└──────────────────────────────────────────┘" << RESET << '\n';

        int choice = get_int(YELLOW "\n>> " RESET);

        switch (choice)
        {
            case 1:
                createUser(usersMap);
                break;
            case 2:
                logInUser(usersMap, userInSession, lockoutStatus);
                break;
            case 3:
                cout << GREEN << "\nThank you for using CyberTrex Password Manager. Goodbye!" << RESET << '\n';
                Sleep(2000);
                return 0;
            default:
                cout << RED << "\nInvalid option. Please try again." << RESET << '\n';
                Sleep(2000);
        }
    }
}

void createUser(map<string, User>& usersMap)
{
    User newUser;

    while (true)
    {
        cout << CYAN << "Enter username (press 0 to exit): ";
        getline(cin >> ws, newUser.username);
        if (newUser.username == "0")
        {
            system("CLS");
            return;
        }

        // Check if the username already exists
        bool usernameExists = false;
        for (const auto& pair : usersMap)
        {
            if (pair.second.username == newUser.username)
            {
                usernameExists = true;
                break;
            }
        }

        if (usernameExists)
        {
            cout << RED << "Username already exists. Please choose a different username." << RESET << endl;
        }
        else
        {
            break;
        }
    }

    newUser.masterPassword = create_password();
    newUser.userID = to_string(generate_unique_id());

    usersMap[newUser.userID] = newUser;

    saveUserToFile(USERS, usersMap);

    cout << GREEN << "User created successfully! Returning to main menu..." << RESET << '\n';
    cin.ignore();
    Sleep(3000);
    system("CLS");
}

void logInUser(const map<string, User>& usersMap, string& userInSession, LockoutStatus& lockoutStatus)
{
    int passwordAttempts = 0;

    if (lockoutStatus.isLocked)
    {
        time_t currentTime = time(0);
        if (difftime(currentTime, lockoutStatus.lockoutTime) < 900)
        {
            cout << BRED << "You have reached the maximum number of attempts. Please try again later.\n";
            Sleep(3000);
            system("CLS");
            return;
        }
        else
        {
            lockoutStatus.isLocked = false;
            saveLockoutStatus(lockoutStatus);
        }
    }

    while (passwordAttempts < 8)
    {
        cout << CYAN << "Username (enter 0 to exit): ";
        string username;
        getline(cin >> ws, username);

        if (username == "0")
        {
            cout << "Returning to main menu...";
            Sleep(2000);
            return;
        }

        auto it = find_if(usersMap.begin(), usersMap.end(),
            [&](const auto& pair) { return pair.second.username == username; });
        if (it == usersMap.end())
        {
            cout << BRED << "Incorrect username. Please try again.\n";
            continue;
        }

        const User& currentUser = it->second;

        cout << "Master Password: ";
        string password;
        char ch;

        while (true)
        {
            ch = getch();
            if (ch == '\r')
                break;
            else if (ch == '\b')
            {
                if (!password.empty())
                {
                    password.pop_back();
                    cout << "\b \b";
                }
            }
            else
            {
                password += ch;
                cout << '*';
            }
        }

        cout << '\n';

        if (comparePasswords(password, currentUser.masterPassword))
        {
            cout << GREEN << "Welcome! Logging you in..." << RESET;
            Sleep(2000);
            userInSession = currentUser.userID;
            system("CLS");
            passwordManagement(usersMap, currentUser.username, userInSession);
            return;
        }
        else
        {
            cout << BRED << "Incorrect password. Please try again.\n";
            passwordAttempts++;
        }
    }

    if (passwordAttempts >= 8)
    {
        cout << BRED << "The client has been locked out temporarily. Please try again later.\n";
        lockoutStatus.isLocked = true;
        lockoutStatus.lockoutTime = time(0);
        saveLockoutStatus(lockoutStatus);
    }
}

void passwordManagement(const map<string, User> usersMap, const string& username, string& userInSession)
{
    map<string, vector<Account>> accountsMap;
    loadAccounts(accountsMap);

    auto it = find_if(usersMap.begin(), usersMap.end(),
            [&](const auto& pair) { return pair.second.username == username; });

    int choice;

    do
    {
        system("CLS");

        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║       CyberTrex Password Manager         ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n';

        cout << "\n" << YELLOW << "Welcome, " << BOLD << username << RESET << "!\n\n";

        cout << BLUE << "┌──────────────────────────────────────────┐" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "1. Add an Account                        " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "2. Display Accounts                      " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "3. Search an Account                     " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "4. Update an Account                     " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "5. Delete an Account                     " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << "6. Log out                               " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "└──────────────────────────────────────────┘" << RESET << '\n';

        choice = get_int(YELLOW "\n>> " RESET);

        switch (choice)
        {
            case 1:
                addAccountToUser(it->second.userID, accountsMap);
                break;
            case 2:
                showAccounts(it->second.userID, accountsMap);
                break;
            case 3:
                searchAccounts(it->second.userID, accountsMap);
                break;
            case 4:
                updateAccount(it->second.userID, accountsMap);
                break;
            case 5:
                deleteAccount(it->second.userID, accountsMap);
                break;
            case 6:
                userInSession = "";
                cout << GREEN << "\nLogging out..." << RESET;
                Sleep(1500);
                system("cls");
                return;
            default:
                cout << RED << "\nInvalid option. Please try again." << RESET << '\n';
                Sleep(1500);
                break;
        }
    }
    while (choice != 6);
}

void addAccountToUser(const string& userID, map<string, vector<Account>>& accountsMap)
{
    Account newAccount;

    cout << "Enter account category (press 0 to exit): ";
    getline(cin >> ws, newAccount.category);

    if (newAccount.category == "0")
    {
        cout << "Returning to main menu...";
        Sleep(2000);
        return;
    }

    cout << "Enter account username: ";
    getline(cin >> ws, newAccount.username);

    cout << "Enter account password: ";
    string plainPassword;
    getline(cin >> ws, plainPassword);
    newAccount.password = xorEncrypt(plainPassword, XOR_KEY);

    newAccount.userID = userID;

    accountsMap[userID].push_back(newAccount);

    ofstream outFile(ACCOUNTS_LIST, ios::app);
    if (outFile)
    {
        outFile << "userID: " << newAccount.userID << '\n'
                << "category: " << newAccount.category << '\n'
                << "username: " << newAccount.username << '\n'
                << "password: " << newAccount.password << '\n'
                << "---\n";
        cout << GREEN << "Account saved!" << '\n';
        Sleep(2000);
        system("CLS");
    }
}

void showAccounts(const string& userID, const map<string, vector<Account>>& accountsMap)
{
    system("cls");
    auto it = accountsMap.find(userID);
    if (it != accountsMap.end() && !it->second.empty())
    {
        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║              Your Accounts               ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n';

        cout << BLUE << "┌──────────────────┬──────────────────┬──────────────────┐" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << setw(16) << left << "Category"
             << BLUE << " │ " << WHITE << setw(16) << left << "Username"
             << BLUE << " │ " << WHITE << setw(16) << left << "Password"
             << BLUE << " │" << RESET << '\n';
        cout << BLUE << "├──────────────────┼──────────────────┼──────────────────┤" << RESET << '\n';

        for (const auto& account : it->second)
        {
            cout << BLUE << "│ " << WHITE << setw(16) << left << account.category
                 << BLUE << " │ " << WHITE << setw(16) << left << account.username
                 << BLUE << " │ " << WHITE << setw(16) << left << xorDecrypt(account.password, XOR_KEY)
                 << BLUE << " │" << RESET << '\n';
        }

        cout << BLUE << "└──────────────────┴──────────────────┴──────────────────┘" << RESET << '\n';

        cout << YELLOW << "\nPress any key to continue..." << RESET;
        cin.get();
    }
    else
    {
        cout << RED << "No accounts found for this user." << RESET << '\n';
        Sleep(2000);
    }
}

void updateAccount(const string& userID, map<string, vector<Account>>& accountsMap)
{
    system("CLS");
    auto it = accountsMap.find(userID);
    if (it != accountsMap.end())
    {
        vector<Account>& userAccounts = it->second;
        if (userAccounts.empty())
        {
            cout << RED << "No accounts available to update." << RESET << '\n';
            Sleep(2000);
            return;
        }

        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║            Update an Account             ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n';

        cout << "\nYour Accounts:\n";
        cout << BLUE << "┌──────┬─────────────┬─────────────┬─────────────┐" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << " No. │  Category   │  Username   │  Password   " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "├──────┼─────────────┼─────────────┼─────────────┤" << RESET << '\n';

        for (size_t i = 0; i < userAccounts.size(); ++i)
        {
            cout << BLUE << "│ " << WHITE << setw(4) << i + 1 << " │ "
                 << setw(11) << left << userAccounts[i].category << " │ "
                 << setw(11) << userAccounts[i].username << " │ "
                 << setw(11) << xorDecrypt(userAccounts[i].password, XOR_KEY) << BLUE << " │" << RESET << '\n';
        }
        cout << BLUE << "└──────┴─────────────┴─────────────┴─────────────┘" << RESET << '\n';

        int choice = get_int("\nSelect the account number you want to update (0 to exit): ") - 1;

        if (choice == -1)
        {
            cout << YELLOW << "Returning to main menu..." << RESET;
            Sleep(2000);
            return;
        }

        if (choice < 0 || choice >= userAccounts.size())
        {
            cout << RED << "Invalid selection. Returning to menu." << RESET << '\n';
            Sleep(2000);
            return;
        }

        cout << YELLOW << "\nUpdating account: " << WHITE << userAccounts[choice].category << RESET << '\n';
        cout << YELLOW << "Leave fields blank to keep current values.\n" << RESET;

        string newCategory, newUsername, newPassword;

        cout << CYAN << "Current category: " << WHITE << userAccounts[choice].category << RESET << '\n';
        cout << "Enter new category: ";
        getline(cin, newCategory);
        if (!newCategory.empty()) userAccounts[choice].category = newCategory;

        cout << CYAN << "Current username: " << WHITE << userAccounts[choice].username << RESET << '\n';
        cout << "Enter new username: ";
        getline(cin, newUsername);
        if (!newUsername.empty()) userAccounts[choice].username = newUsername;

        cout << CYAN << "Current password: " << WHITE << xorDecrypt(userAccounts[choice].password, XOR_KEY) << RESET << '\n';
        cout << "Enter new password: ";
        getline(cin, newPassword);
        if (!newPassword.empty()) userAccounts[choice].password = xorEncrypt(newPassword, XOR_KEY);;

        saveAccounts(accountsMap);


        cout << BGREEN << "Account updated successfully!" << RESET << '\n';
        Sleep(2000);
        system("CLS");
    }
    else
    {
        cout << BRED << "No accounts found for this user.\n";
        Sleep(3000);
        system("CLS");
    }
}

void deleteAccount(const string& userID, map<string, vector<Account>>& accountsMap)
{
    system("cls");
    auto it = accountsMap.find(userID);
    if (it != accountsMap.end())
    {
        vector<Account>& userAccounts = it->second;
        if (userAccounts.empty())
        {
            cout << RED << "No accounts available to delete." << RESET << '\n';
            Sleep(2000);
            return;
        }

        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║            Delete an Account             ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n';

        cout << "\nYour Accounts:\n";
        cout << BLUE << "┌──────┬─────────────┬─────────────┬─────────────┐" << RESET << '\n';
        cout << BLUE << "│ " << WHITE << " No. │  Category   │  Username   │  Password   " << BLUE << "│" << RESET << '\n';
        cout << BLUE << "├──────┼─────────────┼─────────────┼─────────────┤" << RESET << '\n';

        for (size_t i = 0; i < userAccounts.size(); ++i)
        {
            cout << BLUE << "│ " << WHITE << setw(4) << i + 1 << " │ "
                 << setw(11) << left << userAccounts[i].category << " │ "
                 << setw(11) << userAccounts[i].username << " │ "
                 << setw(11) << xorDecrypt(userAccounts[i].password, XOR_KEY) << BLUE << " │" << RESET << '\n';
        }
        cout << BLUE << "└──────┴─────────────┴─────────────┴─────────────┘" << RESET << '\n';

        int choice = get_int("\nSelect the account number you want to delete (0 to exit): ") - 1;

        if (choice == -1)
        {
            cout << YELLOW << "Returning to main menu..." << RESET;
            Sleep(2000);
            return;
        }

        if (choice < 0 || choice >= userAccounts.size())
        {
            cout << RED << "Invalid selection. Returning to menu." << RESET << '\n';
            Sleep(2000);
            return;
        }

        cout << YELLOW << "Are you sure you want to delete this account? (Y/N): " << RESET;
        char prompt;
        cin >> prompt;
        cin.ignore();

        if (toupper(prompt) != 'Y')
        {
            cout << GREEN << "Deletion cancelled. Returning to menu." << RESET << '\n';
            Sleep(2000);
            return;
        }

        userAccounts.erase(userAccounts.begin() + choice);
        saveAccounts(accountsMap);

        cout << GREEN << "\nAccount deleted successfully!" << RESET << '\n';
        Sleep(2000);
    }
    else
    {
        cout << RED << "No accounts found for this user. Return to main menu to add an account." << RESET << '\n';
        Sleep(2000);
    }
}


void searchAccounts(const string& userID, const map<string, vector<Account>>& accountsMap)
{
    if (accountsMap.find(userID) == accountsMap.end() || accountsMap.at(userID).empty())
    {
        cout << YELLOW << "No accounts found for this user." << RESET << endl;
        Sleep(2000);
        return;
    }

    string searchTerm;
    vector<Account> filteredAccounts;

    while (true)
    {
        system("cls");
        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║            Search an Account             ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n' << '\n';

        cout << YELLOW << "Enter search term (press Esc to exit): " << RESET;
        cout << searchTerm;

        if (!searchTerm.empty())
        {
            filteredAccounts.clear();
            string lowerSearchTerm = toLower(searchTerm);
            for (const auto& account : accountsMap.at(userID))
            {
                if (toLower(account.category).find(lowerSearchTerm) != string::npos ||
                    toLower(account.username).find(lowerSearchTerm) != string::npos)
                {
                    filteredAccounts.push_back(account);
                }
            }

            if (!filteredAccounts.empty())
            {
                cout << "\n\n";
                cout << BLUE << "┌────────────────────┬────────────────────┬────────────────────┐" << RESET << endl;
                cout << BLUE << "│ " << WHITE << setw(18) << left << "Category"
                     << BLUE << " │ " << WHITE << setw(18) << left << "Username"
                     << BLUE << " │ " << WHITE << setw(18) << left << "Password" << BLUE << " │" << RESET << endl;
                cout << BLUE << "├────────────────────┼────────────────────┼────────────────────┤" << RESET << endl;

                for (const auto& account : filteredAccounts)
                {
                    cout << BLUE << "│ " << WHITE << setw(18) << left << account.category
                         << BLUE << " │ " << WHITE << setw(18) << left << account.username
                         << BLUE << " │ " << WHITE << setw(18) << left << xorDecrypt(account.password, XOR_KEY) << BLUE << " │" << RESET << endl;
                }

                cout << BLUE << "└────────────────────┴────────────────────┴────────────────────┘" << RESET << endl;
            }
            else
            {
                cout << "\n\n" << YELLOW << "No matching accounts found." << RESET << endl;
            }
        }

        char ch = _getch();

        if (ch == 27) // Esc key
        {
            break;
        }
        else if (ch == 8) // Backspace
        {
            if (!searchTerm.empty())
            {
                searchTerm.pop_back();
            }
        }
        else if (isprint(ch))
        {
            searchTerm += ch;
        }
    }
}

// File operations and helper functions
void loadUsers(map<string, User>& usersMap)
{
    ifstream inFile(USERS);
    if (inFile)
    {
        string line;
        User user;
        while (getline(inFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            if (line == "---")
            {
                usersMap[user.userID] = user;
                continue;
            }

            size_t pos = line.find(": ");
            if (pos != string::npos)
            {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 2);

                if (key == "id")
                {
                    user.userID = value;
                }
                else if (key == "username")
                {
                    user.username = value;
                }
                else if (key == "masterPassword")
                {
                    user.masterPassword = stoull(value);
                }
            }
        }

        inFile.close();
    }
}

void loadAccounts(map<string, vector<Account>>& accountsMap)
{
    ifstream inFile(ACCOUNTS_LIST);
    if (inFile)
    {
        string line;
        Account account;
        string currentUserID;

        while (getline(inFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            if (line == "---")
            {
                accountsMap[currentUserID].push_back(account);
                account = Account();
                continue;
            }

            size_t pos = line.find(": ");
            if (pos != string::npos)
            {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 2);

                if (key == "userID")
                {
                    currentUserID = value;
                }
                else if (key == "category")
                {
                    account.category = value;
                }
                else if (key == "username")
                {
                    account.username = value;
                }
                else if (key == "password")
                {
                    account.password = value;
                }
            }
        }
        inFile.close();
    }
}

void saveUserToFile(const string& filename, const map<string, User>& usersMap)
{
    ofstream outFile(filename);

    if (!outFile)
    {
        cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    for (const auto& pair : usersMap)
    {
        const User& user = pair.second;
        outFile << "id: " << user.userID << '\n'
                << "username: " << user.username << '\n'
                << "masterPassword: " << user.masterPassword << '\n'
                << "---\n";
    }

    outFile.close();
}

void saveAccounts(const map<string, vector<Account>>& accountsMap)
{
    ofstream outFile(ACCOUNTS_LIST);

    if (!outFile)
    {
        cerr << "File cannot be opened for writing\n";
        return;
    }

    for (const auto& [userId, accounts] : accountsMap)
    {
        for (const auto& account : accounts)
        {
            outFile << "userID: " << userId << '\n'
                    << "category: " << account.category << '\n'
                    << "username: " << account.username << '\n'
                    << "password: " << account.password << '\n'
                    << "---\n";
        }
    }
    outFile.close();
}

// Helper functions

int generate_unique_id()
{
    srand(time(0));

    return rand() % 900 + 100;
}

size_t create_password()
{
    regex password_regex("^[\x20-\x7E]{8,64}$");
    string password;

    while (true)
    {
        cout << CYAN << "Enter password: ";
        cin >> password;

        if (regex_match(password, password_regex))
        {
            hash<string> hash_fn;
            size_t hashed_password = hash_fn(password);

            return hashed_password;
        }
        else
        {
            cout << BRED << "Passwords must be at least 8 characters minimum." << '\n';
        }
    }
}

bool comparePasswords(string passInput, size_t hashedPassword)
{
    hash<string> hash_fn;
    size_t hashedInputPassword = hash_fn(passInput);

    return hashedInputPassword == hashedPassword;
}

int get_int(const string& prompt)
{
    regex integer_regex("^-?[0-9]+$");
    string input;

    while (true)
    {
        cout << prompt;
        getline(cin, input);

        if (regex_match(input, integer_regex))
        {
            try
            {
                return stoi(input);
            }
            catch (out_of_range&)
            {
                cout << "Error: Number out of range. Please try again.\n";
            }
        }
        else
        {
            cout << BRED << "Invalid input. Please enter a valid integer.\n" << RESET;
        }
    }
}

string toLower(const string& str)
{
    string lower = str;
    transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return tolower(c); });
    return lower;
}

string xorEncrypt(const string& input, const string& key)
{
    string output = input;
    for (size_t i = 0; i < input.size(); ++i)
    {
        output[i] = input[i] ^ key[i % key.size()];
    }
    return output;
}

string xorDecrypt(const string& input, const string& key)
{
    return xorEncrypt(input, key); // XOR encryption is symmetric
}

// UI
void displayStartScreen()
{
    system("CLS"); // Clear the console screen
    cout << CYAN; // Set color to Cyan for the banner

    cout << R"(
 ▄████▄▓██   ██▓ ▄▄▄▄   ▓█████  ██▀███  ▄▄▄█████▓ ██▀███  ▓█████ ▒██   ██▒
▒██▀ ▀█ ▒██  ██▒▓█████▄ ▓█   ▀ ▓██ ▒ ██▒▓  ██▒ ▓▒▓██ ▒ ██▒▓█   ▀ ▒▒ █ █ ▒░
▒▓█    ▄ ▒██ ██░▒██▒ ▄██▒███   ▓██ ░▄█ ▒▒ ▓██░ ▒░▓██ ░▄█ ▒▒███   ░░  █   ░
▒▓▓▄ ▄██▒░ ▐██▓░▒██░█▀  ▒▓█  ▄ ▒██▀▀█▄  ░ ▓██▓ ░ ▒██▀▀█▄  ▒▓█  ▄  ░ █ █ ▒
▒ ▓███▀ ░░ ██▒▓░░▓█  ▀█▓░▒████▒░██▓ ▒██▒  ▒██▒ ░ ░██▓ ▒██▒░▒████▒▒██▒ ▒██▒
░ ░▒ ▒  ░ ██▒▒▒ ░▒▓███▀▒░░ ▒░ ░░ ▒▓ ░▒▓░  ▒ ░░   ░ ▒▓ ░▒▓░░░ ▒░ ░▒▒ ░ ░▓ ░
  ░  ▒  ▓██ ░▒░ ▒░▒   ░  ░ ░  ░  ░▒ ░ ▒░    ░      ░▒ ░ ▒░ ░ ░  ░░░   ░▒ ░
░       ▒ ▒ ░░   ░    ░    ░     ░░   ░   ░        ░░   ░    ░    ░    ░
░ ░     ░ ░      ░         ░  ░   ░                 ░        ░  ░ ░    ░
░       ░ ░           ░
    )" << RESET << '\n';
    cout << CYAN << "Loading...";

    Sleep(5000);
    return;
}

// Lockout status
LockoutStatus loadLockoutStatus()
{
    LockoutStatus status;
    ifstream inFile(LOCKED_STATUS);
    if (inFile)
    {
        string line;
        while (getline(inFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            if (line.find("locked: true") != string::npos)
            {
                status.isLocked = true;
                if (getline(inFile, line) && !line.empty())
                {
                    try
                    {
                        status.lockoutTime = stoll(line);
                    }
                    catch (const invalid_argument& e)
                    {
                        cout << BRED << "Error: Invalid lockout time format in lockout status file.\n";
                        status.isLocked = false;
                    }
                    catch (const out_of_range& e)
                    {
                        cout << BRED << "Error: Lockout time is out of range.\n";
                        status.isLocked = false;
                    }
                }
                break;
            }
        }
        inFile.close();
    }
    else
    {
        cout << "Error opening lockout status file for reading.\n";
    }
    return status;
}

void saveLockoutStatus(const LockoutStatus& status)
{
    ofstream outFile(LOCKED_STATUS);
    if (outFile)
    {
        if (status.isLocked)
        {
            outFile << "locked: true\n" << status.lockoutTime << '\n';
        }
        else
        {
            outFile << "locked: false\n";
        }
    }
    else
    {
        cout << "Error opening lockout status file for writing.\n";
    }
}
