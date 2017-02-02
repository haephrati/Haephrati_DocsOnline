#pragma once

namespace str
{
  class hash_string
  {
  public:
    hash_string() : m_s(), m_hash(hash_def) { }
    //hash_string(const hash_string &other): m_s(other.m_s), m_hash(other.m_hash) { }
    hash_string(const wstring &s) : m_s(), m_hash(hash_def) 
    {
      *this = s;
    }
    bool empty() const { return m_s.empty(); }
    void clear()
    {
      m_s.clear();
      m_hash = hash_def;
    }
    void operator=(const wstring &s)
    {
      m_s = s;
      calcHash();
    }
    bool equal(const hash_string &other) const
    {
      if (m_hash != other.m_hash)
        return false;
      else
        return m_s == other.m_s;
    }
    operator const wstring &() const { return m_s; }
    /* 
    bool operator==(const hash_string &other) const { return equal(other); }
    bool operator!=(const hash_string &other) const { return !equal(other); }
    */
  private:
    typedef int hash_type;
    static const hash_type hash_def = 0;
    wstring m_s;
    hash_type m_hash;

    void calcHash()
    {
      size_t i;
      size_t count;
      hash_type *p;
      BYTE *remainder;
      size_t remainderLen;
      size_t lengthInBytes;

      m_hash = hash_def;
      if (m_s.length() > 0)
      {
        p = (hash_type*)&m_s[0];
        lengthInBytes = m_s.length() * sizeof(wstring::value_type);
        count = lengthInBytes / sizeof(hash_type);
        for (i = 0; i < count; i++)
          m_hash += *p++;
        remainder = (BYTE*)p;
        remainderLen = lengthInBytes - count * sizeof(hash_type);
        for (i = 0; i < remainderLen; i++)
          m_hash += *remainder++;
      }
    }
  };
}