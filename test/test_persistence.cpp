/******************************************************************************
The MIT License(MIT)

Embedded Template Library.
https://github.com/ETLCPP/etl
https://www.etlcpp.com

Copyright(c) 2021 jwellbelove

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "unit_test_framework.h"

#include "etl/experimental/persistence.h"
#include "etl/string.h"

namespace
{
  using IPersistence = etl::experimental::ipersistence;
  using String = etl::string<10>;
  using SmallerString = etl::string<5>;

  std::ostream& operator <<(std::ostream& os, const String& text)
  {
    os << text.c_str();

    return os;
  }

  //***********************
  struct Data
  {
    uint32_t i;
    String text;
  };

  bool operator ==(const Data& lhs, const Data& rhs)
  {
    return (lhs.i == rhs.i) && (lhs.text == rhs.text);
  }

  //***********************
  // How to save Data
  void save_to_persistent(IPersistence& persistence, const Data& value)
  {
    using etl::experimental::save_to_persistent;

    // Save the integer.
    save_to_persistent(persistence, value.i);

    // Save the string.
    save_to_persistent(persistence, value.text);
  }

  IPersistence& operator <<(IPersistence& persistence, const Data& value)
  {
    save_to_persistent(persistence, value);

    return persistence;
  }

  //***********************
  // How to load Data
  void load_from_persistent(IPersistence& persistence, Data& data)
  {
    using etl::experimental::load_from_persistent;

    // Load the integer.
    load_from_persistent(persistence, data.i);
    
    // Load the string.
    load_from_persistent(persistence, data.text);
  }

  IPersistence& operator >>(IPersistence& persistence, Data& value)
  {
    load_from_persistent(persistence, value);

    return persistence;
  }

  //***************************************************************************
  class Store : public IPersistence
  {
  public:

    //***********************
    Store()
      : index(0U)
    {
      std::fill(std::begin(buffer), std::end(buffer), 0xFF);
    }

    //***********************
    void start() override
    {
      index = 0U;
    }

    //***********************
    void step(size_t n)
    {
      index += n;
    }

    //***********************
    void save(const char* data, size_t length) override
    {
      while (length-- != 0)
      {
        buffer[index++] = *data++;
      }
    }

    //***********************
    void load(char* data, size_t length) override
    {
      while (length-- != 0)
      {
        *data++ = buffer[index++];
      }
    }

    //***********************
    void flush()
    {

    }

  private:

    size_t index;
    char buffer[20];
  };

  SUITE(test_persistence)
  {
    //*************************************************************************
    TEST(test_persistence_profiler_size)
    {
      etl::experimental::persistence_profiler profiler;

      Data data1 = { 99, "99" };

      save_to_persistent(profiler, data1.i);
      save_to_persistent(profiler, data1.text);

      CHECK_EQUAL(sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  (sizeof(String::value_type) * (String::MAX_SIZE + 1U)), profiler.size());
    }

    //*************************************************************************
    TEST(test_persistence_size)
    {
      Data data1 = { 99, "99" };

      size_t size = etl::experimental::persistence_size(data1);

      CHECK_EQUAL(sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  (sizeof(String::value_type) * (String::MAX_SIZE + 1U)), size);
    }

    //*************************************************************************
    TEST(test_save_load_functions)
    {
      Store store;

      store.start();

      Data data1 = { 99, "99" };

      save_to_persistent(store, data1);

      store.start();

      Data data2 = { 0, "0" };

      load_from_persistent(store, data2);

      CHECK(data1 == data2);
      CHECK_EQUAL(data1.i, data2.i);
      CHECK_EQUAL(data1.text.size(), data2.text.size());
      CHECK_EQUAL(data1.text, data2.text);
    }

    //*************************************************************************
    TEST(test_save_load_streaming)
    {
      Store store;

      store.start();

      Data data1 = { 99, "99" };

      store << data1.i << data1.text;

      store.start();

      Data data2 = { 0, "0" };

      store >> data2.i >> data2.text;

      CHECK(data1 == data2);
      CHECK_EQUAL(data1.i, data2.i);
      CHECK_EQUAL(data1.text.size(), data2.text.size());
      CHECK_EQUAL(data1.text, data2.text);
    }

    //*************************************************************************
    TEST(test_save_load_mismatch)
    {
      Store store;

      store.start();

      String text1 { "0123456789" };

      save_to_persistent(store, text1);

      store.start();

      SmallerString text2 = { "00000" };

      CHECK_THROW((load_from_persistent(store, text2)), etl::experimental::persistence_size_mismatch);
    }
  };
}
