#ifndef Fs_H_
#define Fs_H_

#include <LittleFS.h>

class FilesysClass {
    
    public:
        void
            writeFile(const char* path, const char* data),
            initFS();
        
        String
            readFirstLine(const char* path),
            readEntireFile(const char* path);
    
    private:
};

extern FilesysClass Filesys;

#endif