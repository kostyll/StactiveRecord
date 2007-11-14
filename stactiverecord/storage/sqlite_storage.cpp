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

#include "../stactive_record.h"

namespace stactiverecord {

  SQLiteStorage::SQLiteStorage(std::string location) {
    debug("Attempting to open SQLite DB at " + location);
    is_closed = false;
    int rc = sqlite3_open(location.c_str(), &db);
    test_result(rc, "problem opening database");
    execute("CREATE TABLE IF NOT EXISTS id_maximums (id INT, classname VARCHAR(255))");
    execute("CREATE TABLE IF NOT EXISTS relationships (class_one VARCHAR(255), class_one_id INT, class_two VARCHAR(255), class_two_id INT)");
    debug("sqlite database opened successfully");
  };

  void SQLiteStorage::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };

  void SQLiteStorage::test_result(int result, const std::string& context) {
    if(result != SQLITE_OK){
      std::string msg = "SQLite Error in Session Manager - " + context + ": " + std::string(sqlite3_errmsg(db)) + "\n";
      sqlite3_close(db);
      is_closed = true;
      throw Sar_DBException(msg);
    }
  };

  int SQLiteStorage::next_id(std::string classname) {
    sqlite3_stmt *pSelect;
    std::string query = "SELECT id FROM id_maximums WHERE classname = \"" + classname + "\"";
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    if(rc != SQLITE_ROW){
      debug("could not find max id int for " + classname + " - starting at 0");
      rc = sqlite3_finalize(pSelect);
      query = "INSERT INTO id_maximums (id,classname) VALUES(0, \"" + classname + "\")";
      execute(query);
      return 0;
    }
    int maxid = sqlite3_column_int(pSelect, 0) + 1;
    rc = sqlite3_finalize(pSelect);
    std::string maxid_s;
    int_to_string(maxid, maxid_s);
    execute("UPDATE id_maximums SET id = " + maxid_s + " WHERE classname = \"" + classname + "\"");
    return maxid;
  };

  int SQLiteStorage::current_id(std::string classname) {
    sqlite3_stmt *pSelect;
    std::string query = "SELECT id FROM id_maximums WHERE classname = \"" + classname + "\"";
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    if(rc != SQLITE_ROW)
      return -1;
    int id = sqlite3_column_int(pSelect, 0);
    rc = sqlite3_finalize(pSelect);
    return id;
  };
 
  void SQLiteStorage::initialize_tables(std::string classname) {
    char *errMsg;
    std::string tablename;
    int rc;

    // make table for string values
    tablename = classname + "_s";
    if(!table_is_initialized(tablename)) {
      debug("initializing table " + tablename);
      std::string query = "CREATE TABLE IF NOT EXISTS " + tablename + " (id INT, keyname VARCHAR(255), "
	"value VARCHAR(" + VALUE_MAX_SIZE_S + "))";
      execute(query);
      initialized_tables.push_back(tablename);
    }

    // make table for string values
    tablename = classname + "_i";
    if(!table_is_initialized(tablename)) {
      debug("initializing table " + tablename);
      execute("CREATE TABLE IF NOT EXISTS " + tablename + " (id INT, keyname VARCHAR(255), value INT)");
      initialized_tables.push_back(tablename);
    }

    debug("Finished initializing tables for class " + classname);
  };

  void SQLiteStorage::get(int id, std::string classname, SarMap<std::string>& values) {
    sqlite3_stmt *pSelect;
    std::string tablename = classname + "_s";
    std::string id_s;
    int_to_string(id, id_s);
    std::string query = "SELECT keyname,value FROM " + tablename + " WHERE id = " + id_s;
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    char c_key[255];
    char c_value[VALUE_MAX_SIZE + 1];
    while(rc == SQLITE_ROW){
      snprintf(c_key, 255, "%s", sqlite3_column_text(pSelect, 0));
      snprintf(c_value, VALUE_MAX_SIZE, "%s", sqlite3_column_text(pSelect, 1));
      values[std::string(c_key)] = std::string(c_value);
      rc = sqlite3_step(pSelect);
    }    
    rc = sqlite3_finalize(pSelect);
  };

  void SQLiteStorage::get(int id, std::string classname, SarMap<int>& values) {
    sqlite3_stmt *pSelect;
    std::string tablename = classname + "_i";
    std::string id_s;
    int_to_string(id, id_s);
    std::string query = "SELECT keyname,value FROM " + tablename + " WHERE id = " + id_s;
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      char c_key[255];
      snprintf(c_key, 255, "%s", sqlite3_column_text(pSelect, 0));
      int value = sqlite3_column_int(pSelect, 1);
      values[std::string(c_key)] = value;
      rc = sqlite3_step(pSelect);
    }    
    rc = sqlite3_finalize(pSelect);
  };

  void SQLiteStorage::set(int id, std::string classname, SarMap<std::string> values, bool insert) {
    std::string query;
    std::string tablename = classname + "_s";
    std::string id_s;
    int_to_string(id, id_s);
    if(insert) {
      for(std::map<std::string,std::string>::iterator i=values.begin(); i!=values.end(); ++i) {
	query = "INSERT INTO " + tablename + " (id,keyname,value) VALUES (" + id_s + ",\"" + std::string((*i).first) + "\",\"" + std::string((*i).second) + "\")";
	execute(query);
      }
    } else {
      for(std::map<std::string,std::string>::iterator i=values.begin(); i!=values.end(); ++i) {
	query = "UPDATE " + tablename + " SET value=\"" + std::string((*i).second) + "\" WHERE id=" + id_s \
	  + " AND keyname=\"" + std::string((*i).first) + "\"";
	execute(query);
      }
    }
  };

  void SQLiteStorage::set(int id, std::string classname, SarMap<int> values, bool insert) {
    std::string query;
    std::string tablename = classname + "_i";
    std::string id_s;
    std::string value_s;
    int_to_string(id, id_s);
    if(insert) {
      for(std::map<std::string,int>::iterator i=values.begin(); i!=values.end(); ++i) {
	int_to_string((*i).second, value_s);
	query = "INSERT INTO " + tablename + " (id,keyname,value) VALUES (" + id_s + ",\"" + std::string((*i).first) + "\"," + value_s + ")";
	execute(query);
      }
    } else {
      for(std::map<std::string,int>::iterator i=values.begin(); i!=values.end(); ++i) {
	int_to_string((*i).second, value_s);
	query = "UPDATE " + tablename + " SET  value=" + value_s + " WHERE id=" + id_s \
	  + " AND keyname=\"" + std::string((*i).first) + "\"";
	execute(query);
      }
    }
  };

  void SQLiteStorage::del(int id, std::string classname, SarVector<std::string> keys, coltype ct) {
    std::string tablename = (ct == STRING) ? classname+"_s" : classname+"_i";
    std::string id_s;
    int_to_string(id, id_s);
    for(unsigned int i=0; i < keys.size(); i++)
      execute("DELETE FROM " + tablename + " WHERE id = " + id_s + " AND keyname = \"" + keys[i] + "\"");
  };

  void SQLiteStorage::execute(std::string query) {
    debug("SQLite executing: " + query);
    // this var doesn't matter cause it's the same as what will be printed by test_result
    char *errMsg; 
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, query);
  };

  void SQLiteStorage::delete_record(int id, std::string classname) {
    char *errMsg;
    int rc;
    std::string id_s, query, tablename;
    int_to_string(id, id_s);

    // delete all string values
    tablename = classname + "_s";
    execute("DELETE FROM " + tablename + " WHERE id = " + id_s);

    // delete all int values
    tablename = classname + "_i";
    execute("DELETE FROM " + tablename + " WHERE id = " + id_s);

    execute("DELETE FROM relationships WHERE class_one = \"" + classname + "\" AND class_one_id=" + id_s);
    execute("DELETE FROM relationships WHERE class_two = \"" + classname + "\" AND class_two_id=" + id_s);
  };

  void SQLiteStorage::delete_records(std::string classname) {
    char *errMsg;
    int rc;
    std::string query, tablename;

    // delete string values table
    tablename = classname + "_s";
    execute("DELETE FROM " + tablename);

    // delete int values table
    tablename = classname + "_i";
    execute("DELETE FROM " + tablename);

    // delete entries from relationships
    execute("DELETE FROM relationships WHERE class_one = \"" + classname + "\" OR class_two = \"" + classname + "\"");

    // delete max id
    execute("DELETE FROM id_maximums WHERE classname = \"" + classname + "\"");
  };

  void SQLiteStorage::set(int id, std::string classname, SarVector<int> related, std::string related_classname) {
    std::string s_id, related_id;
    int_to_string(id, s_id);
    debug("Adding related " + related_classname + "s to a " + classname);
    bool swap = (strcmp(classname.c_str(), related_classname.c_str()) > 0) ? true : false;
    for(SarVector<int>::size_type i=0; i<related.size(); i++) {
      int_to_string(related[i], related_id);
      if(swap)
	execute("INSERT INTO relationships (class_one, class_one_id, class_two, class_two_id) VALUES(\"" + \
		related_classname + "\", " + related_id + ", \"" + classname + "\", " + s_id + ")");
      else
	execute("INSERT INTO relationships (class_two, class_two_id, class_one, class_one_id) VALUES(\"" + \
		related_classname + "\", " + related_id + ", \"" + classname + "\", " + s_id + ")");
    }
  };

  void SQLiteStorage::get(int id, std::string classname, std::string related_classname, SarVector<int>& related) {
    sqlite3_stmt *pSelect;
    std::string s_id, query;
    int_to_string(id, s_id);
    debug("Getting related " + related_classname + "s to a " + classname);
    bool swap = (strcmp(classname.c_str(), related_classname.c_str()) > 0) ? true : false;
    if(swap)
      query = "SELECT class_one_id FROM relationships WHERE class_one = \"" + related_classname + "\" AND class_two_id = " 
	+ s_id + " AND class_two = \"" + classname + "\"";
    else
      query = "SELECT class_two_id FROM relationships WHERE class_two = \"" + related_classname + "\" AND class_one_id = " 
	+ s_id + " AND class_one = \"" + classname + "\"";

    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      related << sqlite3_column_int(pSelect, 0);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);
  };
  
  void SQLiteStorage::get(int id, std::string classname, SarMap< SarVector<int> >& sm) {
    sqlite3_stmt *pSelect;
    std::string s_id, query, key;
    int_to_string(id, s_id);
    debug("Getting all related objects to a " + classname);

    query = "SELECT class_one, class_one_id FROM relationships WHERE class_two=\"" + classname + "\" AND class_two_id=" + s_id;
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      char c_key[255];
      snprintf(c_key, 255, "%s", sqlite3_column_text(pSelect, 0));
      key = std::string(c_key);
      if(!sm.has_key(key))
	sm[key] = SarVector<int>();
      sm[key] << sqlite3_column_int(pSelect, 1);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);    

    query = "SELECT class_two, class_two_id FROM relationships WHERE class_one=\"" + classname + "\" AND class_one_id=" + s_id;
    debug(query);
    rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      char c_key[255];
      snprintf(c_key, 255, "%s", sqlite3_column_text(pSelect, 0));
      key = std::string(c_key);
      if(!sm.has_key(key))
	sm[key] = SarVector<int>();
      sm[key] << sqlite3_column_int(pSelect, 1);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);    
  };

  void SQLiteStorage::del(int id, std::string classname, SarVector<int> related, std::string related_classname) {
    if(related.size() == 0)
      return;
    std::string s_id, related_id;
    int_to_string(id, s_id);
    debug("Deleting some related " + related_classname + "s to a " + classname);
    bool swap = (strcmp(classname.c_str(), related_classname.c_str()) > 0) ? true : false;

    std::string idlist = "(";
    for(SarVector<int>::size_type i=0; i<related.size(); i++) {
      int_to_string(related[i], related_id);
      idlist += related_id;
      if(i!=(related.size() - 1))
	idlist += ",";
    }
    idlist += ")";

    if(swap)
      execute("DELETE FROM relationships WHERE class_two=\"" + classname + "\" AND class_two_id=" + s_id \
	      + " AND class_one=\"" + related_classname + "\" and class_one_id IN " + idlist);
    else
      execute("DELETE FROM relationships WHERE class_one=\"" + classname + "\" AND class_one_id=" + s_id \
	      + " AND class_two=\"" + related_classname + "\" and class_two_id IN " + idlist);
  };
  
  // some searching stuff
  void SQLiteStorage::get(std::string classname, SarVector<int>& results) {
    sqlite3_stmt *pSelect;
    SarVector<int> others;
    std::string tablename = classname + "_s";
    debug("Getting all objects of type " + classname);
    std::string query = "SELECT DISTINCT id FROM " + tablename;
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      results << sqlite3_column_int(pSelect, 0);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);
   
    tablename = classname + "_i";
    query = "SELECT DISTINCT id FROM " + tablename;
    debug(query);
    rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      others << sqlite3_column_int(pSelect, 0);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);
    results.unionize(others);
    others.clear();

    query = "SELECT DISTINCT class_one_id FROM relationships WHERE class_one = \"" + classname + "\"";
    debug(query);
    rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      others << sqlite3_column_int(pSelect, 0);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);
    results.unionize(others);
    others.clear();

    query = "SELECT DISTINCT class_two_id FROM relationships WHERE class_two = \"" + classname + "\"";
    debug(query);
    rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      others << sqlite3_column_int(pSelect, 0);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);
    results.unionize(others);
    others.clear();
  };

  void SQLiteStorage::get_where(std::string classname, std::string key, Where * where, SarVector<int>& results) {
    bool isnot = where->isnot;
    std::string swhere, table;
    sqlite3_stmt *pSelect;
    if(where->ct == INTEGER) {
      table = classname + "_i";
      std::string sint, second_sint;
      int_to_string(where->ivalue, sint);
      switch(where->type) {
      case GREATERTHAN:
	swhere = ((isnot) ? "<= " : "> ") + sint ;
	break;
      case LESSTHAN:
	swhere = ((isnot) ? ">= " : "< ") + sint;
	break;
      case EQUALS:
	swhere = ((isnot) ? "!= " : "= ") + sint;	
	break;
      case BETWEEN:
	int_to_string(where->ivaluetwo, second_sint);
	swhere = ((isnot) ? "NOT BETWEEN " : "BETWEEN " ) + sint + " AND " + second_sint;
	break;
      }
    } else { //string
      table = classname + "_s";
      switch(where->type) {
      case STARTSWITH:
	swhere = ((isnot) ? "NOT LIKE \"" : "LIKE \"") + where->svalue + "%\"";	
	break;
      case ENDSWITH:
	swhere = ((isnot) ? "NOT LIKE \"%" : "LIKE \"%") + where->svalue + "\"";	
	break;
      case EQUALS:
	swhere = ((isnot) ? "!= \"" : "= \"") + where->svalue + "\"";	
	break;
      case CONTAINS:
	swhere = ((isnot) ? "NOT LIKE \"%" : "LIKE \"%") + where->svalue + "%\"";	
      }
    }
    std::string query = "SELECT id FROM " + table + " WHERE keyname=\"" + key + "\" AND value " + swhere;
    debug(query);
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      throw Sar_DBException("error preparing sql query: " + query);
    }
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      results << sqlite3_column_int(pSelect, 0);
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);
  };

};
