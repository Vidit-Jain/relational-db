#include "cursor.h"

enum IndexingStrategy
{
    BTREE,
    HASH,
    NOTHING
};

/**
 * @brief The Table class holds all information related to a loaded table. It
 * also implements methods that interact with the parsers, executors, cursors
 * and the buffer manager. There are typically 2 ways a table object gets
 * created through the course of the workflow - the first is by using the LOAD
 * command and the second is to use assignment statements (SELECT, PROJECT,
 * JOIN, SORT, CROSS and DISTINCT). 
 *
 */
class Table
{
    vector<unordered_set<int>> distinctValuesInColumns;

public:
    string sourceFileName = "";
    string tableName = "";
    vector<string> columns;
    vector<uint> distinctValuesPerColumnCount;
    uint columnCount = 0;
    long long int rowCount = 0;
    uint blockCount = 0;
    uint maxRowsPerBlock = 0;
    vector<uint> rowsPerBlockCount;
    bool indexed = false;
    string indexedColumn = "";
    IndexingStrategy indexingStrategy = NOTHING;
    map<string, int> colNameToIdx;

    bool extractColumnNames(string firstLine);
    bool blockify();
    void updateStatistics(vector<int> row);
    Table();
    Table(string tableName);
    Table(string tableName, Table *originalTable);
    Table(string tableName, vector<string> columns);
    bool load();
    bool isColumn(string columnName);
    void renameColumn(string fromColumnName, string toColumnName);
    void print();
    void makePermanent();
    bool isPermanent();
    void getNextPage(Cursor *cursor);
    Cursor getCursor();
    int getColumnIndex(string columnName);
    vector<int> getColumnIndex(const vector<string> &columnNames);
    void unload();
    void sort(const vector<string> &colNames, const vector<int> &colMultipliers, const string& originalTableName);
    void sort(const std::string &colName, int colMultiplier, const string& originalTableName);
    void sortingPhase(const vector<int> &colIndices, const vector<int> &colMultipliers, const string& originalTableName);
    void mergingPhase(const vector<int> &colIndices, const vector<int> &colMultipliers);
    void rename(const string &newName);

    /**
 * @brief Static function that takes a vector of valued and prints them out in a
 * comma seperated format.
 *
 * @tparam T current usaages include int and string
 * @param row 
 */
template <typename T>
void writeRow(vector<T> row, ostream &fout)
{
    logger.log("Table::printRow");
    for (int columnCounter = 0; columnCounter < row.size(); columnCounter++)
    {
        if (columnCounter != 0)
            fout << ", ";
        fout << row[columnCounter];
    }
    fout << endl;
}

/**
 * @brief Static function that takes a vector of valued and prints them out in a
 * comma seperated format.
 *
 * @tparam T current usaages include int and string
 * @param row 
 */
template <typename T>
void writeRow(vector<T> row)
{
    logger.log("Table::printRow");
    ofstream fout(this->sourceFileName, ios::app);
    this->writeRow(row, fout);
    fout.close();
}

/**
 * @brief Static function that takes a vector of valued and prints them out in a
 * comma seperated format.
 *
 * @tparam T current usaages include int and string
 * @param row
 */
    template <typename T>
    void writeRows(const vector<vector<T>>& rows, const int rowCount)
    {
        logger.log("Table::printRow");
        ofstream fout(this->sourceFileName, ios::app);
        for (int i = 0; i < rowCount; i++) this->writeRow(rows[i], fout);
        fout.close();
    }
};