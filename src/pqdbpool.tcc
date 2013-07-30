#include "pqdbpool.hh"
#include "str.hh"

using namespace tamer;

namespace pq {

DBPool::DBPool(const String& host, uint32_t port)
    : host_(host), port_(port), min_(5), max_(10) {
}

DBPool::DBPool(const String& host, uint32_t port, uint32_t min, uint32_t max)
    : host_(host), port_(port), min_(min), max_(max) {
}

DBPool::~DBPool() {
    clear();
}

void DBPool::connect() {
#if HAVE_LIBPQ
    for (uint32_t i = 0; i < min_; ++i) {
        PGconn* conn = connect_one();
        assert(conn);

        conn_.push_back(conn);
        pool_.push(conn);
    }
#else
    mandatory_assert(false && "Database not configured.");
#endif
}

void DBPool::clear() {
#if HAVE_LIBPQ
    while(!pool_.empty())
        pool_.pop();

    while(!conn_.empty()) {
        PGconn* conn = conn_.back();
        conn_.pop_back();
        PQfinish(conn);
    }
#endif
}

#if HAVE_LIBPQ
tamed void DBPool::execute(String query, event<Json> e) {
    tvars {
        PGconn* conn;
        int32_t err;
    }

    twait { next_connection(make_event(conn)); }

#if 0
    {
        // todo: this does not handle strings with binary data!
        char buff[1024];
        size_t sz = PQescapeStringConn(conn, buff, query.data(), query.length(), &err);
        String q(buff, sz);

        err = PQsendQuery(conn, q.c_str());
    }
#else
    err = PQsendQuery(conn, query.c_str());
#endif

    mandatory_assert(err == 1 && "Could not send query to DB.");

    do {
        twait { tamer::at_fd_read(PQsocket(conn), make_event()); }
        err = PQconsumeInput(conn);
        mandatory_assert(err == 1 && "Error reading data from DB.");
    } while(PQisBusy(conn));

    PGresult* result = PQgetResult(conn);
    ExecStatusType status = PQresultStatus(result);
    Json ret;

    switch(status) {
        case PGRES_COMMAND_OK:
            // command (e.g. insert, delete) returns no data
            break;
        case PGRES_TUPLES_OK: {
            int32_t nrows = PQntuples(result);
            int32_t ncols = PQnfields(result);

            for (int32_t r = 0; r < nrows; ++r) {
                ret.push_back(Json::make_array_reserve(ncols));
                for (int32_t c = 0; c < ncols; ++c) {
                    ret[r][c] = Str(PQgetvalue(result, r, c), PQgetlength(result, r, c));
                }
            }
            break;
        }
        default: {
            std::cerr << "Error getting result of DB query. " << std::endl
                      << "  Status:  " << PQresStatus(status) << std::endl
                      << "  Message: " << PQresultErrorMessage(result) << std::endl;
            mandatory_assert(false);
            break;
        }
    }

    PQclear(result);
    result = PQgetResult(conn);
    mandatory_assert(!result && "Should only be one result!");

    replace_connection(conn);
    e(ret);
}

void DBPool::next_connection(tamer::event<PGconn*> e) {
    if (!pool_.empty()) {
        e(pool_.front());
        pool_.pop();
    }
    else if (conn_.size() < max_) {
        conn_.push_back(connect_one());
        e(conn_.back());
    }
    else
        waiting_.push(e);
}

void DBPool::replace_connection(PGconn* conn) {
    if (!waiting_.empty()) {
        waiting_.front()(conn);
        waiting_.pop();
    }
    else
        pool_.push(conn);
}

PGconn* DBPool::connect_one() {
    String cs = "dbname=pequod host=" + host_ + " port=" + String(port_);
    PGconn* conn = PQconnectdb(cs.c_str());
    mandatory_assert(conn);
    mandatory_assert(PQstatus(conn) != CONNECTION_BAD);
    return conn;
}
#else

tamed void DBPool::execute(String query, event<Json> e) {
    mandatory_assert(false && "Database not configured.");
}

#endif

}
