#ifndef _DB_POOL_H_
#define _DB_POOL_H_

#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include <memory.h>
#include <pthread.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#include <mysql.h>

#include <logger/logger.h>

class db_exception {
public:
	db_exception(MYSQL *m_handler, const char *_stage = "unknown")
		: msg(mysql_error(m_handler))
		, m_stage(_stage)
	{
	}

	db_exception(MYSQL_STMT *m_stmt, const char *_stage = "unknown")
		: msg(mysql_stmt_error(m_stmt))
		, m_stage(_stage)
	{
	}

	db_exception(const std::string &_msg, const char *_stage = "unknown")
		: msg(_msg)
		, m_stage(_stage)
	{
	}

	const char *what() const {
		return msg.c_str();
	}

    const char *stage() const {
        return m_stage;
    }

	std::string msg;
    const char *m_stage;
};

class DBCred {
public:
	DBCred(const std::string &_host, const std::string &_user, const std::string &_passwd, const std::string &_db,
		unsigned int _port = 0)
		: host(_host)
		, user(_user)
		, passwd(_passwd)
		, db(_db)
		, port(_port)
	{
	}

	std::string host, user, passwd, db;
	unsigned int port;
};

class DBConn {
public:
	DBConn(const DBCred &cred)
	{
		mysql_init(&m_handler);

		if(!mysql_real_connect(&m_handler, cred.host.c_str(), cred.user.c_str(), cred.passwd.c_str(),
			cred.db.c_str(), cred.port, NULL, 0))
		{
			throw db_exception(&m_handler);
		}

        time(&m_last_used);

        LogSQL("connection established");
	}

	~DBConn()
	{
		mysql_close(&m_handler);
        LogSQL("connection closed");
	}

	MYSQL			m_handler;
    time_t          m_last_used;
};

typedef struct {
    unsigned long length;
	u_char *buffer;
    unsigned long actual_length;
    my_bool is_null;
    MYSQL_FIELD *field;
} column_data_t;

class BlobParam {
public:
    BlobParam(int _param, const std::string &_value)
        : param(_param)
        , value(_value)
    {
    }

    BlobParam(const BlobParam &other)
        : param(other.param)
        , value(other.value)
    {
    }

    int param;
    const std::string value;
};

class DBStmt {
public:
	DBStmt(DBConn &conn, const std::string &stmt)
		: m_handler(&conn.m_handler)
		, m_stmt(mysql_stmt_init(&conn.m_handler))
		, m_bind_in(0)
		, m_bind_out(0)
        , m_column_data(0)
		, m_meta_result(0)
        , m_num_fields(0)
        , m_param_count(0)
        , m_blob_params()
	{
        LogSQL(stmt);

		if(mysql_stmt_prepare(m_stmt, stmt.c_str(), stmt.size())) {
			throw db_exception(m_stmt, "prepare");
		}

		m_param_count = mysql_stmt_param_count(m_stmt);

		if(m_param_count != 0) {
			m_bind_in = new MYSQL_BIND[m_param_count];
		}

		m_meta_result = mysql_stmt_result_metadata(m_stmt);

		if(m_meta_result != NULL) {
            m_num_fields = mysql_num_fields(m_meta_result);

            if(m_num_fields != 0) {
                m_bind_out = new MYSQL_BIND[m_num_fields];
                m_column_data = new column_data_t[m_num_fields];

                memset(m_bind_out, 0, sizeof(MYSQL_BIND) * m_num_fields);
            }

            MYSQL_FIELD *field;
            MYSQL_BIND *bind;
            column_data_t *column_data;

            bind = m_bind_out;
            column_data = m_column_data;

            while((field = mysql_fetch_field(m_meta_result)) != NULL)
            {
				if(field->type != MYSQL_TYPE_BLOB)
                {
					column_data->buffer = new u_char[is_time_field(field->type) ?
                        sizeof(MYSQL_TIME) : field->length];
					column_data->length = field->length;
					column_data->field = field;

					bind->buffer_type = field->type;
					bind->buffer = column_data->buffer;
					bind->buffer_length = column_data->length;
					bind->length = &column_data->actual_length;
					bind->is_null = &column_data->is_null;
				}
				else {
					column_data->buffer = NULL;
					column_data->length = 0;
					column_data->field = field;

					bind->buffer_type = field->type;
					bind->buffer = NULL;
					bind->buffer_length = 0;
					bind->length = &column_data->actual_length;
					bind->is_null = &column_data->is_null;
				}

                bind++;
                column_data++;
            }
        }
	}

	~DBStmt()
	{
		for(unsigned i = 0 ; i != m_num_fields ; i++) {
			delete [] (u_char*)m_bind_out[i].buffer;
		}

		delete [] m_bind_in;
		delete [] m_bind_out;
        delete [] m_column_data;

		if(m_meta_result != 0) {
			mysql_free_result(m_meta_result);
		}

		if(mysql_stmt_close(m_stmt)) {
			throw db_exception(m_stmt, "stmt_close");
		}
	}

	void execute()
	{
		if(m_param_count) {
			if(mysql_stmt_bind_param(m_stmt, m_bind_in)) {
				throw db_exception(m_stmt, "bind_param");
			}

            for(std::list<BlobParam>::const_iterator i = m_blob_params.begin() ; i != m_blob_params.end() ; i++) {
                if(mysql_stmt_send_long_data(m_stmt, i->param, i->value.c_str(), i->value.size()))
                {
                    throw db_exception(m_stmt, "send_long_data");
                }
            }
		}

		if(mysql_stmt_execute(m_stmt)) {
			throw db_exception(m_stmt, "execute");
		}

		if(m_num_fields) {
			if(mysql_stmt_bind_result(m_stmt, m_bind_out)) {
				throw db_exception(m_stmt, "bind_result");
			}
		}
	}

	bool fetch() {
		int result = mysql_stmt_fetch(m_stmt);

		if(result == MYSQL_NO_DATA) {
			return false;
		}

        if(result == 1) {
            throw db_exception(m_stmt, "stmt_fetch");
        }

		for(unsigned i = 0 ; i != m_num_fields ; i++) {
			if(m_column_data[i].field->type == MYSQL_TYPE_BLOB) {
				delete [] (u_char*)m_column_data[i].buffer;

				m_column_data[i].buffer = NULL;
				
				if(m_column_data[i].actual_length != 0) {
					m_column_data[i].buffer = new u_char[m_column_data[i].actual_length];
					m_bind_out[i].buffer = m_column_data[i].buffer;
					m_bind_out[i].buffer_length = m_column_data[i].actual_length;

					if(mysql_stmt_fetch_column(m_stmt, &m_bind_out[i], i, 0)) {
						throw db_exception(m_stmt, "fetch_column");
					}
				}
			}
		}

		return true;
	}

	void bindInt(size_t colno, const int &value) {
		if(colno < m_param_count) {
			m_bind_in[colno].buffer_type = MYSQL_TYPE_LONG;
			m_bind_in[colno].is_unsigned = 0;
			m_bind_in[colno].buffer = (void*)&value;
			m_bind_in[colno].buffer_length = sizeof(int);
			m_bind_in[colno].length = NULL;
			m_bind_in[colno].is_null = NULL;
		}
	}

	void bindDouble(size_t colno, const double &value) {
		if(colno < m_param_count) {
			m_bind_in[colno].buffer_type = MYSQL_TYPE_DOUBLE;
			m_bind_in[colno].is_unsigned = 0;
			m_bind_in[colno].buffer = (void*)&value;
			m_bind_in[colno].buffer_length = sizeof(double);
			m_bind_in[colno].length = NULL;
			m_bind_in[colno].is_null = NULL;
		}
	}

	void bindString(size_t colno, const std::string &value) {
		if(colno < m_param_count) {
			m_bind_in[colno].buffer_type = MYSQL_TYPE_STRING;
			m_bind_in[colno].is_unsigned = 0;
			m_bind_in[colno].buffer = (void*)value.c_str();
			m_bind_in[colno].buffer_length = value.size();
			m_bind_in[colno].length = NULL;
			m_bind_in[colno].is_null = NULL;
		}
	}

	void bindBlob(size_t colno, const std::string &value) {
		if(colno < m_param_count) {
            memset(&m_bind_in[colno], 0, sizeof(MYSQL_BIND));
			m_bind_in[colno].buffer_type = MYSQL_TYPE_STRING;
			m_bind_in[colno].length = NULL;
			m_bind_in[colno].is_null = NULL;

            m_blob_params.push_back(BlobParam(colno, value));
		}
	}

	int asInt(int colno) const {
		int i;
		std::istringstream s(this->asString(colno));
		s >> i;
		return i;
	}

	std::string asString(size_t colno) const {
        if(colno >= m_num_fields) {
            return "";
        }

        std::ostringstream o;

        column_to_string(m_column_data + colno, o);

		return o.str();
	}

    bool is_null(int colno) const {
        return m_column_data[colno].is_null ? true : false;
    }

    int last_inserted_id() const
    {
        return (int)mysql_stmt_insert_id(m_stmt);
    }

private:
    static bool is_time_field(unsigned field_type)
    {
        return field_type == MYSQL_TYPE_TIME || field_type == MYSQL_TYPE_DATE ||
            field_type == MYSQL_TYPE_DATETIME || field_type == MYSQL_TYPE_TIMESTAMP;
    }

    static void convert_timestamp(MYSQL_TIME *ts, std::ostream &o) {
        o << std::setfill('0')
          << std::setw(4) << ts->year << "-"
          << std::setw(2) << ts->month << "-"
          << std::setw(2) << ts->day << " "
          << std::setw(2) << ts->hour << ":"
          << std::setw(2) << ts->minute << ":"
          << std::setw(2) << ts->second
         ;
    }

    static void column_to_string(const column_data_t *col, std::ostream &o) {
        switch(col->field->type) {
			case MYSQL_TYPE_TINY:
                o << (int)*((signed char*)col->buffer);
                break;
            case MYSQL_TYPE_SHORT:
                o << *((short int*)(col->buffer));
                break;
            case MYSQL_TYPE_LONG:
                o << *((int*)(col->buffer));
                break;
            case MYSQL_TYPE_LONGLONG:
                o << *((long long int*)(col->buffer));
                break;
            case MYSQL_TYPE_DOUBLE:
                o << *((double*)(col->buffer));
                break;
            case MYSQL_TYPE_FLOAT:
                o << *((float*)(col->buffer));
                break;
            case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
                o.write((const char*)col->buffer, col->actual_length);
                break;
            case MYSQL_TYPE_BLOB:
                o.write((const char*)col->buffer, col->actual_length);
                break;
            case MYSQL_TYPE_TIME:
                throw db_exception("Don't know how to convert type MYSQL_TYPE_TIME", "stmt_fetch");
            case MYSQL_TYPE_DATE:
                throw db_exception("Don't know how to convert type MYSQL_TYPE_DATE", "stmt_fetch");
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_TIMESTAMP:
                convert_timestamp((MYSQL_TIME*)col->buffer, o);
                break;
            default:
                break;
        }
    }

	MYSQL *m_handler;
	MYSQL_STMT *m_stmt;
	MYSQL_BIND *m_bind_in;
	MYSQL_BIND *m_bind_out;
    column_data_t *m_column_data;
	MYSQL_RES  *m_meta_result;
	size_t m_num_fields;
	size_t m_param_count;
	my_ulonglong m_num_rows;
    std::list<BlobParam> m_blob_params;
};

class DBScopedLock {
public:
    DBScopedLock(DBConn &_conn, const std::string &_lockname)
        : conn(_conn)
        , lockname(_lockname)
    {
        DBStmt stmt(conn, "select get_lock(?, 0)");

        stmt.bindString(0, lockname);

        stmt.execute();

        if(!stmt.fetch() || stmt.is_null(0) || (stmt.asInt(0) != 1)) {
            throw db_exception("cannot optain version lock", "get_lock");
        }

        LogInfo("acquire lock " << lockname);
    }

    ~DBScopedLock()
    {
        DBStmt stmt(conn, "do release_lock(?)");
        stmt.bindString(0, lockname);
        stmt.execute();
        LogInfo("release lock " << lockname);
    }

private:
    DBConn &conn;
    std::string lockname;
};

class DBPool {
public:
    DBPool(const DBCred &cred, size_t _min_connections, size_t _max_connections)
        : m_cred(cred)
        , m_all_connections()
        , m_available_connections()
        , m_min_connections(_min_connections)
        , m_max_connections(_max_connections)
        , m_num_connections(0)
        , m_num_available(0)
    {
        pthread_cond_init(&m_not_empty, NULL);
        pthread_mutex_init(&m_lock, NULL);
    }

    ~DBPool() {
        for(std::list<DBConn*>::const_iterator i = m_all_connections.begin() ; i != m_all_connections.end() ; i++) {
            delete *i;
        }
        pthread_mutex_destroy(&m_lock);
        pthread_cond_destroy(&m_not_empty);
    }

    DBConn *grow() {

        std::auto_ptr<DBConn> conn(new DBConn(m_cred));

        m_all_connections.push_back(conn.get());
        m_num_connections++;

        LogSQL("grow connection " << conn.get());

        return conn.release();
    }

    DBConn *get() {
        std::auto_ptr<DBConn> conn;

        pthread_mutex_lock(&m_lock);

        if(m_num_available == 0 && m_num_connections < m_max_connections) {
            /*
             * If there is room to grow connections then do it
             */
            conn.reset(grow());
        }
        else {
            /*
             * Otherwise wait on queue and get existing connection
             */
            if(m_available_connections.empty())
                pthread_cond_wait(&m_not_empty, &m_lock);

            conn.reset(m_available_connections.front());
            m_available_connections.pop_front();
            m_num_available--;
        }

        time(&conn.get()->m_last_used);

        pthread_mutex_unlock(&m_lock);

        LogSQL("get connection " << conn.get() << " (" << m_num_available << '/' << m_num_connections << ')');

        return conn.release();
    }

    void put(DBConn *conn) {
        LogSQL("put connection " << conn << " (" << m_num_available << '/' << m_num_connections << ')');

        pthread_mutex_lock(&m_lock);
        m_available_connections.push_back(conn);
        m_num_available++;
        time(&conn->m_last_used);
        pthread_cond_signal(&m_not_empty);
        pthread_mutex_unlock(&m_lock);
    }

    static void start_maintenance_thread(DBPool&);
    static void join_maintenance_thread();

private:
    static void *maintenance_thread_func(void*);

private:
    pthread_mutex_t     m_lock;
    pthread_cond_t      m_not_empty;

    DBCred              m_cred;
    std::list<DBConn*>  m_all_connections;
    std::list<DBConn*>  m_available_connections;

    size_t              m_min_connections;
    size_t              m_max_connections;
    size_t              m_num_connections;
    size_t              m_num_available;

    static              std::list<DBPool*> pools;
    static              pthread_t maintenance_thread;
    static              pthread_mutex_t startup_lock;
    static              pthread_mutex_t maintenance_lock;
    static volatile     bool maintenance_thread_running;
    static volatile     bool maintenance_thread_exiting;
};

class DBConnHolder {
public:
    DBConnHolder(DBPool &_pool)
        : m_pool(_pool)
        , m_conn(m_pool.get())
    {
    }

    DBConn &get() const {
        return *m_conn;
    }

    ~DBConnHolder()
    {
        if(m_conn != 0) {
            m_pool.put(m_conn);
        }
    }

private:
    DBPool          &m_pool;
    DBConn          *m_conn;
};

#endif
