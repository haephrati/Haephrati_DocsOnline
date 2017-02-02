/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "UploadData.h"
#include "Utils.h"

#define NON_QUEUE_POS_CHAR            '*'

//////////////////////////////////////////////////////////////////////////

void FormatInKilobytes(wstring &r, __int64 largeSize)
{
  const int kb = 1024;

  __int64 rounded = largeSize / kb;
  if (rounded * kb < largeSize)
    rounded++;
  r = str::Int64ToStr(rounded);
  r += L" K";
}

CUploadItem::CUploadItem( size_t queuePos, const wstring &path, size_t googlePathOffset, __int64 fileSize )
{
  m_queuePos = queuePos;
  m_displayPosValid = FALSE;
  m_state = QUEUE;
  m_path = path;
  m_fileSize = fileSize;
  m_googlePathOffset = googlePathOffset;

  ExtractFileName(m_displayFile, m_path);
  if (m_fileSize != -1)
    FormatInKilobytes(m_displaySize, m_fileSize);
}

CUploadItem::~CUploadItem( void )
{

}

void CUploadItem::SetStatus( ITEMSTATE state, const wstring &displayStatus )
{
  m_state = state;
  m_displayStatus = displayStatus;
}

// can't be const
const wchar_t *CUploadItem::GetDisplayQueuePos()
{
  if (!m_displayPosValid)
  {
    if (m_queuePos > 0)
      _itow_s(m_queuePos, m_displayPos, ARRAYDIM(m_displayPos), 10);
    else
    {
      m_displayPos[0] = NON_QUEUE_POS_CHAR;
      m_displayPos[1] = '\0';
    }
    m_displayPosValid = TRUE;
  }
  return m_displayPos;
}

void CUploadItem::NullQueuePos()
{
  ASSERT(m_queuePos != 0);
  m_queuePos = 0;
  m_displayPosValid = FALSE;
}

void CUploadItem::DecQueuePos()
{
  ASSERT(m_queuePos > 1);
  m_queuePos--;
  m_displayPosValid = FALSE;
}

void CUploadItem::Retry( size_t queuePos )
{
  ASSERT(m_queuePos == 0);
  ASSERT(IsErrorState());
  m_queuePos = queuePos;
  m_displayPosValid = FALSE;
  m_state = QUEUE;
}

BOOL CUploadItem::IsErrorState() const
{
  return (m_state != CUploadItem::QUEUE && m_state != CUploadItem::ACTIVE && m_state != CUploadItem::SUCCESS);
}

bool CUploadItem::CaselessGreater( const wstring &elem1, const wstring &elem2 )
{
  return str::CompareI(elem1, elem2) > 0;
}

bool CUploadItem::CaselessLower( const wstring &elem1, const wstring &elem2 )
{
  return str::CompareI(elem1, elem2) < 0;
}
//////////////////////////////////////////////////////////////////////////

CUploadData::CUploadData()
{
}

CUploadData::~CUploadData(void)
{
  clear();
}

const CUploadItem *CUploadData::const_at(int item) const
{
  ASSERT(item >= 0 && item < (int)m_data.size());
  return m_data[item];
}

const wchar_t * CUploadData::GetDisplayQueuePos( int item ) const
{
  ASSERT(item >= 0 && item < (int)m_data.size());
  return m_data[item]->GetDisplayQueuePos();
}

void CUploadData::clear()
{
  upload_vector::iterator iter;
  for (iter = m_data.begin(); iter != m_data.end(); iter++)
    delete *iter;
  m_data.clear();
}

void CUploadData::set_status( int item, CUploadItem::ITEMSTATE state, const wstring &displayStatus )
{
  ASSERT(item >= 0 && item < (int)m_data.size());
  m_data[item]->SetStatus(state, displayStatus);
}

void CUploadData::sort(CUploadItem::COMPARATOR comparator)
{
  std::sort(m_data.begin(), m_data.end(), comparator);
}

size_t CUploadData::initMaxQueuePos()
{
  upload_vector::iterator iter;
  size_t itemPos;

  m_maxQueuePos = 0;
  for (iter = m_data.begin(); iter != m_data.end(); iter++)
  {
    itemPos = (*iter)->GetQueuePos();
    if (itemPos > m_maxQueuePos)
      m_maxQueuePos = itemPos;
  }
  return m_maxQueuePos;
}

void CUploadData::Retry(size_t item)
{
  ASSERT(item < m_data.size());
  m_maxQueuePos++;
  m_data[item]->Retry(m_maxQueuePos);
}

void CUploadData::append_begin()
{
  m_sizeBeforeAddSession = m_data.size();
  initMaxQueuePos();
}

// returns TRUE if a new item has been created and appended to the list
// returns FALSE if item with the specified path already exists in the list
BOOL CUploadData::append( const wstring &path, size_t googlePathOffset, __int64 fileSize )
{
  BOOL result;
  upload_vector::iterator end;
  upload_vector::iterator iter;

  result = TRUE;
  end = m_data.begin() + m_sizeBeforeAddSession;
  for (iter = m_data.begin(); result && iter != end; iter++)
    result = !str::EqualI((*iter)->GetPath(), path);
  if (result)
  {
    m_maxQueuePos++;
    m_data.push_back(new CUploadItem(m_maxQueuePos, path, googlePathOffset, fileSize));
  }
  return result;
}

void CUploadData::append_end()
{

}

BOOL CUploadData::find_pos(size_t &index, size_t queuePos) const
{
  BOOL result;

  result = FALSE;
  for (index = 0; index < m_data.size(); index++)
  {
    if (m_data[index]->GetQueuePos() == queuePos)
    {
      result = TRUE;
      break;
    }
  }
  return result;
}

void CUploadData::null_pos( int item )
{
  upload_vector::iterator iter;
  size_t erasedPos;

  erasedPos = m_data[item]->GetQueuePos();
  if (erasedPos != 0) // a non-null pos, need to decrement positions of queue followers
  {
    m_data[item]->NullQueuePos();
    for (iter = m_data.begin(); iter != m_data.end(); iter++)
    {
      if ((*iter)->GetQueuePos() > erasedPos)
        (*iter)->DecQueuePos();
    }
  }
}

void CUploadData::erase( int item, int count )
{
  upload_vector::iterator start;
  upload_vector::iterator end;
  upload_vector::iterator iter;

  ASSERT(item >= 0 && item < (int)m_data.size());
  ASSERT(item + count <= (int)m_data.size());
  start = m_data.begin() + item;
  end = m_data.begin() + item + count;
  for (iter = start; iter < end; iter++)
    delete *iter;
  m_data.erase(start, end);
}

