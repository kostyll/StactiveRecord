/*
Copyright (C) 2007 Butterfat, LLC (http://butterfat.net)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Created by bmuller <bmuller@butterfat.net>
*/

namespace stactiverecord {

  /**
   * \class Q 
   * \brief A query object that supports boolean operations.
   * \author bmuller <bmuller@butterfat.net>
   */
  class Q {
  private:
    std::string key;
    std::string value;
    int ivalue;
    SarVector<Q> ored;
    SarVector<Q> anded;
    coltype ct;
    Where *where;
  public:
    /** Create query object 
     * @param _key The key being queried
     * @param _w A Where object describing the limits of the query
     */
    Q(std::string _key, Where *_w) : key(_key) { ct = _w->ct; where = _w; };

    /** Create query object 
     * @param _key The key being queried
     * @param _value The exact value to match for the given key.
     */
    Q(std::string _key, std::string _value) : key(_key), value(_value) { ct = STRING; where = equals(_value); };

    /** Create query object 
     * @param _key The key being queried
     * @param _value The exact value to match for the given key.
     */
    Q(std::string _key, int _ivalue) : key(_key), ivalue(_ivalue) { ct = INTEGER; where = equals(_ivalue); };

    /** Boolean or operator */
    Q & operator||(Q other);

    /** Boolean and operator */
    Q & operator&&(Q other);

    /** using the db, find all records that match
     * @param classname The classname to test
     * @param db The Sar_Dbi object - it can be directly queried
     */
    SarVector<int> test(std::string classname, Sar_Dbi *sar_dbi);

    /** Print out all the other Q objects anded and ored */
    void dump();

    /** Free the memory used by the internal where instance var and the ones
     * in the ored and anded objects.
     */
    void free();
  };
  
};
