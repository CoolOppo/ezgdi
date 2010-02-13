#pragma once

template <class T>
class CArray : public CSimpleArray<T>
{
public:
   T* Begin() const
   {
      return m_aT;
   }
   T* End() const
   {
      return m_aT + m_nSize;
   }
};

template <class T>
class CValArray : public CSimpleValArray<T>
{
public:
   T* Begin() const
   {
      return m_aT;
   }
   T* End() const
   {
      return m_aT + m_nSize;
   }
};

template <class T>
class CPtrArray : public CValArray<T*>
{
};

template <class TKey, class TVal>
class CMap : public CSimpleMap<TKey, TVal>
{
};
