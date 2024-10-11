#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <regex>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <windows.h>

using namespace std;

const string USERS = "users.txt";
const string ACCOUNTS_LIST = "accounts.txt";
const string LOCKED_STATUS = "locked.txt";

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

LockoutStatus loadLockoutStatus();
void saveLockoutStatus(const LockoutStatus& status);
int get_int(const string& prompt);
int generate_unique_id();
size_t create_password();
void saveUserToFile(const string& filename, const map<string, User>& usersMap);
void saveAccountToUser(const string& userID, map<string, vector<Account>>& accountsMap);
void loadUsers(map<string, User>& usersMap);
void loadAccounts(map<string, vector<Account>>& accountsMap);
void createUser(map<string, User>& usersMap);
bool comparePasswords(string passInput, size_t hashedPassword);
void logInUser(const map<string, User>& usersMap, string& userInSession, LockoutStatus& lockoutStatus);
void showAccounts(const string& userID, const map<string, vector<Account>>& accountsMap);
void passwordManagement(const map<string, User> usersMap, const string& username);


int main()
{
    map<string, User> usersMap;
    LockoutStatus lockoutStatus = loadLockoutStatus();
    string userInSession = "";

    loadUsers(usersMap);

    while (userInSession == "")
    {
        cout << "CyberTrex Password Manager" << '\n';
        cout << "--------------------------" << '\n';
        cout << "1. Create User\n";
        cout << "2. Log In\n";
        cout << "3. Exit\n";
        int choice = get_int("\n>> ");

        switch (choice)
        {
            case 1:
                createUser(usersMap);
                break;
            case 2:
                logInUser(usersMap, userInSession, lockoutStatus);
                break;
            case 3:
                return 0;
            default:
                cout << "Invalid option. Please try again.\n";
        }
    }
}

LockoutStatus loadLockoutStatus()
{
    LockoutStatus status;
    ifstream inFile(LOCKED_STATUS);
    if (inFile)
    {
        string line;
        while (getline(inFile, line))
        {
            // Skip empty lines
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
                    catch (const std::invalid_argument& e)
                    {
                        cout << "Error: Invalid lockout time format in lockout status file.\n";
                        status.isLocked = false;
                    }
                    catch (const std::out_of_range& e)
                    {
                        cout << "Error: Lockout time is out of range.\n";
                        status.isLocked = false; // Reset lock status on error
                    }
                }
                break; // Exit after finding the locked status
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
            cout << "Invalid input. Please enter a valid integer.\n";
        }
    }
}

int generate_unique_id()
{
    // placeholder
    static int id = 0;

    return id++;
}

size_t create_password()
{
    regex password_regex("^[\x20-\x7E]{8,64}$");
    string password;

    while (true)
    {
        cout << "Enter password: ";
        cin >> password;

        if (regex_match(password, password_regex))
        {
            hash<string> hash_fn;
            size_t hashed_password = hash_fn(password);

            return hashed_password;
        }
        else
        {
            cout << "Passwords must be at least 8 characters minimum." << '\n';
        }
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

void saveAccountToUser(const string& userID, map<string, vector<Account>>& accountsMap)
{
    Account newAccount;

    cout << "Enter account category: ";
    getline(cin >> ws, newAccount.category);

    cout << "Enter account username: ";
    getline(cin >> ws, newAccount.username);

    cout << "Enter account password: ";
    getline(cin >> ws, newAccount.password);

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
        cout << "Account saved!" << '\n';
        Sleep(2000);
        system("CLS");
    }
    else
    {
        cout << "Error opening accounts file for writing.\n";
    }
}

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
    else
    {
        cout << "Error opening user file for reading.\n";
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
    else
    {
        cout << "Error opening accounts file for reading.\n";
    }
}

void createUser(map<string, User>& usersMap)
{
    User newUser;

    cout << "Enter username: ";
    getline(cin >> ws, newUser.username);

    newUser.masterPassword = create_password();
    newUser.userID = to_string(generate_unique_id());

    usersMap[newUser.userID] = newUser;

    saveUserToFile(USERS, usersMap);

    cout << "User created successfully!" << '\n';
}


bool comparePasswords(string passInput, size_t hashedPassword)
{
    hash<string> hash_fn;
    size_t hashedInputPassword = hash_fn(passInput);

    return hashedInputPassword == hashedPassword;
}

void logInUser(const map<string, User>& usersMap, string& userInSession, LockoutStatus& lockoutStatus)
{
    int passwordAttempts = 0;

    if (lockoutStatus.isLocked)
    {
        time_t currentTime = time(0);
        if (difftime(currentTime, lockoutStatus.lockoutTime) < 900)
        {
            cout << "You have reached the maximum number of attempts. Please try again later.\n";
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
        cout << "Username: ";
        string username;
        getline(cin >> ws, username);

        auto it = find_if(usersMap.begin(), usersMap.end(),
            [&](const auto& pair) { return pair.second.username == username; });
        if (it == usersMap.end())
        {
            cout << "Incorrect username. Please try again.\n";
            continue;
        }

        const User& currentUser = it->second;

        cout << "Master Password: ";
        string password;
        getline(cin >> ws, password);

        if (comparePasswords(password, currentUser.masterPassword))
        {
            cout << "Welcome! Logging you in...";
            Sleep(4000);
            userInSession = currentUser.userID;
            system("CLS");
            passwordManagement(usersMap, currentUser.username);
            return;
        }
        else
        {
            cout << "Incorrect password. Please try again.\n";
            passwordAttempts++;
        }
    }

    if (passwordAttempts >= 8)
    {
        cout << "The client has been locked out temporarily. Please try again later.\n";
        lockoutStatus.isLocked = true;
        lockoutStatus.lockoutTime = time(0);
        saveLockoutStatus(lockoutStatus);
    }
}

void showAccounts(const string& userID, const map<string, vector<Account>>& accountsMap)
{
    system("CLS");
    auto it = accountsMap.find(userID);
    if (it != accountsMap.end())
    {
        cout << "Accounts" << '\n';
        cout << "--------------------------\n";
        for (const auto& account : it->second)
        {
            cout << "Category: " << account.category
                 << ", Username: " << account.username
                 << ", Password: " << account.password << '\n';
        }
        cout << "Press any key to continue...";
        cin.get();
        system("CLS");

    }
    else
    {
        cout << "No accounts found for this user.\n";
    }
}


void passwordManagement(const map<string, User> usersMap, const string& username)
{
    map<string, vector<Account>> accountsMap;
    loadAccounts(accountsMap);

    auto it = find_if(usersMap.begin(), usersMap.end(),
            [&](const auto& pair) { return pair.second.username == username; });

    int choice;

    do
    {
        cout << "CyberTrex Password Manager" << '\n';
        cout << "--------------------------" << '\n';
        cout << "Welcome, " << username << "!" << '\n';
        cout << "\nOptions:" << '\n';
        cout << "1. Add an Account" << '\n';
        cout << "2. Display accounts" << '\n';
        cout << "3. Update an account" << '\n';
        cout << "4. Delete an account" << '\n';
        cout << "5. Log out" << '\n';

        choice = get_int("\n>> ");

        switch (choice)
        {
            case 1:
                // TODO: add account function
                saveAccountToUser(it->second.userID, accountsMap);
                break;
            case 2:
                showAccounts(it->second.userID, accountsMap);
                break;
            case 3:
                // TODO: update account function
                break;
            case 4:
                // TODO: delete account function
                break;
            case 5:
                return;
            default:
                cout << "Invalid option. Please try again" << '\n';
                break;


        }
    }
    while (choice != 5);
}
