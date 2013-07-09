#ifndef PQ_PERSISTENT_HH
#define PQ_PERSISTENT_HH
#include "str.hh"
#include "string.hh"
#include "readerwriterqueue.h"
#include <tamer/tamer.hh>
#include <boost/thread.hpp>
#include <atomic>

namespace pq {
class Server;

class PersistentStore {
  public:
    typedef std::pair<String,String> Result;
    typedef std::vector<Result> ResultSet;

    virtual ~PersistentStore() { }

    virtual void scan(Str first, Str last, ResultSet& results) = 0;
    virtual int32_t put(Str key, Str value) = 0;
    virtual String get(Str key) = 0;
    virtual void run_monitor(Server& server) = 0;
};

class PersistentOp {
  public:
    virtual ~PersistentOp() { }
    virtual void operator()(PersistentStore*) = 0;
};

class PersistentRead : public PersistentOp {
  public:
    PersistentRead(Str first, Str last, PersistentStore::ResultSet& rs);

    virtual void operator()(PersistentStore*);
    void set_trigger(tamer::event<> t);

  private:
    PersistentStore::ResultSet& rs_;
    tamer::event<> tev_;
    String first_;
    String last_;
};

class PersistentWrite : public PersistentOp {
  public:
    PersistentWrite(Str key, Str value);
    virtual void operator()(PersistentStore*);

  private:
    String key_;
    String value_;
};

class PersistentStoreThread {
  public:
    PersistentStoreThread(PersistentStore* store);
    ~PersistentStoreThread();

    void enqueue(PersistentOp* op);
    void run();

  private:
    PersistentStore* store_;
    boost::thread worker_;
    boost::mutex mu_;
    boost::condition_variable cond_;
    moodycamel::ReaderWriterQueue<PersistentOp*> pending_;
    std::atomic<bool> running_;
};

}

#if HAVE_DB_CXX_H
#include <db_cxx.h>

class BerkeleyDBStore : public pq::PersistentStore {

  public:
    BerkeleyDBStore(std::string eH = "./db/localEnv",
                    std::string dbN = "pequod.db",
                    uint32_t e_flags = BerkeleyDBStore::env_flags_,
                    uint32_t d_flags = BerkeleyDBStore::db_flags_);
    ~BerkeleyDBStore();

    virtual void scan(Str, Str, pq::PersistentStore::ResultSet&);
    void init(uint32_t, uint32_t);
    virtual int32_t put(Str, Str);
    virtual String get(Str);
    virtual void run_monitor(pq::Server& server);

  private:
    static const uint32_t env_flags_ = DB_CREATE | DB_INIT_MPOOL;
    static const uint32_t db_flags_ = DB_CREATE;
    static const uint32_t cursor_flags_ = DB_CURSOR_BULK;
    std::string env_home_, db_name_;
    DbEnv *env_;
    Db *dbh_;
};
#endif

#if HAVE_PQXX_NOTIFICATION

//#include "pqxx/binarystring"
#include "pqxx/connection"
//#include "pqxx/cursor"
//#include "pqxx/errorhandler"
//#include "pqxx/except"
//#include "pqxx/field"
//#include "pqxx/largeobject"
//#include "pqxx/nontransaction"
//#include "pqxx/notification"
//#include "pqxx/notify-listen"
//#include "pqxx/pipeline"
//#include "pqxx/prepared_statement"
#include "pqxx/result"
//#include "pqxx/robusttransaction"
//#include "pqxx/subtransaction"
//#include "pqxx/strconv"
//#include "pqxx/tablereader"
//#include "pqxx/tablewriter"
#include "pqxx/transaction"
//#include "pqxx/transactor"
//#include "pqxx/tuple"
//#include "pqxx/util"
//#include "pqxx/version"

class PostgreSQLStore : public pq::PersistentStore {

  public:
    PostgreSQLStore(std::string connection_string = "dbname=pequod port=5432 host=127.0.0.1");
    ~PostgreSQLStore();

    virtual void scan(Str, Str, pq::PersistentStore::ResultSet&);
    virtual int32_t put(Str, Str);
    virtual String get(Str);
    virtual void run_monitor(pq::Server& server);

  private:
    pqxx::connection *dbh_;
    tamer::fd monitor_fd_;

    tamed void monitor_db(pq::Server& server);
};

#endif

#endif
