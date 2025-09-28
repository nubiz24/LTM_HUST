#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define MAX_EMAIL 100
#define MAX_PHONE 20
#define MAX_LINE 256
#define ACCOUNT_FILE "account.txt"
#define HISTORY_FILE "history.txt"
#define MAX_LOGIN_ATTEMPTS 3
#define UNLOCK_TIME_MINUTES 10

// User account structure
typedef struct User
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char email[MAX_EMAIL];
    char phone[MAX_PHONE];
    int status;       // 1: active, 0: blocked
    char role[20];    // "admin" or "user"
    time_t lock_time; // Lock time
    struct User *next;
} User;

// Login history structure
typedef struct LoginHistory
{
    char username[MAX_USERNAME];
    char date[20];
    char time[20];
    struct LoginHistory *next;
} LoginHistory;

// Global variables
User *user_list = NULL;
LoginHistory *history_list = NULL;
User *current_user = NULL; // Currently logged in user

// Function declarations
User *create_user(char *username, char *password, char *email, char *phone, int status, char *role, time_t lock_time);
void add_user_to_list(User **list, User *new_user);
void load_users_from_file();
void save_users_to_file();
void reload_users_from_file();
void load_history_from_file();
void save_history_to_file();
void free_user_list();
void free_history_list();
void clear_input_buffer();
void trim_string(char *str);
int is_valid_email(char *email);
int is_valid_phone(char *phone);
void display_menu();
void register_user();
void sign_in();
void change_password();
void update_account_info();
void reset_password();
void view_login_history();
void auto_unlock_accounts();
void admin_functions();
void sign_out();
void handle_choice(int choice);

// Create a new user account
User *create_user(char *username, char *password, char *email, char *phone, int status, char *role, time_t lock_time)
{
    User *new_user = (User *)malloc(sizeof(User));
    if (new_user == NULL)
    {
        printf("Error: Cannot allocate memory!\n");
        return NULL;
    }

    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    strcpy(new_user->email, email);
    strcpy(new_user->phone, phone);
    new_user->status = status;
    strcpy(new_user->role, role);
    new_user->lock_time = lock_time;
    new_user->next = NULL;

    return new_user;
}

// Add user to list
void add_user_to_list(User **list, User *new_user)
{
    if (*list == NULL)
    {
        *list = new_user;
    }
    else
    {
        User *current = *list;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_user;
    }
}

// Load user list from file
void load_users_from_file()
{
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (file == NULL)
    {
        printf("Cannot open file %s. Creating new file...\n", ACCOUNT_FILE);
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file))
    {
        trim_string(line);
        if (strlen(line) == 0)
            continue;

        char username[MAX_USERNAME], password[MAX_PASSWORD], email[MAX_EMAIL], phone[MAX_PHONE], role[20];
        int status;
        time_t lock_time = 0;

        // Try to read with lock_time first (new format)
        if (sscanf(line, "%s %s %s %s %d %s %ld", username, password, email, phone, &status, role, &lock_time) == 7)
        {
            User *new_user = create_user(username, password, email, phone, status, role, lock_time);
            if (new_user != NULL)
            {
                add_user_to_list(&user_list, new_user);
            }
        }
        // Fallback to old format (without lock_time)
        else if (sscanf(line, "%s %s %s %s %d %s", username, password, email, phone, &status, role) == 6)
        {
            User *new_user = create_user(username, password, email, phone, status, role, 0);
            if (new_user != NULL)
            {
                add_user_to_list(&user_list, new_user);
            }
        }
    }

    fclose(file);
}

// Save user list to file
void save_users_to_file()
{
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if (file == NULL)
    {
        printf("Error: Cannot open file %s for writing!\n", ACCOUNT_FILE);
        return;
    }

    User *current = user_list;
    while (current != NULL)
    {
        fprintf(file, "%s %s %s %s %d %s %ld\n",
                current->username, current->password, current->email,
                current->phone, current->status, current->role, current->lock_time);
        current = current->next;
    }

    fclose(file);
}

// Reload user list from file (useful after modifications)
void reload_users_from_file()
{
    char current_username[MAX_USERNAME] = "";
    if (current_user != NULL)
    {
        strcpy(current_username, current_user->username);
    }

    free_user_list();
    load_users_from_file();

    // Restore current_user pointer after reload
    if (strlen(current_username) > 0)
    {
        User *current = user_list;
        while (current != NULL)
        {
            if (strcmp(current->username, current_username) == 0)
            {
                current_user = current;
                break;
            }
            current = current->next;
        }
    }
}

// Load login history from file
void load_history_from_file()
{
    FILE *file = fopen(HISTORY_FILE, "r");
    if (file == NULL)
    {
        return; // File doesn't exist yet, no need to load
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file))
    {
        trim_string(line);
        if (strlen(line) == 0)
            continue;

        char username[MAX_USERNAME], date[20], time[20];
        if (sscanf(line, "%s | %s | %s", username, date, time) == 3)
        {
            LoginHistory *new_history = (LoginHistory *)malloc(sizeof(LoginHistory));
            if (new_history != NULL)
            {
                strcpy(new_history->username, username);
                strcpy(new_history->date, date);
                strcpy(new_history->time, time);
                new_history->next = history_list;
                history_list = new_history;
            }
        }
    }

    fclose(file);
}

// Save login history to file
void save_history_to_file()
{
    FILE *file = fopen(HISTORY_FILE, "w");
    if (file == NULL)
    {
        printf("Error: Cannot open file %s for writing!\n", HISTORY_FILE);
        return;
    }

    LoginHistory *current = history_list;
    while (current != NULL)
    {
        fprintf(file, "%s | %s | %s\n", current->username, current->date, current->time);
        current = current->next;
    }

    fclose(file);
}

// Free user list memory
void free_user_list()
{
    User *current = user_list;
    while (current != NULL)
    {
        User *temp = current;
        current = current->next;
        free(temp);
    }
    user_list = NULL;
}

// Free history list memory
void free_history_list()
{
    LoginHistory *current = history_list;
    while (current != NULL)
    {
        LoginHistory *temp = current;
        current = current->next;
        free(temp);
    }
    history_list = NULL;
}

// Clear input buffer
void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

// Remove leading and trailing whitespace from string
void trim_string(char *str)
{
    int start = 0;
    int end = strlen(str) - 1;

    while (isspace(str[start]))
        start++;
    while (end > start && isspace(str[end]))
        end--;

    int i;
    for (i = start; i <= end; i++)
    {
        str[i - start] = str[i];
    }
    str[end - start + 1] = '\0';
}

// Validate email format
int is_valid_email(char *email)
{
    int at_count = 0;
    int dot_count = 0;
    int at_pos = -1;
    int dot_pos = -1;

    for (int i = 0; email[i] != '\0'; i++)
    {
        if (email[i] == '@')
        {
            at_count++;
            at_pos = i;
        }
        if (email[i] == '.')
        {
            dot_count++;
            dot_pos = i;
        }
    }

    return (at_count == 1 && dot_count >= 1 && at_pos > 0 && dot_pos > at_pos + 1);
}

// Validate phone number format
int is_valid_phone(char *phone)
{
    if (strlen(phone) < 10 || strlen(phone) > 15)
        return 0;

    for (int i = 0; phone[i] != '\0'; i++)
    {
        if (!isdigit(phone[i]) && phone[i] != '+' && phone[i] != '-')
        {
            return 0;
        }
    }
    return 1;
}

// Display main menu
void display_menu()
{
    printf("\n");
    printf("===============================================\n");
    printf("        USER MANAGEMENT PROGRAM                \n");
    printf("===============================================\n");
    printf("1. Register\n");
    printf("2. Sign in\n");
    printf("3. Change password\n");
    printf("4. Update account info\n");
    printf("5. Reset password\n");
    printf("6. View login history\n");
    printf("7. Auto unlock accounts\n");
    printf("8. Admin functions\n");
    printf("9. Sign out\n");
    printf("===============================================\n");
    printf("Your choice (1-9, other to quit): ");
}

// Function 1: Register new account
void register_user()
{
    printf("\n--- REGISTER NEW ACCOUNT ---\n");

    char username[MAX_USERNAME], password[MAX_PASSWORD], email[MAX_EMAIL], phone[MAX_PHONE];

    printf("Enter username: ");
    scanf("%s", username);
    clear_input_buffer();

    // Check if username already exists
    User *current = user_list;
    while (current != NULL)
    {
        if (strcmp(current->username, username) == 0)
        {
            printf("Error: Username already exists!\n");
            return;
        }
        current = current->next;
    }

    printf("Enter password: ");
    scanf("%s", password);
    clear_input_buffer();

    printf("Enter email: ");
    scanf("%s", email);
    clear_input_buffer();

    if (!is_valid_email(email))
    {
        printf("Error: Invalid email format!\n");
        return;
    }

    printf("Enter phone number: ");
    scanf("%s", phone);
    clear_input_buffer();

    if (!is_valid_phone(phone))
    {
        printf("Error: Invalid phone number format!\n");
        return;
    }

    // Create new user account
    User *new_user = create_user(username, password, email, phone, 1, "user", 0);
    if (new_user != NULL)
    {
        add_user_to_list(&user_list, new_user);
        save_users_to_file();
        printf("Account registration successful!\n");
    }
    else
    {
        printf("Error: Cannot create account!\n");
    }
}

// Function 2: Sign in
void sign_in()
{
    printf("\n--- SIGN IN ---\n");

    if (current_user != NULL)
    {
        printf("You are already logged in as: %s\n", current_user->username);
        return;
    }

    char username[MAX_USERNAME], password[MAX_PASSWORD];

    printf("Enter username: ");
    scanf("%s", username);
    clear_input_buffer();

    // Find user first
    User *user = user_list;
    while (user != NULL && strcmp(user->username, username) != 0)
    {
        user = user->next;
    }

    if (user == NULL)
    {
        printf("Account does not exist!\n");
        return;
    }

    // Check account status
    if (user->status == 0)
    {
        // Check lock time
        if (user->lock_time > 0)
        {
            time_t current_time = time(NULL);
            if (current_time - user->lock_time < UNLOCK_TIME_MINUTES * 60)
            {
                printf("Your account is blocked.\n");
                return;
            }
            else
            {
                // Auto unlock
                user->status = 1;
                user->lock_time = 0;
                save_users_to_file();
                printf("Account has been automatically unlocked.\n");
            }
        }
        else
        {
            printf("Your account is blocked.\n");
            return;
        }
    }

    // Try password up to MAX_LOGIN_ATTEMPTS times
    int attempts = 0;
    while (attempts < MAX_LOGIN_ATTEMPTS)
    {
        printf("Enter password (attempt %d/%d): ", attempts + 1, MAX_LOGIN_ATTEMPTS);
        scanf("%s", password);
        clear_input_buffer();

        if (strcmp(user->password, password) == 0)
        {
            // Login successful
            current_user = user;
            printf("Welcome!\n");

            // Save login history
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char date_str[20], time_str[20];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
            strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

            LoginHistory *new_history = (LoginHistory *)malloc(sizeof(LoginHistory));
            if (new_history != NULL)
            {
                strcpy(new_history->username, username);
                strcpy(new_history->date, date_str);
                strcpy(new_history->time, time_str);
                new_history->next = history_list;
                history_list = new_history;
                save_history_to_file();
            }

            return;
        }
        else
        {
            attempts++;
            if (attempts < MAX_LOGIN_ATTEMPTS)
            {
                printf("Wrong password. %d attempts remaining.\n", MAX_LOGIN_ATTEMPTS - attempts);
            }
        }
    }

    // All attempts failed - lock account
    user->status = 0;
    user->lock_time = time(NULL);
    save_users_to_file();
    printf("Account has been locked due to %d failed login attempts.\n", MAX_LOGIN_ATTEMPTS);
}

// Function 3: Change password
void change_password()
{
    printf("\n--- CHANGE PASSWORD ---\n");

    if (current_user == NULL)
    {
        printf("Error: You need to log in first!\n");
        return;
    }

    char old_password[MAX_PASSWORD], new_password[MAX_PASSWORD];

    printf("Enter old password: ");
    scanf("%s", old_password);
    clear_input_buffer();

    if (strcmp(current_user->password, old_password) != 0)
    {
        printf("Error: Old password is incorrect!\n");
        return;
    }

    printf("Enter new password: ");
    scanf("%s", new_password);
    clear_input_buffer();

    strcpy(current_user->password, new_password);
    save_users_to_file();
    printf("Password changed successfully!\n");
}

// Function 4: Update account information
void update_account_info()
{
    printf("\n--- UPDATE ACCOUNT INFO ---\n");

    if (current_user == NULL)
    {
        printf("Error: You need to log in first!\n");
        return;
    }

    int choice;
    printf("1. Update email\n");
    printf("2. Update phone number\n");
    printf("Choice: ");
    scanf("%d", &choice);
    clear_input_buffer();

    if (choice == 1)
    {
        char new_email[MAX_EMAIL];
        printf("Enter new email: ");
        scanf("%s", new_email);
        clear_input_buffer();

        if (is_valid_email(new_email))
        {
            strcpy(current_user->email, new_email);
            save_users_to_file();
            printf("Email updated successfully!\n");
        }
        else
        {
            printf("Error: Invalid email format!\n");
        }
    }
    else if (choice == 2)
    {
        char new_phone[MAX_PHONE];
        printf("Enter new phone number: ");
        scanf("%s", new_phone);
        clear_input_buffer();

        if (is_valid_phone(new_phone))
        {
            strcpy(current_user->phone, new_phone);
            save_users_to_file();
            printf("Phone number updated successfully!\n");
        }
        else
        {
            printf("Error: Invalid phone number format!\n");
        }
    }
    else
    {
        printf("Invalid choice!\n");
    }
}

// Function 5: Reset password
void reset_password()
{
    printf("\n--- RESET PASSWORD ---\n");

    char username[MAX_USERNAME], verification_code[20];

    printf("Enter username: ");
    scanf("%s", username);
    clear_input_buffer();

    // Check if account exists
    User *user = user_list;
    while (user != NULL)
    {
        if (strcmp(user->username, username) == 0)
        {
            printf("Enter verification code (enter '123456' for test): ");
            scanf("%s", verification_code);
            clear_input_buffer();

            if (strcmp(verification_code, "123456") == 0)
            {
                char new_password[MAX_PASSWORD];
                printf("Enter new password: ");
                scanf("%s", new_password);
                clear_input_buffer();

                strcpy(user->password, new_password);
                save_users_to_file();
                printf("Password reset successful!\n");
            }
            else
            {
                printf("Error: Verification code is incorrect!\n");
            }
            return;
        }
        user = user->next;
    }

    printf("Account does not exist!\n");
}

// Function 6: View login history
void view_login_history()
{
    printf("\n--- LOGIN HISTORY ---\n");

    if (current_user == NULL)
    {
        printf("Error: You need to log in first!\n");
        return;
    }

    printf("Login history for %s:\n", current_user->username);
    printf("%-20s %-15s %-10s\n", "Username", "Date", "Time");
    printf("-----------------------------------------\n");

    LoginHistory *current = history_list;
    int found = 0;

    while (current != NULL)
    {
        if (strcmp(current->username, current_user->username) == 0)
        {
            printf("%-20s %-15s %-10s\n", current->username, current->date, current->time);
            found = 1;
        }
        current = current->next;
    }

    if (!found)
    {
        printf("No login history found.\n");
    }
}

// Function 7: Auto unlock accounts
void auto_unlock_accounts()
{
    printf("\n--- CHECK AND UNLOCK ACCOUNTS ---\n");

    time_t current_time = time(NULL);
    User *current = user_list;
    int unlocked_count = 0;

    while (current != NULL)
    {
        if (current->status == 0 && current->lock_time > 0)
        {
            if (current_time - current->lock_time >= UNLOCK_TIME_MINUTES * 60)
            {
                current->status = 1;
                current->lock_time = 0;
                unlocked_count++;
                printf("Unlocked account: %s\n", current->username);
            }
        }
        current = current->next;
    }

    if (unlocked_count > 0)
    {
        save_users_to_file();
        printf("Unlocked %d accounts.\n", unlocked_count);
    }
    else
    {
        printf("No accounts need to be unlocked.\n");
    }
}

// Function 8: Admin functions
void admin_functions()
{
    printf("\n--- ADMIN FUNCTIONS ---\n");

    if (current_user == NULL)
    {
        printf("Error: You need to log in first!\n");
        return;
    }

    if (strcmp(current_user->role, "admin") != 0)
    {
        printf("Error: You do not have permission to access this function!\n");
        return;
    }

    int choice;
    printf("1. View all accounts\n");
    printf("2. Delete account\n");
    printf("3. Reset password for other user\n");
    printf("Choice: ");
    scanf("%d", &choice);
    clear_input_buffer();

    switch (choice)
    {
    case 1:
    {
        // Reload user list to get latest data
        reload_users_from_file();

        printf("\n%-20s %-15s %-20s %-15s %-10s %-10s\n",
               "Username", "Email", "Phone", "Status", "Role", "Lock Time");
        printf("---------------------------------------------------------------------------\n");

        User *current = user_list;
        while (current != NULL)
        {
            char status_str[10] = "Active";
            char lock_time_str[20] = "N/A";

            if (current->status == 0)
            {
                strcpy(status_str, "Blocked");
                if (current->lock_time > 0)
                {
                    time_t remaining = UNLOCK_TIME_MINUTES * 60 - (time(NULL) - current->lock_time);
                    if (remaining > 0)
                    {
                        sprintf(lock_time_str, "%ld min", remaining / 60);
                    }
                }
            }

            printf("%-20s %-15s %-20s %-15s %-10s %-10s\n",
                   current->username, current->email, current->phone,
                   status_str, current->role, lock_time_str);
            current = current->next;
        }
        break;
    }

    case 2:
    {
        char username[MAX_USERNAME];
        printf("Enter username to delete: ");
        scanf("%s", username);
        clear_input_buffer();

        if (strcmp(username, current_user->username) == 0)
        {
            printf("Error: Cannot delete your own account!\n");
            break;
        }

        User **current = &user_list;
        while (*current != NULL)
        {
            if (strcmp((*current)->username, username) == 0)
            {
                User *temp = *current;
                *current = (*current)->next;
                free(temp);
                save_users_to_file();
                printf("Successfully deleted account %s!\n", username);
                return;
            }
            current = &((*current)->next);
        }
        printf("Account %s not found!\n", username);
        break;
    }

    case 3:
    {
        char username[MAX_USERNAME], new_password[MAX_PASSWORD];
        printf("Enter username to reset password: ");
        scanf("%s", username);
        clear_input_buffer();

        User *user = user_list;
        while (user != NULL)
        {
            if (strcmp(user->username, username) == 0)
            {
                printf("Enter new password: ");
                scanf("%s", new_password);
                clear_input_buffer();

                strcpy(user->password, new_password);
                save_users_to_file();
                printf("Successfully reset password for %s!\n", username);
                return;
            }
            user = user->next;
        }
        printf("Account %s not found!\n", username);
        break;
    }

    default:
        printf("Invalid choice!\n");
        break;
    }
}

// Function 9: Sign out
void sign_out()
{
    printf("\n--- SIGN OUT ---\n");

    if (current_user == NULL)
    {
        printf("You are not logged in!\n");
        return;
    }

    printf("Sign out successful! Goodbye %s!\n", current_user->username);
    current_user = NULL;
}

// Handle user choice
void handle_choice(int choice)
{
    switch (choice)
    {
    case 1:
        register_user();
        break;
    case 2:
        sign_in();
        break;
    case 3:
        change_password();
        break;
    case 4:
        update_account_info();
        break;
    case 5:
        reset_password();
        break;
    case 6:
        view_login_history();
        break;
    case 7:
        auto_unlock_accounts();
        break;
    case 8:
        admin_functions();
        break;
    case 9:
        sign_out();
        break;
    default:
        printf("Thank you for using the program!\n");
        break;
    }
}

// Main function
int main()
{
    printf("Initializing program...\n");

    // Load data from files
    load_users_from_file();
    load_history_from_file();

    // Check and auto unlock accounts
    auto_unlock_accounts();

    int choice;
    do
    {
        display_menu();
        if (scanf("%d", &choice) == 1)
        {
            clear_input_buffer();
            handle_choice(choice);
        }
        else
        {
            clear_input_buffer();
            printf("Error: Please enter a number!\n");
        }
    } while (choice >= 1 && choice <= 9);

    // Free memory and save data
    save_users_to_file();
    save_history_to_file();
    free_user_list();
    free_history_list();

    printf("Program ended.\n");
    return 0;
}