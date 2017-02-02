#pragma once

class CUploadItem
{
public:
  typedef enum { QUEUE = 0, ACTIVE, SUCCESS, WARNING, STOPPED, ITEMERROR } ITEMSTATE;
  typedef bool COMPARATOR( CUploadItem *elem1, CUploadItem *elem2 );

  CUploadItem( size_t queuePos, const wstring &path, size_t googlePathOffset, __int64 fileSize );
  ~CUploadItem(void);
  void SetStatus(ITEMSTATE state, const wstring &displayStatus);
  ITEMSTATE GetState() const { return m_state; }
  size_t GetQueuePos() const { return m_queuePos; }
  void NullQueuePos();
  void DecQueuePos();
  void Retry(size_t queuePos);
  BOOL IsErrorState() const;
  const wstring &GetPath() const { return m_path; }
  size_t GetGooglePathOffset() const { return m_googlePathOffset; }

  const wchar_t *GetDisplayQueuePos();
  const wstring &GetDisplayFile() const { return m_displayFile; }
  const wstring &GetDisplaySize() const { return m_displaySize; }
  const wstring &GetDisplayStatus() const { return m_displayStatus; }

  static bool PosGreater( CUploadItem *elem1, CUploadItem *elem2 ) { return elem1->GetQueuePos() > elem2->GetQueuePos(); }
  static bool PosLower( CUploadItem *elem1, CUploadItem *elem2 ) { return elem1->GetQueuePos() < elem2->GetQueuePos(); }
  static bool FileNameGreater( CUploadItem *elem1, CUploadItem *elem2 ) { return CaselessGreater(elem1->GetDisplayFile(), elem2->GetDisplayFile()); }
  static bool FileNameLower( CUploadItem *elem1, CUploadItem *elem2 ) { return CaselessLower(elem1->GetDisplayFile(), elem2->GetDisplayFile()); }
  static bool FileSizeGreater( CUploadItem *elem1, CUploadItem *elem2 ) { return elem1->m_fileSize > elem2->m_fileSize; }
  static bool FileSizeLower( CUploadItem *elem1, CUploadItem *elem2 ) { return elem1->m_fileSize < elem2->m_fileSize; }
private:
  ITEMSTATE m_state;
  int m_retries;
  size_t m_queuePos;
  wstring m_path;
  __int64 m_fileSize;
  size_t m_googlePathOffset;

  wchar_t m_displayPos[21];
  BOOL m_displayPosValid;
  wstring m_displayFile;
  wstring m_displaySize;
  wstring m_displayStatus;

  static bool CaselessGreater(const wstring &elem1, const wstring &elem2);
  static bool CaselessLower(const wstring &elem1, const wstring &elem2);
};

class CUploadData
{
public:
  CUploadData(void);
  ~CUploadData(void);

  void append_begin();
  BOOL append(const wstring &path, size_t googlePathOffset, __int64 fileSize);
  void append_end();

  const CUploadItem *const_at(int item) const;
  const wchar_t *GetDisplayQueuePos(int item) const;
  void clear();
  size_t size() const { return m_data.size(); }
  void set_status(int item, CUploadItem::ITEMSTATE state, const wstring &displayStatus);
  void erase(int item, int count);
  void sort(CUploadItem::COMPARATOR comparator);
  BOOL find_pos(size_t &index, size_t queuePos) const;
  void null_pos(int item);
  size_t initMaxQueuePos();
  void Retry(size_t item);
private:
  typedef vector<CUploadItem*> upload_vector;
  upload_vector m_data;
  size_t m_maxQueuePos;
  size_t m_sizeBeforeAddSession;
};
