#pragma once

#define GOOGLE_XML_FEED                     L"feed"
#define GOOGLE_XML_ENTRY                    L"entry"
#define GOOGLE_XML_TITLE                    L"title"
#define GOOGLE_XML_ID                       L"id"
#define GOOGLE_XML_LINK                     L"link"
#define GOOGLE_XML_REL                      L"rel"
#define GOOGLE_XML_LINKRELNEXT              L"next"
#define GOOGLE_XML_PARENTLINKREL            L"http://schemas.google.com/docs/2007#parent"
#define GOOGLE_XML_HREF                     L"href"
#define GOOGLE_XML_ENTRY_CONTENT            L"content"
#define GOOGLE_XML_ENTRY_CONTENT_TYPE       L"type"
#define GOOGLE_XML_ENTRY_CONTENT_SRC        L"src"
#define GOOGLE_XML_GD_RESOURCEID            L"gd:resourceId"

typedef vector<const struct _GOOGLE_FOLDER_DESCR> folder_list;
typedef struct _GOOGLE_FOLDER_DESCR
{
  str::hash_string id;
  wstring title;
  folder_list children;

  _GOOGLE_FOLDER_DESCR() { ; }
  _GOOGLE_FOLDER_DESCR(const str::hash_string &_id, const wstring &_title);

} GOOGLE_FOLDER_DESCR;

typedef vector<wstring> path_vector;

typedef struct _GOOGLE_FILE_DESCR
{
  wstring title;
  wstring id;
  wstring gdResourceType;
  wstring contentType;
  wstring contentSrc;
} GOOGLE_FILE_DESCR;

class CGoogleFolderTree
{
public:
  typedef struct
  {
    BOOL processed;
    str::hash_string id;
    str::hash_string parent_id;
    wstring title;
  } GOOGLE_FOLDER_BD;

  static void BuildFolderTree(GOOGLE_FOLDER_DESCR &googleFolderTree, vector<GOOGLE_FOLDER_BD> &folders);
  static const wstring &RootFolderId();
  static wstring ExtractIdFromIdRef(const wstring &https_id_folder);
  static BOOL AddFiles(size_t &entriesProcessed, vector<GOOGLE_FILE_DESCR> &files, wstring &nextHref, const string &filesXml, const wstring &parentFolderId);
  static BOOL AddFolders(size_t &entriesProcessed, vector<GOOGLE_FOLDER_BD> &folders, wstring &nextHref, const string &foldersXml);
private:

  static wstring StrWithoutPrefix(const wstring &s, size_t prefixLength);
  static void BuildFolderChildren(GOOGLE_FOLDER_DESCR &folderDescr, vector<GOOGLE_FOLDER_BD> &folders, size_t &toProcess);
};
