/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "UploadList.h"
#include "UploadData.h"
#include "Application.h"
#include "Utils.h"
#include "resource.h"
#include "UIStrings.h"

#define FILESLV_ICONNWIDTH                20
#define FILESLV_QUEUEPOSWIDTH             30
#define FILESLV_FILENAMEWIDTH             250
#define FILESLV_FILESIZEWIDTH             75
#define FILESLV_STATUSWIDTH               150

// Status image list
#define STATUS_IACTIVE                    0
#define STATUS_ISUCCESS                   1
#define STATUS_IWARNING                   2
#define STATUS_IFAILED                    3
#define STATUS_ICRITICALERROR             4
#define STATUS_ISTOPPED                   5

CUploadList::CUploadList(void)
{
}

CUploadList::~CUploadList(void)
{
}

void CUploadList::Create(HWND parentWindow)
{
  LVCOLUMN lvc;
  wstring text;
  HIMAGELIST statusImages;

  m_parentWindow = parentWindow;
  m_updateList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW,
    L"", WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA,
    0, 0, 0, 0, parentWindow, NULL, AppInstance()->GetInstance(), NULL);
  ListView_SetExtendedListViewStyle(m_updateList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

  statusImages = ImageList_LoadImage(AppInstance()->GetResInstance(), (LPCWSTR)IDB_STATI, 
    15, 1, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
  ListView_SetImageList(m_updateList, statusImages, LVSIL_SMALL);

  lvc.mask = LVCF_FMT | LVCF_WIDTH;
  lvc.fmt = LVCFMT_LEFT;

  lvc.cx = FILESLV_ICONNWIDTH;
  ListView_InsertColumn(m_updateList, FILESLV_COL_ICON, &lvc);

  lvc.mask |= LVCF_TEXT;
  lvc.cx = FILESLV_QUEUEPOSWIDTH;
  text = UIFormat(lbl_queuepos);
  lvc.pszText = (LPWSTR)text.c_str();
  ListView_InsertColumn(m_updateList, FILESLV_COL_QUEUEPOS, &lvc);

  lvc.cx = FILESLV_FILENAMEWIDTH;
  text = UIFormat(lbl_file);
  lvc.pszText = (LPWSTR)text.c_str();
  ListView_InsertColumn(m_updateList, FILESLV_COL_FILENAME, &lvc);

  lvc.cx = FILESLV_FILESIZEWIDTH;
  text = UIFormat(lbl_size);
  lvc.pszText = (LPWSTR)text.c_str();
  ListView_InsertColumn(m_updateList, FILESLV_COL_FILESIZE, &lvc);

  lvc.cx = FILESLV_STATUSWIDTH;
  text = UIFormat(lbl_status);
  lvc.pszText = (LPWSTR)text.c_str();
  ListView_InsertColumn(m_updateList, FILESLV_COL_STATUS, &lvc);

  m_header = ListView_GetHeader(m_updateList);
  m_sortColumn = FILESLV_COL_QUEUEPOS;
  m_sortDown = FALSE;
  m_comparator = CUploadItem::PosLower;
  SetHeaderSort(m_sortColumn, HDF_SORTUP);

  /*m_oldUpdateListProc = (WNDPROC) GetWindowLongPtr(m_updateList, GWLP_WNDPROC);
  SetWindowLongPtr(m_updateList, GWLP_WNDPROC, (LONG_PTR)StaticUpdateListProc);*/
}

/*LRESULT CALLBACK CUploadList::UpdateListProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_RBUTTONUP:
    break;
  default:
    return m_oldUpdateListProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

LRESULT CALLBACK CUploadList::StaticUpdateListProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  return AppWindow()->UpdateListProc(hWnd, message, wParam, lParam);
}*/

void CUploadList::RedrawItems(int first, int last)
{
  ListView_RedrawItems(m_updateList, first, last);
}

void CUploadList::RedrawAll()
{
  RedrawItems(0, m_queue.size()-1);
}

void CUploadList::Resort()
{
  m_queue.sort(m_comparator);
}

void CUploadList::SetHeaderSort(int column, DWORD format)
{
  HD_ITEM hdi;

  hdi.mask = HDI_FORMAT;
  hdi.fmt = HDF_STRING | format;
  Header_SetItem(m_header, column, &hdi);
}

// on input activeItemIndex index contains index of item being uploaded or npos if none
// activeItemIndex on return receives the new index of active item
void CUploadList::OnColumnClick(int column, size_t &activeItemIndex)
{
  CUploadItem::COMPARATOR *comp;
  BOOL newSortDown;

  if (column != m_sortColumn) // click on column other then current
    newSortDown = FALSE;
  else
    newSortDown = !m_sortDown; // reverse sort order
  switch (column)
  {
  case FILESLV_COL_QUEUEPOS:
    comp = newSortDown ? CUploadItem::PosGreater : CUploadItem::PosLower;
    break;
  case FILESLV_COL_FILENAME:
    comp = newSortDown ? CUploadItem::FileNameGreater : CUploadItem::FileNameLower;
    break;
  case FILESLV_COL_FILESIZE:
    comp = newSortDown ? CUploadItem::FileSizeGreater : CUploadItem::FileSizeLower;
    break;
  default:
    comp = NULL;
  }
  if (comp != NULL)
  {
    m_comparator = comp;
    if (column != m_sortColumn)
      SetHeaderSort(m_sortColumn, 0);
    m_sortColumn = column;
    m_sortDown = newSortDown;
    SetHeaderSort(m_sortColumn, m_sortDown ? HDF_SORTDOWN : HDF_SORTUP);
    if (m_queue.size() > 1)
    {
      Resort();
      if (activeItemIndex != npos)
      {
        BOOL found = FindPos1(activeItemIndex);
        ASSERT(found);
      }
      RedrawAll();
    }
  }
}

void CUploadList::OnGetDispInfo(NMLVDISPINFO* pnmv)
{
  const wchar_t *sz;
  int image;
  const CUploadItem *u = m_queue.const_at(pnmv->item.iItem);

  if (pnmv->item.mask & LVIF_TEXT) 
  {
    switch (pnmv->item.iSubItem)
    {
    case FILESLV_COL_ICON:
      sz = str::EmptyString().c_str();
      break;
    case FILESLV_COL_QUEUEPOS:
      sz = m_queue.GetDisplayQueuePos(pnmv->item.iItem);
      break;
    case FILESLV_COL_FILENAME: 
      sz = u->GetDisplayFile().c_str();
      break;
    case FILESLV_COL_FILESIZE:
      sz = u->GetDisplaySize().c_str();
      break;
    case FILESLV_COL_STATUS:
      sz = u->GetDisplayStatus().c_str();
      break;
    default:
      ASSERT(FALSE);
      sz = str::EmptyString().c_str();
    }
    pnmv->item.pszText = const_cast<LPWSTR>(sz);
  }

  if (pnmv->item.mask & LVIF_IMAGE) 
  {
    switch (u->GetState())
    {
    case CUploadItem::ACTIVE:
      image = STATUS_IACTIVE;
      break;
    case CUploadItem::SUCCESS:
      image = STATUS_ISUCCESS;
      break;
    case CUploadItem::WARNING:
      image = STATUS_IWARNING;
      break;
    case CUploadItem::ITEMERROR:
      image = STATUS_IFAILED;
      break;
    case CUploadItem::STOPPED:
      image = STATUS_ISTOPPED;
      break;
    default:
      image = I_IMAGENONE;
    }
    pnmv->item.iImage = image;
  }

  if (pnmv->item.mask & LVIF_STATE)
  {
    pnmv->item.state = 0;
  }

}

void CUploadList::ResizeListControl()
{
  ListView_SetItemCount(m_updateList, m_queue.size());
}

// on input activeItemIndex index contains index of item being uploaded or npos if none
// the item will not be removed from the list
// activeItemIndex on return receives the new index of active item
void CUploadList::RemoveSelectedItems(size_t &activeItemIndex)
{
  int item;
  int count;
  int toErase;
  int erasedCount;
  int start;
  size_t newActiveItemIndex;

  ASSERT(activeItemIndex == npos || activeItemIndex < m_queue.size());
  if (ListView_GetSelectedCount(m_updateList) > 0)
  {
    newActiveItemIndex = activeItemIndex;
    erasedCount = 0;
    count = (int)m_queue.size();
    start = -1; // no selection start
    for (item = 0; item < count; item++)
    {
      if (item == activeItemIndex || !IsItemSelected(item)) // end of selection series
      {
        if (start != -1)
        {
          toErase = item - start;
          m_queue.erase(start-erasedCount, toErase);
          erasedCount += toErase;
          start = -1;
        }
        if (item == activeItemIndex)
          newActiveItemIndex = item-erasedCount; // update activeItemIndex
      }
      else // selected item
      {
        m_queue.null_pos(item-erasedCount);
        if (item < (int)m_queue.size())
          ListView_SetItemState(m_updateList, item, 0, LVIS_SELECTED); // remove selection if not out of scope
        if (start == -1)
        {
          start = item;
        }
      } // else just continue selection count increment
    }
    if (start != -1) // remove the last selection block if any
      m_queue.erase(start-erasedCount, count - start); 
    ResizeListControl(); // no need to remove state of the last items, just resize
    activeItemIndex = newActiveItemIndex;
  }
}

void CUploadList::Clear()
{
  ListView_DeleteAllItems(m_updateList);
  m_queue.clear();
}

void CUploadList::SetCurrentStatus(int item, CUploadItem::ITEMSTATE state, const wstring &msg)
{
  m_queue.set_status(item, state, msg);
  ListView_Update(m_updateList, item);
}

const wstring &CUploadList::GetItemPath(int item) const
{
  return m_queue.const_at(item)->GetPath();
}

void CUploadList::GetUploadItemParams(int item, wstring &path, size_t &googlePathOffset)
{
  const CUploadItem *uploadItem = m_queue.const_at(item);
  path = uploadItem->GetPath();
  googlePathOffset = uploadItem->GetGooglePathOffset();
}

size_t CUploadList::Size() const
{
  return m_queue.size();
}

void CUploadList::AddFilesBegin()
{
  m_queue.append_begin();
  m_addCount = 0;
}

void CUploadList::AddFile( const wstring &path, size_t googlePathOffset, __int64 fileSize )
{
  if (m_queue.append(path, googlePathOffset, fileSize))
    m_addCount++;
}

// on input activeItemIndex index contains index of item being uploaded or npos if none
// activeItemIndex on return receives the new index of active item
// returns number of items that were actually added to the list
size_t CUploadList::AddFilesEnd( size_t &activeItemIndex )
{
  m_queue.append_end();
  if (m_addCount > 0)
  {
    if (!IsPosLowerSort()) // no need to sort if currently Pos-sorting is on
    {
      Resort();
      if (activeItemIndex != npos)
      {
        BOOL found = FindPos1(activeItemIndex);
        ASSERT(found);
      }
    }
    ResizeListControl();
  }
  return m_addCount;
}

// TRUE if default sort mode is on with Pos 0 - 1 - 2...
BOOL CUploadList::IsPosLowerSort()
{
  return m_comparator == CUploadItem::PosLower;
}

void CUploadList::NullPos1()
{
  size_t pos1_index;

  if (m_queue.find_pos(pos1_index, 1))
  {
    m_queue.null_pos(pos1_index);
    // no need to resort in any active sorting case
    RedrawAll();
  }
  else
  {
    ASSERT(FALSE);
  }
}

BOOL CUploadList::FindPos1(size_t &pos1_index)
{
  BOOL result = m_queue.find_pos(pos1_index, 1);
  if (!result)
    pos1_index = npos;
  return result;
}

void CUploadList::OnContextMenu( int x, int y )
{
  HMENU menu;
  int selectedItem;

  selectedItem = -1;
  if (ListView_GetSelectedCount(m_updateList) == 1)
    selectedItem = ListView_GetSelectionMark(m_updateList);

  menu = CreatePopupMenu();
  InsertMenu(menu, -1, MF_BYPOSITION | MF_STRING, IDM_UPLOADREMOVE, UIFormat(mnu_uploadlistremove).c_str());
  if (selectedItem != -1 && m_queue.const_at(selectedItem)->GetState() == CUploadItem::ACTIVE)
    EnableMenuItem(menu, IDM_UPLOADREMOVE, MF_BYCOMMAND | MF_DISABLED);
  InsertMenu(menu, -1, MF_BYPOSITION | MF_STRING, IDM_UPLOADRETRY, UIFormat(mnu_uploadlistretry).c_str());
  if (selectedItem != -1 && !m_queue.const_at(selectedItem)->IsErrorState())
    EnableMenuItem(menu, IDM_UPLOADRETRY, MF_BYCOMMAND | MF_DISABLED);
  TrackPopupMenu(menu, 0, x, y, 0, m_parentWindow, NULL);
  DestroyMenu(menu);
}

BOOL CUploadList::IsItemSelected(int item)
{
  return (ListView_GetItemState(m_updateList, item, LVIS_SELECTED) != 0);
}

// returns TRUE if any of the selected items were queued for upload
// pos1_index on returns contains index of item with GetPos = 1 i.e. the first 
// to upload if there was no active upload on function call and npos otherwise
BOOL CUploadList::RetrySelectedItems(size_t &pos1_index)
{
  BOOL result;
  int item;
  size_t initialMaxPos;
  int first;
  int last;
  int selectedProcessed;
  int selectedCount;

  first = -1;
  pos1_index = npos;
  selectedCount = ListView_GetSelectedCount(GetHandle());
  if (selectedCount > 0)
  {
    selectedProcessed = 0;
    for (item = 0; selectedProcessed < selectedCount; item++)
    {
      if (item == m_queue.size())
      {
        ASSERT(FALSE);
        break;
      }
      if (IsItemSelected(item))
      {
        selectedProcessed++;
        if (m_queue.const_at(item)->IsErrorState())
        {
          if (first == -1) // first item to re-queue
          {
            first = item;
            result = TRUE;
            initialMaxPos = m_queue.initMaxQueuePos();
            if (initialMaxPos == 0)
              pos1_index = item;
          }
          m_queue.Retry(item);
          last = item;
        }
      }
    }
    if (first != -1)
      RedrawItems(first, last);
  }
  return (first != -1);
}

