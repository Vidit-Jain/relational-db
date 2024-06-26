#include "global.h"

/**
 * @brief Construct a new Matrix::Matrix object
 *
 */
Matrix::Matrix()
{
    logger.log("Matrix::Matrix");
}

/**
 * @brief Construct a new Matrix::Matrix object used in the case where the data
 * file is available and LOAD command has been called. This command should be
 * followed by calling the load function;
 *
 * @param matrixName
 */
Matrix::Matrix(string matrixName)
{
    logger.log("Matrix::Matrix");
    this->sourceFileName = "../data/" + matrixName + ".csv";
    this->matrixName = this->originalMatrixName = matrixName;
}

/**
 * @brief Construct a new Matrix::Matrix object used when an assignment command
 * is encountered. To create the Matrix object both the matrix name and the
 * dimensions of the matrix should be specified
 *
 * @param matrixName
 * @param dimension
 */
Matrix::Matrix(string matrixName, Matrix* originalMatrix)
{
    logger.log("Matrix::Matrix");
    this->sourceFileName = "../data/temp/" + matrixName + ".csv";
    this->matrixName = matrixName;
    this->originalMatrixName = matrixName;
    this->dimension = originalMatrix->dimension;
    this->blockCount = originalMatrix->blockCount;
    this->symmetric = originalMatrix->symmetric;
    this->m = originalMatrix->m;
    this->concurrentBlocks = originalMatrix->concurrentBlocks;
    this->dimsPerBlock = originalMatrix->dimsPerBlock;
}

/**
 * @brief The load function is used when the LOAD command is encountered. It
 * reads data from the source file, splits it into blocks.
 *
 * @return true if the matrix has been successfully loaded
 * @return false if an error occurred
 */
bool Matrix::load()
{
    logger.log("Matrix::load");
    if (this->extractDimension(this->sourceFileName)) {
        if (this->blockify())
            return true;
    }
    return false;
}

/**
 * Opens fileName, and computes N, the dimension of the matrix being loaded
 * @param fileName
 * @return true if it successfully reads a line to determine dimension
 * @return false otherwise, indicates failure and matrix won't be loaded
 */
bool Matrix::extractDimension(string fileName)
{
    logger.log("Matrix::extractDimension");
    fstream fin(fileName, ios::in);
    string line;
    if (getline(fin, line)) {
        this->dimension = std::count(line.begin(), line.end(), ',') + 1;
        fin.close();
        return true;
    }
    fin.close();
    return false;
}

/**
 * @brief Calculated the largest M x M submatrix that can be stored in a block,
 * and the number of blocks that will be in each row.
 * @return true if block size is big enough to store an integer
 * @return false otherwise, indicating it can't be blockified
 */
bool Matrix::blockDimensions() {
    int totalIntegers = (BLOCK_SIZE * 1000) / (sizeof(int));
    int c = sqrt(totalIntegers);
    // avoiding precision issues
    while ((c + 1) * (c + 1) <= totalIntegers) c++;
    while (c * c > totalIntegers) c--;
    if (c == 0) return false;
    this->m = c;
    this->concurrentBlocks = (this->dimension + this->m - 1) / this->m;
    return true;
}

/**
 * @brief This function splits all the rows and stores them in multiple files of
 * one block size.
 *
 * @return true if successfully blockified
 * @return false otherwise
 */
bool Matrix::blockify() {
    logger.log("Matrix::blockify");
    ifstream fin(this->sourceFileName, ios::in);
    if (!blockDimensions()) return false;

    vector<vector<vector<int>>> grids(concurrentBlocks, \
                              vector<vector<int>>(m, \
                                      vector<int>(m)));
    string line, word;
    int rowIndex = 0, rowsRead = 0;
    function<void()> writeToBuffer = [&] () {
        for (int i = 0; i < concurrentBlocks; i++) {
            int colSize = (i == concurrentBlocks - 1 && this->dimension % m) ? (this->dimension % m) : m;
            bufferManager.writePage(this->matrixName, this->blockCount, grids[i], rowIndex, colSize);
            this->blockCount++;
            this->dimsPerBlock.emplace_back(rowIndex, colSize);
        }
        rowIndex = 0;
    };
    while (getline(fin, line))
    {
        stringstream s(line);
        for (int columnCounter = 0; columnCounter < this->dimension; columnCounter++) {
            if (!getline(s, word, ',')) return false;
            grids[columnCounter / m][rowIndex][columnCounter % m] = stoi(word);
        }
        rowIndex++, rowsRead++;
        if (rowIndex == m) writeToBuffer();
    }
    if (rowIndex) writeToBuffer();
    if (rowsRead != 0) return true;
    return false;
}

/**
 * @brief Function prints the first few rows and columns of the matrix. If the matrix contains
 * more rows than PRINT_COUNT, exactly PRINT_COUNT rows and columns are printed, else all
 * the rows and columns are printed.
 */
void Matrix::print()
{
    uint count = min((long long)PRINT_COUNT, this->dimension);
    vector<vector<int>> mat(count, vector<int>(count));

    Cursor cursor(this->matrixName, 0, MATRIX);
    int rows = (count + m - 1) / m;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < rows; j++) {
            cursor.nextPage(i * concurrentBlocks + j);

            for (int k = 0; k < min(m, (int)count - i * m); k++) {
                vector<int> row = cursor.getNext();
                for (int l = 0; l < min(m, (int)count - j * m); l++) {
                    mat[i * m + k][j * m + l] = row[l];
                }
            }
        }
    }
    writeRows(mat, count, cout);
    printRowCount(this->dimension);
}

/**
 * @brief This function returns one row of the matrix using the cursor object. It
 * returns an empty row is all rows have been read.
 *
 * @param cursor
 * @return vector<int>
 */
void Matrix::getNextPage(Cursor *cursor)
{
    logger.log("Matrix::getNext");

    if (cursor->pageIndex < this->blockCount - 1)
    {
        cursor->nextPage(cursor->pageIndex+1);
    }
}

/**
 * @brief called when EXPORT command is invoked to move source file to "data"
 * folder.
 */
void Matrix::makePermanent()
{
    logger.log("Matrix::makePermanent");
    if(!this->isPermanent())
        bufferManager.deleteFile(this->sourceFileName);
    string newSourceFile = "../data/" + this->matrixName + ".csv";
    ofstream fout(newSourceFile, ios::out);

    vector<vector<int>> mat(m, vector<int>(this->dimension));
    Cursor cursor(this->matrixName, 0, MATRIX);
    for (int i = 0; i < concurrentBlocks; i++) {
        for (int j = 0; j < concurrentBlocks; j++) {
            cursor.nextPage(i * concurrentBlocks + j);

            for (int k = 0; k < min(m, (int)this->dimension - i * m); k++) {
                vector<int> row = cursor.getNext();
                for (int l = 0; l < min(m, (int)this->dimension - j * m); l++) {
                    mat[k][j * m + l] = row[l];
                }
            }
        }
        int rowCount = min(m, (int)this->dimension - i * m);
        this->writeRows(mat, rowCount, fout);
    }
    fout.close();
}

/**
 * @brief Function to check if matrix is already exported
 *
 * @return true if exported
 * @return false otherwise
 */
bool Matrix::isPermanent()
{
    logger.log("Matrix::isPermanent");
    if (this->sourceFileName == "../data/" + this->originalMatrixName + ".csv")
        return true;
    return false;
}

/**
 * @brief The unload function removes the matrix from the database by deleting
 * all temporary files created as part of this matrix
 *
 */
void Matrix::unload(){
    logger.log("Matrix::~unload");
    for (int pageCounter = 0; pageCounter < this->blockCount; pageCounter++)
        bufferManager.deleteFile(this->matrixName, pageCounter);
    if (!isPermanent())
        bufferManager.deleteFile(this->sourceFileName);
}
/**
 * Renames the Matrix to the newName specified. Renames all pages in memory and on disk
 * @param newName
 */
void Matrix::rename(string newName){
    logger.log("Matrix::rename");
    bufferManager.renamePagesInMemory(this->matrixName, newName);
    for (int pageCounter = 0; pageCounter < this->blockCount; pageCounter++)
        bufferManager.renameFile(this->matrixName, newName, pageCounter);
    this->matrixName = newName;
}

/**
 * @brief Goes through the entire matrix to check if it's symmetric. If checked before,
 * the value is cached and used instead of going through the matrix for subsequent calls
 * @return true if symmetric
 * @return false if asymmetric
 */
bool Matrix::symmetry() {
    logger.log("Matrix::symmetry");
    if (symmetric != -1) return symmetric;
    for (int i = 0; i < concurrentBlocks; i++) {
        for (int j = i; j < concurrentBlocks; j++) {
            if (i == j) {
                Cursor a(this->matrixName, i * concurrentBlocks + j, MATRIX);
                int lim = min((long long) this->m, this->dimension - i * m);
                for (int k = 0; k < lim; k++)
                    for (int l = k + 1; l < lim; l++)
                        if (a.getCell(k, l) != a.getCell(l, k)) return symmetric = false;
            }
            else {
                Cursor a(this->matrixName, i * concurrentBlocks + j, MATRIX);
                Cursor b(this->matrixName, j * concurrentBlocks + i, MATRIX);
                int limrow = min((long long)this->m, this->dimension - i * m);
                int limcol = min((long long)this->m, this->dimension - j * m);
                logger.log(to_string(limrow) + " " + to_string(limcol) + " " + to_string(i * m + j) +  " " + to_string(j * m + i));
                for (int k = 0; k < limrow; k++)
                    for (int l = k + 1; l < limcol; l++)
                        if (a.getCell(k, l) != b.getCell(l, k)) return symmetric = false;
            }
        }
    }
    return symmetric = true;
}

/**
 * @brief Tranposes the matrix in place
 */
void Matrix::transpose() {
    logger.log("Matrix::transpose");
    if (symmetric == 1) return;
    for (int i = 0; i < concurrentBlocks; i++) {
        Page* a = bufferManager.getPage(this->matrixName, i * concurrentBlocks + i, MATRIX);
        a->transpose();
        for (int j = i + 1; j < concurrentBlocks; j++) {
            Page* a = bufferManager.getPage(this->matrixName, i * concurrentBlocks + j, MATRIX);
            Page* b = bufferManager.getPage(this->matrixName, j * concurrentBlocks + i, MATRIX);
            a->transpose(b);
        }
    }
}

/**
 * Ran only for new matrices with no pages associated to it. Accesses pages of the
 * originalMatrix it was copied off of, and performs the computation, and writes a duplicate
 * page with it's own name, leaving the original page unchanged.
 * @param originalMatrix
 */
void Matrix::compute(string originalMatrix) {
    logger.log("Matrix::compute");
    for (int i = 0; i < concurrentBlocks; i++) {
        Page a = *bufferManager.getPage(originalMatrix, i * concurrentBlocks + i, MATRIX);
        a.subtractTranspose();
        a.setPageName(this->matrixName);
        a.writePage();
        for (int j = i + 1; j < concurrentBlocks; j++) {
            Page a = *bufferManager.getPage(originalMatrix, i * concurrentBlocks + j, MATRIX);
            Page b = *bufferManager.getPage(originalMatrix, j * concurrentBlocks + i, MATRIX);
            a.subtractTranspose(&b);
            a.setPageName(this->matrixName); a.writePage();
            b.setPageName(this->matrixName); b.writePage();
        }
    }
}

/**
 * @brief Function that returns a cursor that reads rows from this matrix
 *
 * @return Cursor
 */
Cursor Matrix::getCursor()
{
    logger.log("Matrix::getCursor");
    Cursor cursor(this->matrixName, 0, MATRIX);
    return cursor;
}
