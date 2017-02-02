#pragma once

class out_of_memory :
  public std::exception
{
public:
  out_of_memory() : exception("Out of memory") { ; }
};

template<typename Type>
class CBuffer
{
public:
  CBuffer(): m_vector()
  {
  }
  CBuffer(size_t count): m_vector(count)
  {
  }
  ~CBuffer(void)
  {
  }
  operator Type* () { return &m_vector[0]; }
  size_t Size() { return m_vector.size(); }
  size_t Capacity() { return m_vector.capacity(); }
  size_t CapacityInBytes() { return Capacity() * sizeof(Type); }
  void SetSize(size_t newSize) // sets new size without buffer reallocation if possible
  {
    m_vector.resize(newSize);
  }
  void ProvideCapacity(size_t newCapacity)
  {
    //m_vector.reserve(newCapacity);
    m_vector.resize(newCapacity);
  }
  void Clear()
  {
    m_vector.resize(0);
  }
  void Append(const void *data, size_t count)
  {
    size_t oldSize;

    oldSize = m_vector.Size();
    m_vector.resize(m_vector.size() + count);
    memcpy(&m_vector[oldSize], data, count * sizeof(Type));
  }
private:
  std::vector<Type> m_vector;
};
