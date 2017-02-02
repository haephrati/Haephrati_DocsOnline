#pragma once

template<typename T> class Singleton
{

public:
  static T& Instance()
  {
    static T theSingleInstance;
    return theSingleInstance;
  }
};