#pragma once

class uFindFile
{
protected:
    uFindFile() {}
public:
    virtual ~uFindFile() {};
    virtual bool FindNext() = 0;
    virtual const char * GetFileName() = 0;
    virtual bool IsDirectory() = 0;
};

typedef uFindFile * puFindFile;

puFindFile uFindFirstFile(const char * path);

pfc::string8 uStringPrintf(const char * format, ...);
void uPrintfV(pfc::string_base & out,const char * fmt,va_list arglist);
void uPrintf(pfc::string_base & out,const char * fmt,...);

bool uGetTempPath(pfc::string_base & out);
bool uGetTempFileName(const char * path_name,const char * prefix,unsigned unique,pfc::string_base & out);
pfc::string8 uGetTempFileName();

