#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "pfm.h"

#include <stddef.h>

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}

PagedFileManager::PagedFileManager()
{

}

PagedFileManager::~PagedFileManager()
{

}
int fileExists(const char *fileName) {
    if (access(fileName, F_OK) != -1) {
        return 1;
    } else {
        return 0;
    }
}

RC PagedFileManager::createFile(const string &fileName) {

    FILE *pageFile;

    // Check if file exists
    if (fileExists(fileName.c_str())) {
        return -1;
    } else {
        pageFile = std::fopen(fileName.c_str(), "wb+");
    }
    if (pageFile == NULL) {
        return -1;

        initializeFileStat(pageFile);

        int closedSuccessfully = std::fclose(pageFile);
        if (closedSuccessfully == 0) {

        } else {
            return -1;
        }
        return 0;
    }
}

void PagedFileManager::initializeFileStat(FILE *pageFile) {
  fseek(pageFile, 0, SEEK_SET);
  FileStat initialStat;
  initialStat.numberOfPages = 0;
  initialStat.readPageCounter = 0;
  initialStat.writePageCounter = 0;
  initialStat.appendPageCounter = 0;
  fwrite(&initialStat, sizeof(struct FileStat), 1, pageFile);
}

void PagedFileManager::updateFileStat(FILE *pageFile, FileStat &fileStat) {
  fseek(pageFile, 0, SEEK_SET);
  fwrite(&fileStat, sizeof(struct FileStat), 1, pageFile);
}

RC PagedFileManager::destroyFile(const string &fileName) {
    if (!fileExists(fileName.c_str())) {
        return -1;
    }
    if (!remove(fileName.c_str()) == 0) {
        return -1;
    }
    return 0;
}

RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {

    if (fileHandle.getFile() != NULL) {
        return -1;
    }

    // Check that the file exists
    if (fileExists(fileName.c_str())) {
        return 0;
    } else {
        return -1;
    }

    FILE *file = fopen(fileName.c_str(), "rb+");
    if (file == NULL) {
        return -1;
    }
    fileHandle.setFile(file);
    fileHandle.loadFileStats();
    return 0;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    FILE *file = fileHandle.getFile();
    FileStat currentFileStats;
    currentFileStats.numberOfPages = fileHandle.numberOfPages;
    currentFileStats.readPageCounter = fileHandle.readPageCounter;
    currentFileStats.writePageCounter = fileHandle.writePageCounter;
    currentFileStats.appendPageCounter = fileHandle.appendPageCounter;

    updateFileStat(file, currentFileStats);

    int IfFlushed = fflush(file);
    int IfClosed = fclose(file);

    fileHandle.resetFile();

    if (IfFlushed == 0 && IfClosed == 0) {
        return 0;
    } else {
        return -1;
    }

}

FileHandle::FileHandle()
{
    numberOfPages = 0;
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    file = NULL;
}
FILE *FileHandle::getFile() { return file; }

RC FileHandle::setFile(FILE *_file) {
  if (file != NULL) {
    return -1;
  }
  file = _file;
  return 0;
}
FileHandle::~FileHandle()
{

}

RC FileHandle::readPage(PageNum pageNum, void *data)
{
    if (numberOfPages <= pageNum) {
    return -1;
  }
  fseek(file, PageStartCounter + pageNum * PAGE_SIZE, SEEK_SET);
  size_t pagesRead = fread(data, PAGE_SIZE, 1, file);
  if (pagesRead != 1) {
    return -1;
  }
  readPageCounter = readPageCounter + 1;
  return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data) {
    if (pageNum >= numberOfPages) {
    return -1;
  }
  fseek(file, PageStartCounter + PAGE_SIZE * pageNum, SEEK_SET);
  int pagesWritten = fwrite(data, PAGE_SIZE, 1, file);
  if (pagesWritten != 1) {
    return -1;
  }
  int success = fflush(file);
  writePageCounter = writePageCounter + 1;
  return 0;
}


RC FileHandle::appendPage(const void *data) {
fseek(file, PageStartCounter + numberOfPages * PAGE_SIZE, SEEK_SET);
  int pagesAppended = fwrite(data, PAGE_SIZE, 1, file);
  int writeSuccess = fflush(file);
  if (pagesAppended != 1) {
    return -1;
  }
  appendPageCounter = appendPageCounter + 1;
  numberOfPages++;

  FileStat currentFileStats;
  currentFileStats.numberOfPages = numberOfPages;
  currentFileStats.readPageCounter = readPageCounter;
  currentFileStats.writePageCounter = writePageCounter;
  currentFileStats.appendPageCounter = appendPageCounter;

  updateFileStat(currentFileStats);

  return 0;
}


unsigned FileHandle::getNumberOfPages() {
return numberOfPages;
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    readPageCount = readPageCounter;
    writePageCount = writePageCounter;
    appendPageCount = appendPageCounter;
    return 0;
}

void FileHandle::loadFileStats() {
  if (file == NULL) {
  }
  FileStat fileStat;
  fseek(file, 0, SEEK_SET);
  fread(&fileStat, sizeof(struct FileStat), 1, file);
  numberOfPages = fileStat.numberOfPages;
  readPageCounter = fileStat.readPageCounter;
  writePageCounter = fileStat.writePageCounter;
  appendPageCounter = fileStat.appendPageCounter;
}

void FileHandle::updateFileStat(FileStat &fileStat) {
  fseek(file, 0, SEEK_SET);
  fwrite(&fileStat, sizeof(struct FileStat), 1, file);
  fflush(file);
}

RC FileHandle::resetFile() {
  file = NULL;
  numberOfPages = 0;
  readPageCounter = 0;
  writePageCounter = 0;
  appendPageCounter = 0;
}