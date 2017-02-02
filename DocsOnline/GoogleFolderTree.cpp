/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "GoogleFolderTree.h"
#include "Utils.h"
#include "myxml.h"

//////////////////////////////////////////////////////////////////////////

const wchar_t google_folderid_prefix[]=             L"https://docs.google.com/feeds/id/folder%3A";
const wchar_t google_folderhref_prefix[]=           L"https://docs.google.com/feeds/default/private/full/folder%3A";
const wchar_t google_resourceIdFolder[]=            L"folder";

//////////////////////////////////////////////////////////////////////////

_GOOGLE_FOLDER_DESCR::_GOOGLE_FOLDER_DESCR( const str::hash_string &_id, const wstring &_title )
{
  id = _id;
  title = _title;
}

//////////////////////////////////////////////////////////////////////////

wstring CGoogleFolderTree::StrWithoutPrefix(const wstring &s, size_t prefixLength)
{
  if (s.length() > prefixLength)
    return s.substr(prefixLength);
  else
    return str::EmptyString();
}

void CGoogleFolderTree::BuildFolderChildren( GOOGLE_FOLDER_DESCR &folderDescr, vector<GOOGLE_FOLDER_BD> &folders, size_t &toProcess )
{
  vector<GOOGLE_FOLDER_BD>::iterator iter;
  folder_list::iterator childIter;

  for (iter = folders.begin(); toProcess > 0 && iter != folders.end(); iter++)
  {
    if (!iter->processed && iter->parent_id.equal(folderDescr.id))
    { // a child folder found, let's add to my list
      folderDescr.children.push_back(GOOGLE_FOLDER_DESCR(iter->id, iter->title));
      iter->processed = TRUE;
      toProcess--;
    }
  }
  for (childIter = folderDescr.children.begin(); toProcess > 0 && childIter != folderDescr.children.end(); childIter++)
    BuildFolderChildren(*childIter, folders, toProcess);
}

void CGoogleFolderTree::BuildFolderTree( GOOGLE_FOLDER_DESCR &googleFolderTree, vector<GOOGLE_FOLDER_BD> &folders )
{
  vector<GOOGLE_FOLDER_BD>::iterator iter;
  size_t toProcess;
  folder_list::iterator childIter;

  googleFolderTree.id = RootFolderId();
  googleFolderTree.title = L"ROOT";
  toProcess = folders.size();
  for (iter = folders.begin(); iter != folders.end(); iter++)
  {
    if (iter->parent_id.empty())
    {
      googleFolderTree.children.push_back(GOOGLE_FOLDER_DESCR(iter->id, iter->title));
      iter->processed = TRUE;
      toProcess--;
    }
  }
  for (childIter = googleFolderTree.children.begin(); toProcess > 0 && childIter != googleFolderTree.children.end(); childIter++)
    BuildFolderChildren(*childIter, folders, toProcess);
}

wstring CGoogleFolderTree::ExtractIdFromIdRef(const wstring &https_id_folder)
{
  return StrWithoutPrefix(https_id_folder, ARRAYDIM(google_folderid_prefix)-1);
}

BOOL CGoogleFolderTree::AddFolders(size_t &entriesProcessed, vector<GOOGLE_FOLDER_BD> &folders, wstring &nextHref, const string &foldersXml)
{
  vector<folder_list> folderTree;
  xml_document xmlDoc;
  wstring entry_elem_name;
  BOOL result;

  result = FALSE;
  entriesProcessed = 0;
  nextHref.clear();
  xmlDoc.load(foldersXml);
  if (xmlDoc.root().name() == GOOGLE_XML_FEED)
  {
    result = TRUE;
    for (xml_element::const_iterator entry = xmlDoc.root().begin(); entry != xmlDoc.root().end(); entry++)
    {
      if (entry->name() == GOOGLE_XML_ENTRY)
      {
        entriesProcessed++;
        GOOGLE_FOLDER_BD folder;
        folder.processed = FALSE;
        for (xml_element::const_iterator entry_elem = entry->begin(); entry_elem != entry->end(); entry_elem++)
        {
          entry_elem_name = entry_elem->name();
          if (entry_elem_name == GOOGLE_XML_ID)
            folder.id = ExtractIdFromIdRef(entry_elem->val());
          else if (entry_elem_name == GOOGLE_XML_TITLE)
            folder.title = entry_elem->val();
          else if (entry_elem_name == GOOGLE_XML_LINK)
          {
            if (entry_elem->attr(GOOGLE_XML_REL) == GOOGLE_XML_PARENTLINKREL)
              folder.parent_id = StrWithoutPrefix(entry_elem->attr(GOOGLE_XML_HREF), ARRAYDIM(google_folderhref_prefix)-1);
          }
        }
        if (!folder.id.empty())
          folders.push_back(folder);
      }
      else if (entry->name() == GOOGLE_XML_LINK && entry->attr(GOOGLE_XML_REL) == GOOGLE_XML_LINKRELNEXT)
      {
        nextHref = entry->attr(GOOGLE_XML_HREF);
      }
    }
  }
  return result;
}

const wstring & CGoogleFolderTree::RootFolderId()
{
  return str::EmptyString();
}

BOOL CGoogleFolderTree::AddFiles(size_t &entriesProcessed, vector<GOOGLE_FILE_DESCR> &files, wstring &nextHref, const string &filesXml, const wstring &parentFolderId)
{
  vector<folder_list> folderTree;
  xml_document xmlDoc;
  wstring entry_elem_name;
  BOOL result = FALSE;
  wstring gdResourceId;
  wstring parent;
  wstring rel;
  size_t dividerIndex;

  entriesProcessed = 0;
  nextHref.clear();
  xmlDoc.load(filesXml);
  if (xmlDoc.root().name() == GOOGLE_XML_FEED)
  {
    result = TRUE;
    for (xml_element::const_iterator entry = xmlDoc.root().begin(); entry != xmlDoc.root().end(); entry++)
    {
      if (entry->name() == GOOGLE_XML_ENTRY)
      {
        entriesProcessed++;
        GOOGLE_FILE_DESCR googleFile;
        parent.clear();
        for (xml_element::const_iterator entry_elem = entry->begin(); entry_elem != entry->end(); entry_elem++)
        {
          entry_elem_name = entry_elem->name();
          if (entry_elem_name == GOOGLE_XML_GD_RESOURCEID)
          {
            gdResourceId = entry_elem->val();
            if (!gdResourceId.empty())
            {
              dividerIndex = gdResourceId.find(':');
              if (dividerIndex != wstring::npos)
              {
                if (dividerIndex > 0)
                  googleFile.gdResourceType = gdResourceId.substr(0, dividerIndex);
                if (dividerIndex+1 < gdResourceId.length())
                  googleFile.id = gdResourceId.substr(dividerIndex+1);
              }
            }
          }
          else if (entry_elem_name == GOOGLE_XML_TITLE)
            googleFile.title = entry_elem->val();
          else if (entry_elem_name == GOOGLE_XML_ENTRY_CONTENT)
          {
            googleFile.contentType = entry_elem->attr(GOOGLE_XML_ENTRY_CONTENT_TYPE);
            googleFile.contentSrc = entry_elem->attr(GOOGLE_XML_ENTRY_CONTENT_SRC);
          }
          else if (entry_elem_name == GOOGLE_XML_LINK)
          {
            if (entry_elem->attr(GOOGLE_XML_REL) == GOOGLE_XML_PARENTLINKREL)
              parent = StrWithoutPrefix(entry_elem->attr(GOOGLE_XML_HREF), ARRAYDIM(google_folderhref_prefix)-1);
          }
        }
        if (!googleFile.id.empty() && !googleFile.title.empty() && parent == parentFolderId && googleFile.gdResourceType != google_resourceIdFolder)
          files.push_back(googleFile);
      }
      else if (entry->name() == GOOGLE_XML_LINK && entry->attr(GOOGLE_XML_REL) == GOOGLE_XML_LINKRELNEXT)
      {
        nextHref = entry->attr(GOOGLE_XML_HREF);
      }
    }
  }
  return result;
}

