#pragma once

#include "UploadData.h"

// Files list view
#define FILESLV_COL_ICON                  0
#define FILESLV_COL_QUEUEPOS			        1
#define FILESLV_COL_FILENAME              2
#define FILESLV_COL_FILESIZE              3
#define FILESLV_COL_STATUS                4

class CUploadData;

class CUploadList
{
public:
  static const size_t npos = (size_t)(-1);
  CUploadList(void);
  ~CUploadList(void);

  void AddFilesBegin();
  void AddFile(const wstring &path, size_t googlePathOffset, __int64 fileSize);
  size_t AddFilesEnd(size_t &activeItemIndex);

  void Create(HWND parentWindow);
  HWND GetHandle() const { return m_updateList; }
  void OnGetDispInfo(NMLVDISPINFO* pnmv);
  void OnColumnClick(int column, size_t &activeItemIndex);
  void Clear();
  void SetCurrentStatus(int item, CUploadItem::ITEMSTATE state, const wstring &msg);
  void RemoveSelectedItems(size_t &activeItemIndex);
  const wstring &GetItemPath(int item) const;
  void GetUploadItemParams(int item, wstring &path, size_t &googlePathOffset);
  size_t Size() const;
  void NullPos1();
  BOOL FindPos1(size_t &pos1_index);
  void OnContextMenu(int x, int y);
  BOOL RetrySelectedItems(size_t &pos1_index);
private:
  HWND m_parentWindow;
  CUploadData m_queue;
  HWND m_updateList;
  WNDPROC m_oldUpdateListProc;
  HWND m_header;
  int m_sortColumn;
  BOOL m_sortDown;
  CUploadItem::COMPARATOR *m_comparator;
  size_t m_addCount; // total count of items added during Add session

  static LRESULT CALLBACK StaticUpdateListProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  void ResizeListControl();
  void RedrawAll();
  LRESULT CALLBACK UpdateListProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  void SetHeaderSort(int column, DWORD format);
  void Resort();
  BOOL IsPosLowerSort();
  BOOL IsItemSelected(int item);
  void RedrawItems(int first, int last);
};
