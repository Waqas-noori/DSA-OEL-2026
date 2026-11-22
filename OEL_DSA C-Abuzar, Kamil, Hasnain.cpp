
//Full n Final
#define _CRT_SECURE_NO_WARNINGS   // lets Visual Studio accept localtime()
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cstdio>      // remove() and rename() for safe file saving
#include <cctype>
using namespace std;
const string PRODUCT_FILE     = "products.txt";
const string TRANSACTION_FILE = "transactions.txt";
const int    MIN_ID           = 1;
const int    MAX_ID           = 999999;
const int    MAX_QUANTITY     = 100000;
const double MIN_PRICE        = 0.01;
const double MAX_PRICE        = 1000000.00;
const int    MAX_NAME_LENGTH  = 30;
const int    LOW_STOCK_LIMIT  = 10;     // quantity <= 10 means "low stock"
const int SORT_BY_ID       = 1;
const int SORT_BY_NAME     = 2;
const int SORT_BY_QUANTITY = 3;

//  2. RECORD STRUCTURES  (one node of each linked list)
struct Product {
    int      id;
    string   name;
    int      quantity;
    double   price;
    Product* next;          // link to the next product
};

struct Transaction {
    int          number;        // transaction number 1, 2, 3 ...
    string       type;          // "STOCK-IN" or "STOCK-OUT"
    int          productId;
    string       productName;
    int          quantity;      // units added or removed
    int          balance;       // stock left after the transaction
    string       dateTime;
    Transaction* next;          // link to the next transaction
};

//  3. SMALL HELPER FUNCTIONS (text handling and date/time)

// Remove spaces from the start and end of a string
string trim(const string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && isspace((unsigned char)text[start])) start++;
    while (end > start && isspace((unsigned char)text[end - 1])) end--;
    return text.substr(start, end - start);
}

// Convert a string to lower case (used for case-insensitive search and sort)
string toLower(string text) {
    for (size_t i = 0; i < text.size(); i++)
        text[i] = (char)tolower((unsigned char)text[i]);
    return text;
}

// Convert an integer to a string
string toText(int number) {
    ostringstream out;
    out << number;
    return out.str();
}

// Current date and time like "2026-11-19 14:30:05"
string getCurrentDateTime() {
    time_t now = time(0);
    tm* t = localtime(&now);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    return string(buffer);
}
//  4. LINKED LIST OF PRODUCTS
class ProductList {
private:
    Product* head;      // first product of the list
    int      count;     // number of products in the list

    // The list owns its nodes, so copying it is forbidden (a copy would share
    // the same nodes and they would be deleted twice). Declared, never defined.
    ProductList(const ProductList&);
    ProductList& operator=(const ProductList&);

    // Swap the DATA of two nodes (the links stay as they are)
    static void swapData(Product* a, Product* b) {
        int    tempId       = a->id;        a->id       = b->id;        b->id       = tempId;
        string tempName     = a->name;      a->name     = b->name;      b->name     = tempName;
        int    tempQuantity = a->quantity;  a->quantity = b->quantity;  b->quantity = tempQuantity;
        double tempPrice    = a->price;     a->price    = b->price;     b->price    = tempPrice;
    }

    // Returns true if node 'a' must come AFTER node 'b' in the chosen order
    static bool isOutOfOrder(const Product* a, const Product* b, int field, bool ascending) {
        int result;     // > 0 means a is bigger, < 0 means a is smaller, 0 means equal
        if (field == SORT_BY_ID)
            result = a->id - b->id;
        else if (field == SORT_BY_NAME)
            result = toLower(a->name).compare(toLower(b->name));
        else
            result = a->quantity - b->quantity;

        if (ascending) return result > 0;
        else           return result < 0;
    }

public:
    ProductList() : head(NULL), count(0) {}

    // Destructor: free every node so there is no memory leak
    ~ProductList() {
        while (head != NULL) {
            Product* temp = head;
            head = head->next;
            delete temp;
        }
    }

    Product* getHead() const { return head; }
    int  getCount()    const { return count; }
    bool isEmpty()     const { return head == NULL; }

    // INSERT: add a new product at the END of the list
    void addProduct(int id, const string& name, int quantity, double price) {
        Product* node = new Product;
        node->id       = id;
        node->name     = name;
        node->quantity = quantity;
        node->price    = price;
        node->next     = NULL;

        if (head == NULL) {
            head = node;
        } else {
            Product* current = head;
            while (current->next != NULL)
                current = current->next;
            current->next = node;
        }
        count++;
    }

    // LINEAR SEARCH: look at every node from the head until the ID matches.
    // Returns the node, or NULL when the ID does not exist.
    Product* findById(int id) const {
        Product* current = head;
        while (current != NULL) {
            if (current->id == id)
                return current;
            current = current->next;
        }
        return NULL;
    }

    // DELETE: unlink the node with this ID and free it.
    // Returns false when the ID does not exist.
    bool removeProduct(int id) {
        Product* previous = NULL;
        Product* current  = head;

        while (current != NULL && current->id != id) {
            previous = current;
            current  = current->next;
        }
        if (current == NULL)
            return false;                   // not found

        if (previous == NULL)
            head = current->next;           // deleting the first node
        else
            previous->next = current->next; // deleting a middle / last node

        delete current;
        count--;
        return true;
    }

    // BUBBLE SORT: repeatedly compare neighbouring nodes and swap them
    // when they are in the wrong order. After each pass the biggest
    // (or smallest) value moves to the end, so the list gets shorter.
    void sortBy(int field, bool ascending) {
        if (head == NULL || head->next == NULL)
            return;                         // 0 or 1 product: nothing to sort

        bool     swapped;
        Product* end = NULL;                // nodes from 'end' onward are sorted

        do {
            swapped = false;
            Product* current = head;
            while (current->next != end) {
                if (isOutOfOrder(current, current->next, field, ascending)) {
                    swapData(current, current->next);
                    swapped = true;
                }
                current = current->next;
            }
            end = current;                  // last node of this pass is now in place
        } while (swapped);                  // stop when a pass makes no swap
    }
};

//  5. LINKED LIST OF TRANSACTIONS
class TransactionList {
private:
    Transaction* head;
    Transaction* tail;          // remembering the tail makes adding fast
    int          count;
    int          nextNumber;    // number given to the next new transaction

    // Copying is forbidden for the same reason as in ProductList
    TransactionList(const TransactionList&);
    TransactionList& operator=(const TransactionList&);

public:
    TransactionList() : head(NULL), tail(NULL), count(0), nextNumber(1) {}

    ~TransactionList() {
        while (head != NULL) {
            Transaction* temp = head;
            head = head->next;
            delete temp;
        }
    }

    Transaction* getHead() const { return head; }
    int getCount()         const { return count; }

    // Add a transaction at the END of the list (used also when loading from file)
    void append(int number, const string& type, int productId, const string& productName,
                int quantity, int balance, const string& dateTime) {
        Transaction* node = new Transaction;
        node->number      = number;
        node->type        = type;
        node->productId   = productId;
        node->productName = productName;
        node->quantity    = quantity;
        node->balance     = balance;
        node->dateTime    = dateTime;
        node->next        = NULL;

        if (tail == NULL) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        count++;

        if (number >= nextNumber)
            nextNumber = number + 1;
    }

    // Record a NEW transaction (automatic number and current date/time)
    void record(const string& type, int productId, const string& productName,
                int quantity, int balance) {
        append(nextNumber, type, productId, productName, quantity, balance, getCurrentDateTime());
    }
};

//  6. INPUT FUNCTIONS (every input is read as text and checked before use)

// Read one line from the keyboard
string readLine(const string& prompt) {
    string line;
    cout << prompt;
    if (!getline(cin, line)) {                      // keyboard input has ended
        cout << "\nNo more input. Closing the program." << endl;
        exit(0);                                    // data is already saved
    }
    return trim(line);
}

// Convert text to a whole number. Returns false if the text is not a number.
bool parseInt(const string& text, int& value) {
    if (text.empty() || text.size() > 9)            // 9 characters can never overflow int
        return false;

    size_t i = 0;
    bool negative = false;
    if (text[0] == '-') {
        negative = true;
        i = 1;
    }
    if (i == text.size())                           // only a '-' sign
        return false;

    int result = 0;
    for (; i < text.size(); i++) {
        if (!isdigit((unsigned char)text[i]))
            return false;
        result = result * 10 + (text[i] - '0');
    }
    value = negative ? -result : result;
    return true;
}

// Convert text like "25.50" to a price. Returns false if the text is invalid.
// Only digits and ONE decimal point are allowed, with at most 2 decimal places.
bool parsePrice(const string& text, double& value) {
    if (text.empty() || text.size() > 12)
        return false;

    int dots = 0, digits = 0, decimals = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '.') {
            dots++;
        } else if (isdigit((unsigned char)text[i])) {
            digits++;
            if (dots == 1)
                decimals++;                         // digit after the decimal point
        } else {
            return false;                           // letters, symbols, '-' are not allowed
        }
    }
    if (dots > 1 || digits == 0 || decimals > 2)
        return false;

    value = atof(text.c_str());
    return true;
}

// Keep asking until the user enters a whole number between minValue and maxValue
int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        string line = readLine(prompt);
        int value;
        if (parseInt(line, value) && value >= minValue && value <= maxValue)
            return value;
        cout << "  Invalid input! Enter a whole number from " << minValue
             << " to " << maxValue << "." << endl;
    }
}

// Keep asking until the user enters a valid price
double readPrice(const string& prompt) {
    while (true) {
        string line = readLine(prompt);
        double value;
        if (parsePrice(line, value) && value >= MIN_PRICE && value <= MAX_PRICE)
            return value;
        cout << "  Invalid input! Enter a price from " << MIN_PRICE
             << " to " << MAX_PRICE << " (numbers only, at most 2 decimal places)." << endl;
    }
}

// Keep asking until the user enters a valid product name
string readName(const string& prompt) {
    while (true) {
        string name = readLine(prompt);
        if (name.empty())
            cout << "  Invalid input! Name cannot be empty." << endl;
        else if ((int)name.size() > MAX_NAME_LENGTH)
            cout << "  Invalid input! Name can have at most " << MAX_NAME_LENGTH
                 << " characters." << endl;
        else if (name.find('|') != string::npos)
            cout << "  Invalid input! Name cannot contain the '|' character." << endl;
        else
            return name;
    }
}

// Ask a yes/no question
bool readYesNo(const string& prompt) {
    while (true) {
        string answer = toLower(readLine(prompt));
        if (answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no")  return false;
        cout << "  Please enter y or n." << endl;
    }
}

//  7. DISPLAY FUNCTIONS
void printLine(int length) {
    cout << string(length, '-') << endl;
}

// Stock status of a product
string getStatus(int quantity) {
    if (quantity == 0)                return "OUT OF STOCK";
    if (quantity <= LOW_STOCK_LIMIT)  return "LOW STOCK";
    return "In Stock";
}

void printProductHeader() {
    cout << left << setw(8) << "ID" << setw(32) << "Name" << setw(10) << "Qty"
         << setw(12) << "Price" << setw(18) << "Value" << "Status" << endl;
    printLine(92);
}

void printProductRow(const Product* p) {
    double value = p->quantity * p->price;
    cout << left << setw(8) << p->id << setw(32) << p->name << setw(10) << p->quantity
         << setw(12) << p->price << setw(18) << value << getStatus(p->quantity) << endl;
}

void printTransactionHeader() {
    cout << left << setw(6) << "No." << setw(21) << "Date & Time" << setw(11) << "Type"
         << setw(8) << "ID" << setw(32) << "Product Name" << setw(8) << "Qty"
         << "Balance" << endl;
    printLine(94);
}

void printTransactionRow(const Transaction* t) {
    cout << left << setw(6) << t->number << setw(21) << t->dateTime << setw(11) << t->type
         << setw(8) << t->productId << setw(32) << t->productName << setw(8) << t->quantity
         << t->balance << endl;
}

//  8. FILE HANDLING (permanent storage)
//     products.txt     : id|name|quantity|price
//     transactions.txt : number|type|productId|productName|quantity|balance|dateTime

// Split a line at every '|' character. Returns the number of fields found.
int splitLine(const string& line, string fields[], int maxFields) {
    int count = 0;
    string part;
    stringstream stream(line);
    while (count < maxFields && getline(stream, part, '|'))
        fields[count++] = part;
    return count;
}

// Replace a real file by its finished temporary file.
// (Windows cannot rename over an existing file, so the old one is removed first.)
bool replaceFile(const string& tempName, const string& realName) {
    remove(realName.c_str());
    return rename(tempName.c_str(), realName.c_str()) == 0;
}

// Write all products and all transactions to the files.
// SAFE SAVING: everything is written to temporary files first. The real files
// are replaced only after BOTH temporary files were written without any error,
// so a failed save can never destroy the data already stored in the real files.
bool saveAll(const ProductList& inventory, const TransactionList& history) {
    const string productTemp     = PRODUCT_FILE + ".tmp";
    const string transactionTemp = TRANSACTION_FILE + ".tmp";
    bool productsWritten     = false;
    bool transactionsWritten = false;

    ofstream productOut(productTemp.c_str());
    if (productOut) {
        productOut << fixed << setprecision(2);
        for (Product* p = inventory.getHead(); p != NULL; p = p->next)
            productOut << p->id << '|' << p->name << '|' << p->quantity << '|' << p->price << '\n';
        productOut.close();
        productsWritten = !productOut.fail();       // false if any write failed
    }

    ofstream transactionOut(transactionTemp.c_str());
    if (transactionOut) {
        for (Transaction* t = history.getHead(); t != NULL; t = t->next)
            transactionOut << t->number << '|' << t->type << '|' << t->productId << '|'
                           << t->productName << '|' << t->quantity << '|' << t->balance << '|'
                           << t->dateTime << '\n';
        transactionOut.close();
        transactionsWritten = !transactionOut.fail();
    }

    if (!productsWritten || !transactionsWritten) {
        remove(productTemp.c_str());                // throw the unfinished files away
        remove(transactionTemp.c_str());
        cout << "ERROR: Could not write the data files!" << endl;
        return false;
    }

    bool productsReplaced     = replaceFile(productTemp, PRODUCT_FILE);
    bool transactionsReplaced = replaceFile(transactionTemp, TRANSACTION_FILE);
    if (!productsReplaced || !transactionsReplaced) {
        cout << "ERROR: Could not replace the old data files!" << endl;
        return false;
    }
    return true;
}

// Save and tell the user whether it worked (used after every change)
void saveChanges(const ProductList& inventory, const TransactionList& history) {
    if (saveAll(inventory, history))
        cout << "Changes saved to file." << endl;
    else
        cout << "WARNING: Changes could NOT be saved to file! "
             << "They will be lost when the program closes." << endl;
}

// Read the files (if they exist) when the program starts.
void loadAll(ProductList& inventory, TransactionList& history) {
    int skipped = 0;
    string line;
    string fields[10];

    // ---- load products ----
    ifstream productIn(PRODUCT_FILE.c_str());
    if (productIn) {
        while (getline(productIn, line)) {
            line = trim(line);
            if (line.empty()) continue;

            int id, quantity;
            double price;
            if (splitLine(line, fields, 10) == 4
                && parseInt(fields[0], id)       && id >= MIN_ID && id <= MAX_ID
                && !fields[1].empty()            && (int)fields[1].size() <= MAX_NAME_LENGTH
                && parseInt(fields[2], quantity) && quantity >= 0 && quantity <= MAX_QUANTITY
                && parsePrice(fields[3], price)  && price >= MIN_PRICE && price <= MAX_PRICE
                && inventory.findById(id) == NULL) {
                inventory.addProduct(id, fields[1], quantity, price);
            } else {
                skipped++;          // damaged line or duplicate ID
            }
        }
    }

    // ---- load transactions ----
    ifstream transactionIn(TRANSACTION_FILE.c_str());
    if (transactionIn) {
        while (getline(transactionIn, line)) {
            line = trim(line);
            if (line.empty()) continue;

            int number, productId, quantity, balance;
            if (splitLine(line, fields, 10) == 7
                && parseInt(fields[0], number)
                && (fields[1] == "STOCK-IN" || fields[1] == "STOCK-OUT")
                && parseInt(fields[2], productId)
                && parseInt(fields[4], quantity)
                && parseInt(fields[5], balance)) {
                history.append(number, fields[1], productId, fields[3], quantity, balance, fields[6]);
            } else {
                skipped++;
            }
        }
    }

    if (inventory.isEmpty() && history.getCount() == 0 && skipped == 0)
        cout << "No saved data found. Starting with an empty inventory." << endl;
    else
        cout << "Saved data loaded: " << inventory.getCount() << " product(s), "
             << history.getCount() << " transaction(s)." << endl;
    if (skipped > 0)
        cout << "Warning: " << skipped << " damaged line(s) in the files were ignored." << endl;
}

//  9. PRODUCT MANAGEMENT MODULE

// ---- INSERT ----
void addProduct(ProductList& inventory, TransactionList& history) {
    cout << "\n========== ADD NEW PRODUCT ==========" << endl;

    int id = readInt("Enter Product ID (" + toText(MIN_ID) + "-" + toText(MAX_ID) + "): ",
                     MIN_ID, MAX_ID);

    // Duplicate check: product IDs must be unique
    if (inventory.findById(id) != NULL) {
        cout << "Error: Product ID " << id << " already exists. Product not added." << endl;
        return;
    }

    string name     = readName("Enter Product Name: ");
    int    quantity = readInt("Enter Opening Quantity (0-" + toText(MAX_QUANTITY) + "): ",
                              0, MAX_QUANTITY);
    double price    = readPrice("Enter Unit Price: ");

    inventory.addProduct(id, name, quantity, price);
    cout << "Product added successfully." << endl;

    // Opening stock is also recorded, so the history explains every unit in stock
    if (quantity > 0) {
        history.record("STOCK-IN", id, name, quantity, quantity);
        cout << "Opening stock of " << quantity << " recorded as a STOCK-IN transaction." << endl;
    }
    saveChanges(inventory, history);
}

// ---- UPDATE ----
void updateProduct(ProductList& inventory, TransactionList& history) {
    cout << "\n========== UPDATE PRODUCT ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    int id = readInt("Enter Product ID to update: ", MIN_ID, MAX_ID);
    Product* p = inventory.findById(id);
    if (p == NULL) {
        cout << "Error: Product ID " << id << " not found." << endl;
        return;
    }

    cout << "Current record:" << endl;
    printProductHeader();
    printProductRow(p);

    cout << "\nWhat do you want to update?" << endl;
    cout << "  1. Name" << endl;
    cout << "  2. Price" << endl;
    cout << "  3. Quantity" << endl;
    cout << "  0. Cancel" << endl;
    int choice = readInt("Enter choice: ", 0, 3);

    if (choice == 1) {
        p->name = readName("Enter new name: ");
        cout << "Name updated successfully." << endl;
    }
    else if (choice == 2) {
        p->price = readPrice("Enter new price: ");
        cout << "Price updated successfully." << endl;
    }
    else if (choice == 3) {
        int newQuantity = readInt("Enter new quantity (0-" + toText(MAX_QUANTITY) + "): ",
                                  0, MAX_QUANTITY);
        if (newQuantity == p->quantity) {
            cout << "Quantity is unchanged." << endl;
            return;
        } else {
            // A change of quantity is saved in the transaction history too,
            // so the history always matches the stock.
            int difference = newQuantity - p->quantity;
            if (difference > 0)
                history.record("STOCK-IN", p->id, p->name, difference, newQuantity);
            else
                history.record("STOCK-OUT", p->id, p->name, -difference, newQuantity);
            p->quantity = newQuantity;
            cout << "Quantity updated successfully (recorded in transaction history)." << endl;
        }
    }
    else {
        cout << "Update cancelled." << endl;
        return;
    }

    saveChanges(inventory, history);
}

// ---- DELETE ----
void deleteProduct(ProductList& inventory, const TransactionList& history) {
    cout << "\n========== DELETE PRODUCT ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    int id = readInt("Enter Product ID to delete: ", MIN_ID, MAX_ID);
    Product* p = inventory.findById(id);
    if (p == NULL) {
        cout << "Error: Product ID " << id << " not found." << endl;
        return;
    }

    printProductHeader();
    printProductRow(p);
    if (readYesNo("Are you sure you want to delete this product? (y/n): ")) {
        inventory.removeProduct(id);
        cout << "Product deleted successfully." << endl;
        saveChanges(inventory, history);
    } else {
        cout << "Delete cancelled." << endl;
    }
}

// ---- SEARCH ----
void searchProduct(const ProductList& inventory) {
    cout << "\n========== SEARCH PRODUCT ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    cout << "  1. Search by Product ID" << endl;
    cout << "  2. Search by Product Name" << endl;
    cout << "  3. Search by Quantity" << endl;
    int choice = readInt("Enter choice: ", 1, 3);

    if (choice == 1) {
        int id = readInt("Enter Product ID: ", MIN_ID, MAX_ID);
        Product* p = inventory.findById(id);            // linear search by ID
        if (p == NULL) {
            cout << "Product with ID " << id << " not found." << endl;
        } else {
            printProductHeader();
            printProductRow(p);
        }
    }
    else if (choice == 2) {
        string keyword = readLine("Enter name (or part of the name): ");
        if (keyword.empty()) {
            cout << "Error: Search text cannot be empty." << endl;
            return;
        }

        string key = toLower(keyword);
        int found = 0;
        // linear search by name: check every product
        for (Product* p = inventory.getHead(); p != NULL; p = p->next) {
            if (toLower(p->name).find(key) != string::npos) {
                if (found == 0)
                    printProductHeader();
                printProductRow(p);
                found++;
            }
        }
        if (found == 0)
            cout << "No product found with the name \"" << keyword << "\"." << endl;
        else
            cout << found << " product(s) found." << endl;
    }
    else {
        int quantity = readInt("Enter quantity to search (0-" + toText(MAX_QUANTITY) + "): ",
                               0, MAX_QUANTITY);
        int found = 0;
        // linear search by quantity: several products can have the same quantity
        for (Product* p = inventory.getHead(); p != NULL; p = p->next) {
            if (p->quantity == quantity) {
                if (found == 0)
                    printProductHeader();
                printProductRow(p);
                found++;
            }
        }
        if (found == 0)
            cout << "No product found with quantity " << quantity << "." << endl;
        else
            cout << found << " product(s) found." << endl;
    }
}

// ---- DISPLAY ALL ----
void displayAll(const ProductList& inventory) {
    cout << "\n========== ALL PRODUCTS ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }
    printProductHeader();
    for (Product* p = inventory.getHead(); p != NULL; p = p->next)
        printProductRow(p);
    cout << "Total products: " << inventory.getCount() << endl;
}

// ---- SORT ----
void sortInventory(ProductList& inventory, const TransactionList& history) {
    cout << "\n========== SORT INVENTORY ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    cout << "  1. Sort by Product ID" << endl;
    cout << "  2. Sort by Product Name" << endl;
    cout << "  3. Sort by Quantity" << endl;
    int field = readInt("Enter choice: ", 1, 3);

    cout << "  1. Ascending  (small to large / A to Z)" << endl;
    cout << "  2. Descending (large to small / Z to A)" << endl;
    int order = readInt("Enter choice: ", 1, 2);

    inventory.sortBy(field, order == 1);                // bubble sort
    cout << "Inventory sorted successfully." << endl;
    saveChanges(inventory, history);                    // keep the new order in the file
    displayAll(inventory);
}

//  10. STOCK TRANSACTION MODULE

// ---- STOCK-IN (goods received) ----
void stockIn(ProductList& inventory, TransactionList& history) {
    cout << "\n========== STOCK-IN ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    int id = readInt("Enter Product ID: ", MIN_ID, MAX_ID);
    Product* p = inventory.findById(id);
    if (p == NULL) {
        cout << "Error: Product ID " << id << " not found." << endl;
        return;
    }
    cout << "Product: " << p->name << " | Current stock: " << p->quantity << endl;

    int quantity = readInt("Enter quantity to add (1-" + toText(MAX_QUANTITY) + "): ",
                           1, MAX_QUANTITY);

    // Maximum stock check
    if (p->quantity + quantity > MAX_QUANTITY) {
        cout << "Error: Stock would exceed the maximum limit of " << MAX_QUANTITY
             << ". You can add at most " << (MAX_QUANTITY - p->quantity) << " more." << endl;
        return;
    }

    p->quantity += quantity;
    history.record("STOCK-IN", p->id, p->name, quantity, p->quantity);
    cout << "Stock-in successful. New stock of " << p->name << ": " << p->quantity << endl;
    saveChanges(inventory, history);
}

// ---- STOCK-OUT (goods issued / sold) ----
void stockOut(ProductList& inventory, TransactionList& history) {
    cout << "\n========== STOCK-OUT ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    int id = readInt("Enter Product ID: ", MIN_ID, MAX_ID);
    Product* p = inventory.findById(id);
    if (p == NULL) {
        cout << "Error: Product ID " << id << " not found." << endl;
        return;
    }
    cout << "Product: " << p->name << " | Available stock: " << p->quantity << endl;

    // Zero stock check
    if (p->quantity == 0) {
        cout << "Error: Zero stock! There is nothing to issue for this product." << endl;
        return;
    }

    int quantity = readInt("Enter quantity to issue (1-" + toText(MAX_QUANTITY) + "): ",
                           1, MAX_QUANTITY);

    // Insufficient stock check
    if (quantity > p->quantity) {
        cout << "Error: Insufficient stock! Requested " << quantity
             << " but only " << p->quantity << " available. Stock-out cancelled." << endl;
        return;
    }

    p->quantity -= quantity;
    history.record("STOCK-OUT", p->id, p->name, quantity, p->quantity);
    cout << "Stock-out successful. Remaining stock of " << p->name << ": " << p->quantity << endl;

    if (p->quantity == 0)
        cout << "WARNING: " << p->name << " is now OUT OF STOCK." << endl;
    else if (p->quantity <= LOW_STOCK_LIMIT)
        cout << "WARNING: " << p->name << " is now LOW on stock." << endl;
    saveChanges(inventory, history);
}

// ---- TRANSACTION HISTORY ----
void viewTransactions(const TransactionList& history) {
    cout << "\n========== TRANSACTION HISTORY ==========" << endl;
    if (history.getCount() == 0) {
        cout << "No transactions recorded yet." << endl;
        return;
    }

    cout << "  1. Show all transactions" << endl;
    cout << "  2. Show transactions of one product" << endl;
    int choice = readInt("Enter choice: ", 1, 2);

    int filterId = 0;
    if (choice == 2)
        filterId = readInt("Enter Product ID: ", MIN_ID, MAX_ID);

    int shown = 0;
    for (Transaction* t = history.getHead(); t != NULL; t = t->next) {
        if (choice == 1 || t->productId == filterId) {
            if (shown == 0)
                printTransactionHeader();
            printTransactionRow(t);
            shown++;
        }
    }

    if (shown == 0)
        cout << "No transactions found for Product ID " << filterId << "." << endl;
    else
        cout << "Records shown: " << shown << endl;
}

//  11. REPORT MODULE

// ---- FULL INVENTORY REPORT ----
void inventoryReport(const ProductList& inventory) {
    cout << "\n========== INVENTORY REPORT ==========" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    int    totalUnits = 0, lowCount = 0, outCount = 0;
    double totalValue = 0;

    printProductHeader();
    for (Product* p = inventory.getHead(); p != NULL; p = p->next) {
        printProductRow(p);
        totalUnits += p->quantity;
        totalValue += p->quantity * p->price;
        if (p->quantity == 0)                      outCount++;
        else if (p->quantity <= LOW_STOCK_LIMIT)   lowCount++;
    }

    printLine(92);
    cout << "Total products          : " << inventory.getCount() << endl;
    cout << "Total units in stock    : " << totalUnits << endl;
    cout << "Total inventory value   : " << totalValue << endl;
    cout << "Low-stock products      : " << lowCount << "  (quantity 1 to " << LOW_STOCK_LIMIT << ")" << endl;
    cout << "Out-of-stock products   : " << outCount << endl;
}

// ---- LOW-STOCK REPORT ----
void lowStockReport(const ProductList& inventory) {
    cout << "\n========== LOW-STOCK REPORT ==========" << endl;
    cout << "(Products with quantity of " << LOW_STOCK_LIMIT << " or less)" << endl;
    if (inventory.isEmpty()) {
        cout << "No products in the inventory." << endl;
        return;
    }

    int found = 0;
    for (Product* p = inventory.getHead(); p != NULL; p = p->next) {
        if (p->quantity <= LOW_STOCK_LIMIT) {
            if (found == 0)
                printProductHeader();
            printProductRow(p);
            found++;
        }
    }

    if (found == 0)
        cout << "Good news: no product is low on stock." << endl;
    else
        cout << found << " product(s) need to be restocked." << endl;
}

//  12. MAIN MENU
void showMenu() {
    cout << "\n                     MAIN MENU                     " << endl;
    cout << "   1. Add New Product" << endl;
    cout << "   2. Update Product" << endl;
    cout << "   3. Delete Product" << endl;
    cout << "   4. Search Product" << endl;
    cout << "   5. Display All Products" << endl;
    cout << "   6. Sort Inventory" << endl;
    cout << "   7. Stock-In  (receive stock)" << endl;
    cout << "   8. Stock-Out (issue stock)" << endl;
    cout << "   9. View Transaction History" << endl;
    cout << "  10. Inventory Report" << endl;
    cout << "  11. Low-Stock Report" << endl;
    cout << "   0. Save and Exit" << endl;
    cout << "                                                     " << endl;
}

int main() {
    cout << fixed << setprecision(2);       // show prices with 2 decimal places

    ProductList     inventory;              // all products (linked list)
    TransactionList history;                // all transactions (linked list)

    cout << "                                                       " << endl;
    cout << "     INVENTORY AND STOCK MANAGEMENT SYSTEM" << endl;
    cout << "                                                       " << endl;

    loadAll(inventory, history);            // read the saved data from the files

    int choice;
    do {
        showMenu();
        choice = readInt("Enter your choice (0-11): ", 0, 11);

        switch (choice) {
            case 1:  addProduct(inventory, history);      break;
            case 2:  updateProduct(inventory, history);   break;
            case 3:  deleteProduct(inventory, history);   break;
            case 4:  searchProduct(inventory);            break;
            case 5:  displayAll(inventory);               break;
            case 6:  sortInventory(inventory, history);   break;
            case 7:  stockIn(inventory, history);         break;
            case 8:  stockOut(inventory, history);        break;
            case 9:  viewTransactions(history);           break;
            case 10: inventoryReport(inventory);          break;
            case 11: lowStockReport(inventory);           break;
            case 0:
                if (saveAll(inventory, history))
                    cout << "\nAll data saved to " << PRODUCT_FILE << " and "
                         << TRANSACTION_FILE << ". Goodbye!" << endl;
                else
                    cout << "\nWARNING: Data could not be saved. Goodbye!" << endl;
                break;
        }
    } while (choice != 0);

    return 0;
}
